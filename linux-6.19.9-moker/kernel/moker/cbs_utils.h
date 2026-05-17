#ifndef __CBS_UTILS_
#define __CBS_UTILS_

#include <linux/rbtree.h>
#include "../sched/sched.h"
#include "cbs_rq.h"
#include "cbs_task.h"

void enqueue_cbs_entity(struct sched_cbs_entity *cbs_entity,
			struct cbs_rq *cbs_rq, int flags);
void dequeue_cbs_entity(struct sched_cbs_entity *cbs_entity,
			struct cbs_rq *cbs_rq);
struct task_struct *__pick_task_cbs(struct cbs_rq *cbs_rq);
struct sched_cbs_entity *pick_next_cbs_entity(struct cbs_rq *cbs_rq);
void set_cbs_entity_dyn_args(struct sched_cbs_entity *cbs_entity);
void __setparam_cbs(struct task_struct *p, const struct sched_attr *attr);

#define __node_2_cbs_entity(node) \
	rb_entry_safe((node), struct sched_cbs_entity, position_node)

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

static inline bool __abs_deadline_comp(struct rb_node *a,
				       const struct rb_node *b)
{
	return cbs_time_before(__node_2_cbs_entity(a)->absolute_deadline,
			       __node_2_cbs_entity(b)->absolute_deadline);
}

#endif