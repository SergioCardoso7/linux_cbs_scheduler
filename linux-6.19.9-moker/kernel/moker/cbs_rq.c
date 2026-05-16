#include "cbs_rq.h"

void init_cbs_rq(struct cbs_rq *rq){
    raw_spin_lock_init(&rq->lock);
    rq->root = RB_ROOT_CACHED;
    rq->task = NULL;
    rq->dl_nr_running = 0;
}