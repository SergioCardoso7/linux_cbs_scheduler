#include "../sched/sched.h"

/*
LIFO scheduling class
Implements SCHED_LIFO
*/
static void enqueue_task_lf(struct rq *rq, struct task_struct *p, int flags)
{
    raw_spin_lock(&rq->lf.lock);
    list_add(&p->lf.node,&rq->lf.tasks);
    rq->lf.task = p;
    rq->lf.nr_running++;
    add_nr_running(rq, 1);
    raw_spin_unlock(&rq->lf.lock);
#ifdef CONFIG_MOKER_TRACING
	moker_trace(ENQUEUE_RQ, p, -1);
#endif		
}

static bool dequeue_task_lf(struct rq *rq, struct task_struct *p, int flags)
{
    struct sched_lf_entity *t = NULL;
    raw_spin_lock(&rq->lf.lock);
    list_del(&p->lf.node);
    if(list_empty(&rq->lf.tasks)){
        rq->lf.task = NULL;
    }else{
        t = list_first_entry(&rq->lf.tasks,struct sched_lf_entity, node);
        rq->lf.task = container_of(t,struct task_struct, lf);
    }
    rq->lf.nr_running--;
    sub_nr_running(rq, 1);
    raw_spin_unlock(&rq->lf.lock);
#ifdef CONFIG_MOKER_TRACE
	moker_trace(DEQUEUE_RQ, p, -1);
#endif
    return true;
}
/*
* Preempt the current task with a newly woken task if needed:
*/
static void wakeup_preempt_lf(struct rq *rq, struct task_struct *p, int flags)
{
    switch(rq->donor->policy){
        case SCHED_DEADLINE:
        break;
        case SCHED_FIFO:
        case SCHED_RR:
        case SCHED_NORMAL:
        case SCHED_BATCH:
        case SCHED_IDLE:
        //case SCHED_RESET_ON_FORK:
        case SCHED_LIFO:
        resched_curr(rq);
        break;
    }
}
static struct task_struct *pick_task_lf(struct rq *rq, struct rq_flags *rf)
{
    struct task_struct * p = NULL;
    raw_spin_lock(&rq->lf.lock);
    p = rq->lf.task;
    raw_spin_unlock(&rq->lf.lock);
    return p;
}
static void put_prev_task_lf(struct rq *rq, struct task_struct *p, struct task_struct *next)
{
}
static void set_next_task_lf(struct rq *rq, struct task_struct *p, bool first)
{
}
static int select_task_rq_lf(struct task_struct *p, int cpu, int flags)
{
return cpu;
}
static void task_tick_lf(struct rq *rq, struct task_struct *p, int queued)
{
}
static void prio_changed_lf(struct rq *rq, struct task_struct *p, u64 oldprio)
{
}
static void switched_to_lf(struct rq *rq, struct task_struct *p)
{
}
static void update_curr_lf(struct rq *rq)
{
}

DEFINE_SCHED_CLASS(lf) = {
.queue_mask = 8,
.enqueue_task = enqueue_task_lf,
.dequeue_task = dequeue_task_lf,
.wakeup_preempt = wakeup_preempt_lf,
.pick_task = pick_task_lf,
.put_prev_task = put_prev_task_lf,
.set_next_task = set_next_task_lf,
.select_task_rq = select_task_rq_lf,
.set_cpus_allowed = set_cpus_allowed_common,
.task_tick = task_tick_lf,
.prio_changed = prio_changed_lf,
.switched_to = switched_to_lf,
.update_curr = update_curr_lf,
};
