#include "../sched/sched.h"

/* 
* CBS scheduling class
* Implements SCHED_CBS
*/

static void enqueue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
}

static bool dequeue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
return true;
}
/*
* Preempt the current task with a newly woken task if needed:
*/
static void wakeup_preempt_cbs(struct rq *rq, struct task_struct *p, int flags)
{
}
static struct task_struct *pick_task_cbs(struct rq *rq, struct rq_flags *rf)
{
return NULL;
}
static void put_prev_task_cbs(struct rq *rq, struct task_struct *p, struct task_struct *next)
{
}
static void set_next_task_cbs(struct rq *rq, struct task_struct *p, bool first)
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