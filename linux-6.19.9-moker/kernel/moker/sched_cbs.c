#include "../sched/sched.h"
#include "cbs_utils.h"

/* 
* CBS scheduling class
* Implements SCHED_CBS
*/

static void enqueue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
    raw_spin_lock(&rq->cbs.lock);

	enqueue_cbs_entity(&p->cbs, &rq->cbs, flags);
    // need for a call to resched
    raw_spin_unlock(&rq->cbs.lock);    
}

static bool dequeue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
    raw_spin_lock(&rq->cbs.lock);

    dequeue_cbs_entity(&p->cbs, &rq->cbs);

    raw_spin_unlock(&rq->cbs.lock);
	return true;
}
/*
* Preempt the current task with a newly woken task if needed:
*/
static void wakeup_preempt_cbs(struct rq *rq, struct task_struct *p, int flags)
{
    if (rq->curr && rq->curr->sched_class == &cbs_sched_class){
        
        if (__abs_deadline_comp(&p->cbs.position_node, &rq->curr->cbs.position_node)) {
            resched_curr(rq);
        }
    } else {
        // If current task is non-CBS, preemption is immediate
        resched_curr(rq);
    }
}

static struct task_struct *pick_task_cbs(struct rq *rq, struct rq_flags *rf)
{
	return __pick_task_cbs(&rq->cbs);
}


static void put_prev_task_cbs(struct rq *rq, struct task_struct *p,
			      struct task_struct *next)
{
    struct sched_cbs_entity *cbs_entity = &p->cbs;
    struct cbs_rq *cbs_rq = &rq->cbs;

    // TODO: SUPER IMPORTANT
    /*
        this function is called whenever a task stops running,
        either because of dequeing (task sleeps, blocks, exits, etc) or preemption

        Therefore this is the perfect place to "pause" the budget control hrtimer and do accounting
    */ 


}
static void set_next_task_cbs(struct rq *rq, struct task_struct *p, bool first)
{

    // TODO: SUPER IMPORTANT
    /*
        This is a good place to put the budget timer restart
        Account for the new execution time start
    */

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