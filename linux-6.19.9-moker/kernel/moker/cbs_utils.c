#include <uapi/linux/sched/types.h>
#include <linux/sched/types.h>
#include <linux/hrtimer.h>
#include "../sched/sched.h"
#include "cbs_utils.h"

/*
* Callback function when the budget expires
*/

static enum hrtimer_restart cbs_budget_timer_callback(struct hrtimer *timer)
{
	struct sched_cbs_entity *cbs_entity = container_of(
		timer, struct sched_cbs_entity, budget_control_timer);
	struct task_struct *task_struct = cbs_task_of(cbs_entity);
	struct rq *rq;
	struct rq_flags rf;

	trace_printk(
		"[CBS][BUDGET_EXPIRE] pid=%d comm=%s deadline=%llu runtime=%lld\n",
		task_struct->pid, task_struct->comm,
		cbs_entity->absolute_deadline, cbs_entity->remaining_runtime);

	if (!task_struct)
		return HRTIMER_NORESTART;

	rq = task_rq_lock(task_struct, &rf);

	dequeue_cbs_entity(cbs_entity, &rq->cbs);

	// Pushing the deadline and replenishing the server just like in the paper officers Baltarejo and Maia
	cbs_entity->absolute_deadline += cbs_entity->declared_period;
	cbs_entity->remaining_runtime = cbs_entity->max_capacity;

	trace_printk(
		"[CBS][REPLENISH] pid=%d new_deadline=%llu new_runtime=%lld\n",
		task_struct->pid, cbs_entity->absolute_deadline,
		cbs_entity->remaining_runtime);

	enqueue_cbs_entity(cbs_entity, &rq->cbs);

	task_rq_unlock(rq, task_struct, &rf);

	// This call will force the kernel to call __schedule() which in turns calls put_prev_task and set_next_task
	// In put_prev_task we will cancel the timer and account the remaining budget
	if (task_current(rq, task_struct)) {
		trace_printk("[CBS][FORCE_RESCHED] pid=%d\n", task_struct->pid);
		resched_curr(rq);
	}

	return HRTIMER_NORESTART;
}

void enqueue_cbs_entity(struct sched_cbs_entity *cbs_entity,
			struct cbs_rq *cbs_rq)
{
	raw_spin_lock(&cbs_rq->lock);
	/* 
		FIRST ACTIVATION OF A TASK (Does not matter if soft or hard task)
		When a new task is created __setparam_cbs sets the absolute deadline value to zero.
		Need to give it an actual absolute deadline before enqueing for the first time
	*/
	u64 time_now = ktime_get_ns();

	if (cbs_entity->absolute_deadline == 0) {
		cbs_entity->absolute_deadline =
			time_now + cbs_entity->relative_deadline;
	} else {
		set_server_task_absolute_deadline(cbs_entity, time_now);
	}

	if (!RB_EMPTY_NODE(&cbs_entity->position_node)) {
		raw_spin_unlock(&cbs_rq->lock);
		return;
	}

	rb_add_cached(&cbs_entity->position_node, &cbs_rq->root,
		      __abs_deadline_comp);

	cbs_rq->cbs_nr_running++;

	raw_spin_unlock(&cbs_rq->lock);
}

void dequeue_cbs_entity(struct sched_cbs_entity *cbs_entity,
			struct cbs_rq *cbs_rq)
{
	raw_spin_lock(&cbs_rq->lock);

	if (RB_EMPTY_NODE(&cbs_entity->position_node)) {
		raw_spin_unlock(&cbs_rq->lock);
		return;
	}

	rb_erase_cached(&cbs_entity->position_node, &cbs_rq->root);

	RB_CLEAR_NODE(&cbs_entity->position_node);

	cbs_rq->cbs_nr_running--;

	raw_spin_unlock(&cbs_rq->lock);
}

struct sched_cbs_entity *pick_next_cbs_entity(struct cbs_rq *cbs_rq)
{
	struct rb_node *left = rb_first_cached(&cbs_rq->root);

	if (!left)
		return NULL;

	return rb_entry_safe(left, struct sched_cbs_entity, position_node);
}

/*
* This function has been introduced in kernel/sched/syscalls.c
* - When sched_setscheduler is called in userspace, this hits eventually __setscheduler_params in kernel/sched/syscalls.c
* - the __setscheduler_params was modified so that if the policy == SCHED_CBS, the function to set the parameters is this one (__setparam_cbs) 
*/
void __setparam_cbs(struct task_struct *p, const struct sched_attr *attr)
{
	struct sched_cbs_entity *cbs_entity = &p->cbs;

	// initialization of static properties
	cbs_entity->max_capacity = attr->sched_runtime;
	cbs_entity->relative_deadline = attr->sched_deadline;
	cbs_entity->declared_period = attr->sched_period;

	// Determination if a task is a soft (server)task is if it was passed a max capacity
	cbs_entity->is_cbs_server = (cbs_entity->max_capacity > 0);

	// initialization of dynamic server properties
	if (cbs_entity->is_cbs_server) {
		cbs_entity->server_bandwidth =
			cbs_entity->max_capacity / cbs_entity->declared_period;

		cbs_entity->remaining_runtime = cbs_entity->max_capacity;

		// The absolute deadline set at zero in the beggining is used to know if this is an actual new task
		cbs_entity->absolute_deadline = 0;

		// Budget timer iniitialization and hook to callback function
		hrtimer_setup(&cbs_entity->budget_control_timer,
			      cbs_budget_timer_callback, CLOCK_MONOTONIC,
			      HRTIMER_MODE_REL);
		cbs_entity->budget_control_timer.function =
			cbs_budget_timer_callback;
	}
}

void pause_timer_account_remaining_budget(struct sched_cbs_entity *cbs_entity)
{
	trace_printk("[CBS][PAUSE_TIMER] deadline=%llu\n",
		     cbs_entity->absolute_deadline);

	ktime_t remaining_time =
		hrtimer_get_remaining(&cbs_entity->budget_control_timer);
	s64 remaining_ns = ktime_to_ns(remaining_time);

	if (remaining_ns < 0) {
		remaining_ns = 0;
	}

	trace_printk("[CBS][REMAINING] runtime=%lld\n", remaining_ns);

	cbs_entity->remaining_runtime = remaining_ns;

	hrtimer_try_to_cancel(&cbs_entity->budget_control_timer);
}

void start_cbs_budget_timer(struct sched_cbs_entity *cbs_entity)
{
	if (!cbs_entity->is_cbs_server || cbs_entity->remaining_runtime <= 0)
		return;

	hrtimer_start(&cbs_entity->budget_control_timer,
		      ns_to_ktime(cbs_entity->remaining_runtime),
		      HRTIMER_MODE_REL);
}

void set_server_task_absolute_deadline(struct sched_cbs_entity *cbs_entity,
				       u64 time_now)
{
	if (cbs_entity->is_cbs_server) {
		trace_printk(
			"[CBS][BEG_SET_SERVER_TASK_ABSOLUTE_DEADLINE] deadline=%llu now=%llu runtime=%lld\n",
			cbs_entity->absolute_deadline, time_now,
			cbs_entity->remaining_runtime);

		u64 threshold_budget =
			(cbs_entity->absolute_deadline - time_now) *
			cbs_entity->max_capacity;

		if (cbs_entity->remaining_runtime *
			    cbs_entity->declared_period >=
		    threshold_budget) {
			cbs_entity->absolute_deadline =
				time_now + cbs_entity->declared_period;
			cbs_entity->remaining_runtime =
				cbs_entity->max_capacity;
		}

		trace_printk(
			"[CBS][END_SET_SERVER_TASK_ABSOLUTE_DEADLINE] new_deadline=%llu runtime=%lld\n",
			cbs_entity->absolute_deadline,
			cbs_entity->remaining_runtime);
	}
}
