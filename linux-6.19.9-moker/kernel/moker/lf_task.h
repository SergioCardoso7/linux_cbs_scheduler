#ifndef __LF_TASK_H_
#define __LF_TASK_H_

#include <linux/list.h>

struct sched_lf_entity {
	int id;
	struct list_head node;
};

#endif
