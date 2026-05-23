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

	if (!cbs_entity) {
		return HRTIMER_NORESTART;
	}

	struct task_struct *task_struct = cbs_task_of(cbs_entity);
	struct rq *rq;
	struct rq_flags rf;

	trace_printk(
		"[CBS][BUDGET_EXPIRE] pid=%d abs_deadline=%llu",
		task_struct->pid, cbs_entity->absolute_deadline);

	rq = task_rq_lock(task_struct, &rf);

	// Pushing the deadline of the server just like in the paper officers Baltarejo and Maia
	// This will avoid being picked again in pick_task_cbs
	remove_cbs_entity_from_tree(cbs_entity, &rq->cbs);

	cbs_entity->absolute_deadline += cbs_entity->declared_period;
	cbs_entity->remaining_runtime = cbs_entity->max_capacity;

	ktime_t future_expiry = ns_to_ktime(cbs_entity->max_capacity);
	hrtimer_set_expires(timer, future_expiry);

	push_cbs_entity_to_tree(cbs_entity, &rq->cbs);

	trace_printk("[CBS][REPLENISH] pid=%d new_abs_deadline=%llu rem_runtime=%llu\n",
		     task_struct->pid, cbs_entity->absolute_deadline, cbs_entity->remaining_runtime);

	// This call will force the kernel to call __schedule() which in turns calls put_prev_task and set_next_task
	// In put_prev_task we will cancel the timer and account the remaining budget
	resched_curr(rq);

	task_rq_unlock(rq, task_struct, &rf);

	return HRTIMER_NORESTART;
}

void push_cbs_entity_to_tree(struct sched_cbs_entity *cbs_entity,
			     struct cbs_rq *cbs_rq)
{
	raw_spin_lock(&cbs_rq->lock);

	if (!RB_EMPTY_NODE(&cbs_entity->position_node)) {
		raw_spin_unlock(&cbs_rq->lock);
		return;
	}

	rb_add_cached(&cbs_entity->position_node, &cbs_rq->root,
		      __abs_deadline_comp);

	cbs_rq->cbs_nr_running++;

	raw_spin_unlock(&cbs_rq->lock);
}

void remove_cbs_entity_from_tree(struct sched_cbs_entity *cbs_entity,
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

	RB_CLEAR_NODE(&cbs_entity->position_node);

	// initialization of static properties
	cbs_entity->max_capacity = attr->sched_runtime;
	cbs_entity->relative_deadline = attr->sched_deadline;
	cbs_entity->declared_period = attr->sched_period;

	// Determinating if a task is a soft(server) task or a hard task
	cbs_entity->is_cbs_server = attr->sched_flags == 1;

	// initialization of dynamic server properties
	if (cbs_entity->is_cbs_server) {
		cbs_entity->remaining_runtime = cbs_entity->max_capacity;

		// The absolute deadline set at zero in the beginning and is used to know if this is an actual new task
		cbs_entity->absolute_deadline = 0;

		cbs_entity->server_bandwidth =
			cbs_entity->max_capacity / cbs_entity->declared_period;

		// Budget timer initialization and hook to callback function
		hrtimer_setup(&cbs_entity->budget_control_timer,
			      cbs_budget_timer_callback, CLOCK_MONOTONIC,
			      HRTIMER_MODE_REL);
		cbs_entity->budget_control_timer.function =
			cbs_budget_timer_callback;
	}
}

void pause_timer(struct sched_cbs_entity *cbs_entity)
{
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

void set_server_task_abs_deadline_and_budget(
	struct sched_cbs_entity *cbs_entity, u64 time_now)
{
	if (cbs_entity->is_cbs_server) {
		trace_printk(
			"[CBS][BEFORE_UPDATING_PROPERTIES] abs_deadline=%llu time_now=%llu rem_runtime=%lld\n",
			cbs_entity->absolute_deadline, time_now,
			cbs_entity->remaining_runtime);

		// This is done by the timer callback function, in here just in case for safety
		if (cbs_entity->remaining_runtime <= 0) {
			cbs_entity->absolute_deadline +=
				cbs_entity->declared_period;
			cbs_entity->remaining_runtime =
				cbs_entity->max_capacity;
		}

		// To avoid overflows since we are multiplying large numbers (in ns) we use 128 bits sized variables to do the comparison
		__uint128_t threshold_budget =
			(__uint128_t)(cbs_entity->absolute_deadline -
				      time_now) *
			cbs_entity->max_capacity;

		__uint128_t compare_instant =
			(__uint128_t)cbs_entity->remaining_runtime *
			cbs_entity->declared_period;

		if (compare_instant >= threshold_budget) {
			cbs_entity->absolute_deadline =
				time_now + cbs_entity->declared_period;
			cbs_entity->remaining_runtime =
				cbs_entity->max_capacity;
		}

		trace_printk(
			"[CBS][AFTER_UPDATING_PROPERTIES] new_deadline=%llu time_now=%llu rem_runtime=%lld\n",
			cbs_entity->absolute_deadline, time_now,
			cbs_entity->remaining_runtime);
	}
}

s64 get_remaining_runtime_from_timer(struct sched_cbs_entity *cbs_entity)
{
	ktime_t remaining_time =
		hrtimer_get_remaining(&cbs_entity->budget_control_timer);

	s64 remaining_ns = ktime_to_ns(remaining_time);

	if (remaining_ns < 0)
		remaining_ns = 0;

	return remaining_ns;
}

void trace_enqueue(struct task_struct *p)
{
#ifdef CONFIG_MOKER_TRACING
	moker_trace(ENQUEUE_RQ, p, -1);
#endif
}

void trace_dequeue(struct task_struct *p)
{
#ifdef CONFIG_MOKER_TRACING
	moker_trace(DEQUEUE_RQ, p, -1);
#endif
}
