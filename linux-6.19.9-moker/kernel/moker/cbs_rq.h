#ifndef __CBS_RQ_H_
#define __CBS_RQ_H_

#include <linux/sched.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>

struct cbs_rq {
    struct rb_root_cached root;              // RB-tree (cached version) ordered by absolute deadline
    raw_spinlock_t lock;
    unsigned int          cbs_nr_running;     // runnable DL task count
};

void init_cbs_rq(struct cbs_rq *rq);

#endif