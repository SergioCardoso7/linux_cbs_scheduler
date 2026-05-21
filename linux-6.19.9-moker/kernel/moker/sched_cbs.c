#include "../sched/sched.h"
#include "cbs_utils.h"

/* 
* CBS scheduling class
* Implements SCHED_CBS
*/

static void wakeup_preempt_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	trace_printk(
		"[CBS][WAKEUP_PREEMPT] pid=%d rq->nr_running=%u cbs_nr_running=%u curr_policy = (policy=%d)\n",
		p->pid, rq->nr_running, rq->cbs.cbs_nr_running,
		rq->curr->policy);

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
			"[CBS][WAKEUP_PREEMPT] Comparing deadlines: curr->dl=%llu incoming->dl=%llu\n",
			rq->curr->cbs.absolute_deadline,
			p->cbs.absolute_deadline);

		if (rq->curr->cbs.absolute_deadline > p->cbs.absolute_deadline)
			resched_curr(rq);

		raw_spin_unlock(&rq->cbs.lock);
		break;
	}
}

static void enqueue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	trace_printk(
		"[CBS][ENQUEUE_BEFORE] pid=%d abs_deadline=%llu rem_runtime=%lld on_rq=%d rq->nr_running=%u cbs_nr_running=%u flags=%x\n",
		p->pid, p->cbs.absolute_deadline, p->cbs.remaining_runtime,
		p->on_rq, rq->nr_running, rq->cbs.cbs_nr_running, flags);

	enqueue_cbs_entity(&p->cbs, &rq->cbs, flags);
	add_nr_running(rq, 1);
	trace_enqueue(p);

	trace_printk(
		"[CBS][ENQUEUE_AFTER] pid=%d abs_deadline=%llu rem_runtime=%lld on_rq=%d rq->nr_running=%u cbs_nr_running=%u flags=%x\n",
		p->pid, p->cbs.absolute_deadline,
		p->cbs.remaining_runtime, p->on_rq, rq->nr_running,
		rq->cbs.cbs_nr_running, flags);

	// need for a call to resched (if the newly enqueued task has an earlier deadline)
	// wakeup_preempt_cbs(rq, p, 0);
}

static bool dequeue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	trace_printk(
		"[CBS][DEQUEUE_BEFORE] pid=%d abs_deadline=%llu rem_runtime=%lld on_rq=%d rq->nr_running=%u cbs_nr_running=%u flags=%x\n",
		p->pid, p->cbs.absolute_deadline, p->cbs.remaining_runtime,
		p->on_rq, rq->nr_running, rq->cbs.cbs_nr_running, flags);

	dequeue_cbs_entity(&p->cbs, &rq->cbs);
	sub_nr_running(rq, 1);
	trace_dequeue(p);

	trace_printk(
		"[CBS][DEQUEUE_AFTER] pid=%d abs_deadline=%llu rem_runtime=%lld on_rq=%d rq->nr_running=%u cbs_nr_running=%u flags=%x\n",
		p->pid, p->cbs.absolute_deadline, p->cbs.remaining_runtime,
		p->on_rq, rq->nr_running, rq->cbs.cbs_nr_running, flags);

	return true;
}
/*
* Preempt the current task with a newly woken task if needed:
*/

static struct task_struct *pick_task_cbs(struct rq *rq, struct rq_flags *rf)
{
	struct sched_cbs_entity *next_entity = pick_next_cbs_entity(&rq->cbs);
	if (!next_entity) {
		trace_printk("[CBS][PICK] no CBS task\n");
		return NULL;
	}
	struct task_struct *next = cbs_task_of(next_entity);
	trace_printk(
		"[CBS][PICK] picked pid=%d comm=%s abs_deadline=%llu rem_runtime=%lld\n",
		next->pid, next->comm, next->cbs.absolute_deadline,
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
	trace_printk(
		"[CBS][PUT_PREV] pid=%d next=%s on_rq=%d runtime_before=%lld\n",
		p->pid, next ? next->comm : "NULL" ,p->on_rq, p->cbs.remaining_runtime);

	struct sched_cbs_entity *cbs_entity = &p->cbs;

	// Either dequeing or simple preemption, we need to cancel the timer and account the remaining runtime (budget)
	if (cbs_entity->is_cbs_server) {
		pause_timer_account_remaining_budget(cbs_entity);

		trace_printk(
			"[CBS][ACCOUNT_AFTER_PAUSE] pid=%d remaining_runtime=%lld\n",
			p->pid, p->cbs.remaining_runtime);
	}
	/*
	// This means that this was a simple preemption (task still in runqueue)
	// if it was a preemption caused by a dequeue (sleeping, blocked, exited),
	// the dequeue_task function would be called first and the task would no longer be in the runqueue
	if (p->on_rq && cbs_entity->is_cbs_server) {
		// Removal and re-introduction into rbtree
		// This is done so it can pass through the enqueue logic again
		// and therefore (if applicable) get it replenished budget and new abs. deadline (and new position in the tree)
		// Later on if it gets called to run it will pass by set_next_task
		// which will restart the budget timer with the updated budget
		trace_printk(
			"[CBS][PUT_PREV_BEFORE_REQUEUE] pid=%d abs_deadline=%llu rem_runtime=%lld\n",
			p->pid, p->cbs.absolute_deadline,
			p->cbs.remaining_runtime);
			
			dequeue_cbs_entity(cbs_entity, &rq->cbs);
			enqueue_cbs_entity(cbs_entity, &rq->cbs, 0);
			
			trace_printk(
				"[CBS][PUT_PREV_AFTER_REQUEUE] pid=%d abs_deadline=%llu rem_runtime=%lld\n",
				p->pid, p->cbs.absolute_deadline,
				p->cbs.remaining_runtime);
			}
			*/
}

/*
* This is a good place to put the budget timer (re)start
* Account for the new execution time start
*/
static void set_next_task_cbs(struct rq *rq, struct task_struct *p, bool first)
{
	struct sched_cbs_entity *cbs_entity = &p->cbs;

	trace_printk(
		"[CBS][SET_NEXT_BEFORE_TIMER_START] pid=%d abs_deadline=%llu rem_runtime=%lld first=%d\n",
		p->pid, p->cbs.absolute_deadline,
		p->cbs.remaining_runtime, first);

	start_cbs_budget_timer(cbs_entity);

	// change this print because we are not printing the actual remaining_runtime because we did not fetch it
	trace_printk("[CBS][SET_NEXT_AFTER_TIMER_START] pid=%d rem_runtime=%lld\n",
		     p->pid, p->cbs.remaining_runtime);
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
static void update_curr_cbs(struct rq *rq)
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
