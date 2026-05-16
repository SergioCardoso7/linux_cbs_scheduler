#ifndef __LF_RQ_H_
#define __LF_RQ_H_

#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>

struct lf_rq {
	struct list_head tasks;
	struct task_struct *task;
	raw_spinlock_t lock;
	unsigned nr_running;
};

void init_lf_rq(struct lf_rq *rq);
#endif
