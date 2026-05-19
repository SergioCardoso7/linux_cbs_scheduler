#include "../sched/sched.h"
#include "cbs_utils.h"

/* 
* CBS scheduling class
* Implements SCHED_CBS
*/

static void wakeup_preempt_cbs(struct rq *rq, struct task_struct *p, int flags)
{
    if (rq->curr && rq->curr->sched_class == &cbs_sched_class) {
        if (__abs_deadline_comp(&p->cbs.position_node,
                    &rq->curr->cbs.position_node)) {
            resched_curr(rq);
        }
    } else {
        // If current task is non-CBS, preemption is immediate
        resched_curr(rq);
    }
}

static void enqueue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	
    enqueue_cbs_entity(&p->cbs, &rq->cbs);
    
	// need for a call to resched (if the newly enqueued task has an earlier deadline)
    wakeup_preempt_cbs(rq, p, 0);
}

static bool dequeue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{

    dequeue_cbs_entity(&p->cbs, &rq->cbs);
	return true;
}
/*
* Preempt the current task with a newly woken task if needed:
*/

static struct task_struct *pick_task_cbs(struct rq *rq, struct rq_flags *rf)
{
	struct sched_cbs_entity *next_entity = pick_next_cbs_entity(&rq->cbs);
	if (!next_entity)
		return NULL;

	return cbs_task_of(next_entity);
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

    // Either dequeing or simple preemption, we need to cancel the timer and account the remaining runtime (budget)
	if (cbs_entity->is_cbs_server) {
		pause_timer_account_remaining_budget(cbs_entity);
	}
    
	// This means that this was a simple preemption (task still in runqueue)
	// if it was a preemption caused by a dequeue (sleeping, blocked, exited), 
    // the dequeue_task function would be called first and the task would no longer be in the runqueue
	if (p->on_rq && cbs_entity->is_cbs_server) {

        // Removal and re-introduction into rbtree
        // This is done so it can pass through the enqueue logic again 
        // and therefore (if applicable) get it replenished budget and new abs. deadline (and new position in the tree)
        // Later on if it gets called to run it will pass by set_next_task 
        // which will restart the budget timer with the updated budget 
        dequeue_cbs_entity(cbs_entity, &rq->cbs);
		enqueue_cbs_entity(cbs_entity, &rq->cbs);
	}
    
}

/*
* This is a good place to put the budget timer (re)start
* Account for the new execution time start
*/
static void set_next_task_cbs(struct rq *rq, struct task_struct *p, bool first)
{

    struct sched_cbs_entity *cbs_entity = &p->cbs;

    start_cbs_budget_timer(cbs_entity);

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



