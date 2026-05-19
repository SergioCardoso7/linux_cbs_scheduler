#ifndef __CBS_TASK_H_
#define __CBS_TASK_H_

#include <linux/types.h>
#include <linux/rbtree_types.h>
#include <linux/hrtimer_types.h>

struct sched_cbs_entity {

//TODO: check if we have everyhting we need in this struct
    struct rb_node  position_node; // position in the RB-tree organized by deadline
    
    struct hrtimer  budget_control_timer;
    
    u64             max_capacity; // This is the server's max capacity - Q
    u64             declared_period; 
    u64             relative_deadline;
    
    u64             server_bandwidth;
    
    s64             remaining_runtime; // this is the server's remaining budget 
    u64             absolute_deadline;
    unsigned int    id;
    bool            is_cbs_server;
    
};

#endif