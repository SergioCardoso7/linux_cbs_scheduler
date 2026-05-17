#ifndef __CBS_UTILS_
#define __CBS_UTILS_

#include <linux/rbtree.h>
#include "../sched/sched.h"
#include "cbs_rq.h"
#include "cbs_task.h"

static void update_cbs_entity(struct sched_cbs_entity *cbs_entity);
static struct sched_cbs_entity *pick_next_cbs_entity(struct cbs_rq *cbs_rq);
static inline struct task_struct *cbs_task_of(struct sched_cbs_entity *cbs_entity);
static inline bool __abs_deadline_comp(struct rb_node *a, const struct rb_node *b);
static inline bool cbs_time_before(u64 a, u64 b);

#define __node_2_cbs_entity(node) \
	rb_entry_safe((node), struct sched_cbs_entity, position_node)
	
static void enqueue_cbs_entity(struct sched_cbs_entity *cbs_entity,
					struct cbs_rq *cbs_rq, int flags)
{
	//TODO: ADD A CALL TO RESCHED CURR
	update_cbs_entity(
		cbs_entity); // absolute deadline and remaining runtime set here

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


static struct task_struct *__pick_task_cbs(struct cbs_rq *cbs_rq){
	
	struct sched_cbs_entity* next_entity = pick_next_cbs_entity(cbs_rq);
	if(!next_entity)
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

static inline bool __abs_deadline_comp(struct rb_node *a,
				       const struct rb_node *b)
{
	return cbs_time_before(__node_2_cbs_entity(a)->absolute_deadline,
			       __node_2_cbs_entity(b)->absolute_deadline);
}

static inline bool cbs_time_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

static inline struct task_struct *
cbs_task_of(struct sched_cbs_entity *cbs_entity)
{
	if(!cbs_entity)
		return NULL;

	return container_of(cbs_entity, struct task_struct, cbs);
}

#endif