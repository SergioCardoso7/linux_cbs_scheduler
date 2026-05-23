#include "../sched/sched.h"
#include "cbs_utils.h"

/* 
* CBS scheduling class
* Implements SCHED_CBS
*/

static void enqueue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_cbs_entity *cbs_entity = &p->cbs;

	u64 time_now = ktime_get_ns();

	cbs_entity->activation_time = time_now;

	if (cbs_entity->is_cbs_server) {
		set_server_task_abs_deadline_and_budget(cbs_entity, time_now);
	} else {
		cbs_entity->absolute_deadline =
			time_now + cbs_entity->relative_deadline;
	}

	push_cbs_entity_to_tree(&p->cbs, &rq->cbs);
	add_nr_running(rq, 1);

	trace_enqueue(p);

	trace_printk(
		"[ENQ_A] pid=%d cbs_server=%s activ_time=%llu abs_dl=%llu budget=%lld on_rq=%d rq_nr_running=%u cbs_nr_running=%u flags=%x\n",
		p->pid, cbs_entity->is_cbs_server ? "yes" : "no",
		cbs_entity->activation_time, p->cbs.absolute_deadline,
		p->cbs.remaining_runtime, p->on_rq, rq->nr_running,
		rq->cbs.cbs_nr_running, flags);
}

static bool dequeue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	remove_cbs_entity_from_tree(&p->cbs, &rq->cbs);
	sub_nr_running(rq, 1);
	trace_dequeue(p);

	trace_printk(
		"[DEQ_A] pid=%d cbs_server=%s abs_dl=%llu budget=%lld on_rq=%d rq_nr_running=%u cbs_nr_running=%u flags=%x\n",
		p->pid, p->cbs.is_cbs_server ? "yes" : "no",
		p->cbs.absolute_deadline, p->cbs.remaining_runtime, p->on_rq,
		rq->nr_running, rq->cbs.cbs_nr_running, flags);

	return true;
}

static struct task_struct *pick_task_cbs(struct rq *rq, struct rq_flags *rf)
{
	struct sched_cbs_entity *next_entity = pick_next_cbs_entity(&rq->cbs);
	if (!next_entity) {
		trace_printk("[PICK] no SCHED_CBS task\n");
		return NULL;
	}
	struct task_struct *next = cbs_task_of(next_entity);
	trace_printk(
		"[PICK] picked pid=%d abs_deadline=%llu rem_runtime=%lld\n",
		next->pid, next->cbs.absolute_deadline,
		next->cbs.remaining_runtime);

	return next;
}

/*
this function is called whenever a task leaves execution context,
either because of dequeing (task sleeps, blocks, exits, etc) or simple preemption

Therefore this is the perfect place to "pause" the budget control hrtimer and do accounting
*/

static void put_prev_task_cbs(struct rq *rq, struct task_struct *p,
			      struct task_struct *next)
{
	struct sched_cbs_entity *cbs_entity = &p->cbs;

	if (cbs_entity->is_cbs_server) {
		pause_timer(cbs_entity);

		if (p->on_rq) {
			remove_cbs_entity_from_tree(cbs_entity, &rq->cbs);

			cbs_entity->remaining_runtime =
				get_remaining_runtime_from_timer(cbs_entity);

			set_server_task_abs_deadline_and_budget(
				cbs_entity, cbs_entity->activation_time);

			push_cbs_entity_to_tree(cbs_entity, &rq->cbs);
		}
	}
	trace_printk(
		"[PUT_PREV] prev_pid=%d next_pid=%d on_rq=%d prev_rem_budget=%lld\n",
		p->pid, next->pid, p->on_rq, p->cbs.remaining_runtime);
}

/*
* This is a good place to put the budget timer (re)start
* Account for the new execution time start
*/
static void set_next_task_cbs(struct rq *rq, struct task_struct *p, bool first)
{
	struct sched_cbs_entity *cbs_entity = &p->cbs;

	start_cbs_budget_timer(cbs_entity);

	trace_printk(
		"[SET_NEXT] pid=%d abs_deadline = %llurem_runtime=%lld first=%d\n",
		p->pid, p->cbs.absolute_deadline, p->cbs.remaining_runtime,
		first);
}

static void wakeup_preempt_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	switch (rq->curr->policy) {
	case SCHED_DEADLINE:
		break;

	case SCHED_FIFO:
	case SCHED_RR:
	case SCHED_NORMAL:
	case SCHED_BATCH:
	case SCHED_IDLE:
		trace_printk(
			"[CBS][WAKEUP_PREEMPT] Triggering resched_curr for non-CBS task of lower rank, policy: %d\n",
			rq->curr->policy);

		resched_curr(rq);
		break;
	case SCHED_CBS:
		// this lock and unlock should be reviewed
		raw_spin_lock(&rq->cbs.lock);

		trace_printk(
			"[WAKEUP_PREEMPT] Comparing deadlines: (pid=%d)curr->dl=%llu (pid=%d)incoming->dl=%llu\n",
			rq->curr->pid, rq->curr->cbs.absolute_deadline, p->pid,
			p->cbs.absolute_deadline);

		if (rq->curr->cbs.absolute_deadline >
		    p->cbs.absolute_deadline) {
			trace_printk("[WAKEUP_PREEMPT] Triggering resched\n");
			resched_curr(rq);
		}

		raw_spin_unlock(&rq->cbs.lock);
		break;
	}
}

static void update_curr_cbs(struct rq *rq)
{
}
static int select_task_rq_cbs(struct task_struct *p, int cpu, int flags)
{
	return cpu;
}
static void task_tick_cbs(struct rq *rq, struct task_struct *p, int queued)
{
}
static void prio_changed_cbs(struct rq *rq, struct task_struct *p, u64 oldprio)
{
}
static void switched_to_cbs(struct rq *rq, struct task_struct *p)
{
}

DEFINE_SCHED_CLASS(cbs) = {
	.queue_mask = 8,
	.enqueue_task = enqueue_task_cbs,
	.dequeue_task = dequeue_task_cbs,
	.wakeup_preempt = wakeup_preempt_cbs,
	.pick_task = pick_task_cbs,
	.put_prev_task = put_prev_task_cbs,
	.set_next_task = set_next_task_cbs,
	.select_task_rq = select_task_rq_cbs,
	.set_cpus_allowed = set_cpus_allowed_common,
	.task_tick = task_tick_cbs,
	.prio_changed = prio_changed_cbs,
	.switched_to = switched_to_cbs,
	.update_curr = update_curr_cbs,
};