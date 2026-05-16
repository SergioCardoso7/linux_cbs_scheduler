#ifndef __CBS_RQ_H_
#define __CBS_RQ_H_

#include <linux/sched.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>

struct cbs_rq {
    struct rb_root_cached root;              // RB-tree (cached version) ordered by absolute deadline
    struct task_struct *task;                // highest priority task (I think this could be removed)
    raw_spinlock_t lock;
    unsigned int          dl_nr_running;     // runnable DL task count
};


void init_cbs_rq(struct cbs_rq *rq);
#endif