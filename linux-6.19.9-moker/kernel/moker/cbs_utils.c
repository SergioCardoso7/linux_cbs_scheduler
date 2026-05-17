#ifndef __CBS_UTILS_
#define __CBS_UTILS_

#include "../sched/sched.h"
#include "cbs_task.h"
#include "cbs_utils.h"

static void enqueue_cbs_entity(struct sched_cbs_entity *cbs_entity,
			       struct cbs_rq *cbs_rq, int flags)
{
	/* 
		FIRST ACTIVATION OF A TASK
		When a new task is created __setparam_cbs sets the absolute deadline value to zero.
		Need to give it an actual absolute deadline before enqueing
	*/

	if (cbs_entity->absolute_deadline == 0) {
		u64 time_now = ktime_get_ns();
		cbs_entity->absolute_deadline =
			time_now + cbs_entity->relative_deadline;
	}

	rb_add_cached(&cbs_entity->position_node, &cbs_rq->root,
		      __abs_deadline_comp);

	cbs_rq->dl_nr_running++;
}

static void dequeue_cbs_entity(struct sched_cbs_entity *cbs_entity,
			       struct cbs_rq *cbs_rq)
{
	if (RB_EMPTY_NODE(&cbs_entity->position_node))
		return;

	rb_erase_cached(&cbs_entity->position_node, &cbs_rq->root);

	RB_CLEAR_NODE(&cbs_entity->position_node);

	cbs_rq->dl_nr_running--;
}

static void update_cbs_entity(struct sched_cbs_entity *cbs_entity)
{
}

static struct task_struct *__pick_task_cbs(struct cbs_rq *cbs_rq)
{
	struct sched_cbs_entity *next_entity = pick_next_cbs_entity(cbs_rq);
	if (!next_entity)
		return NULL;

	return cbs_task_of(next_entity);
}

static struct sched_cbs_entity *pick_next_cbs_entity(struct cbs_rq *cbs_rq)
{
	struct rb_node *left = rb_first_cached(&cbs_rq->root);

	if (!left)
		return NULL;

	return rb_entry_safe(left, struct sched_cbs_entity, position_node);
}

static void set_cbs_entity_dyn_args(struct sched_cbs_entity *cbs_entity)
{
	if (cbs_entity->is_cbs_server) {
		u64 declared_period = cbs_entity->declared_period;
		u64 declared_runtime = cbs_entity->declared_runtime;
		s64 remaining_runtime = cbs_entity->remaining_runtime;

		if (remaining_runtime <= 0) {
			cbs_entity->absolute_deadline =
				cbs_entity->absolute_deadline + declared_period;
			cbs_entity->remaining_runtime = declared_runtime;
		}

		// u64 time_to_compare =
	}
}


static inline bool cbs_time_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

static inline struct task_struct *
cbs_task_of(struct sched_cbs_entity *cbs_entity)
{
	if (!cbs_entity)
	return NULL;
	
	return container_of(cbs_entity, struct task_struct, cbs);
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
	cbs_entity->declared_runtime = attr->sched_runtime;
	cbs_entity->relative_deadline = attr->sched_deadline;
	cbs_entity->declared_period = attr->sched_period;
	
	cbs_entity->is_cbs_server = (cbs_entity->declared_runtime > 0);
	
	// initialization of dynamic properties
	cbs_entity->remaining_runtime = cbs_entity->declared_runtime;
	cbs_entity->absolute_deadline = 0;
}

#endif