#ifndef __TRACE_H_
#define __TRACE_H_
#define TRACE_ENTRY_NAME "moker_trace"
#define TRACE_BUFFER_SIZE 1000
#define TRACE_STRING_BUFFER_SIZE 200
#define TRACE_TASK_COMM_LEN 16

enum evt {
	MISC_EVT = 0,
	SCHED_TICK,
	SWITCH_AWAY,
	SWITCH_TO,
	ENQUEUE_RQ,
	DEQUEUE_RQ
};
struct trace_evt {
	enum evt event;
	unsigned long long time;
	int number;
	pid_t pid;
	int state;
	int prio;
	int policy;
	char comm[TRACE_TASK_COMM_LEN];
	int id;
};

struct trace_evt_buffer {
	struct trace_evt events[TRACE_BUFFER_SIZE];
	int write_item;
	int read_item;
	spinlock_t lock;
};

ssize_t trace_read(struct file *filp, char __user *buf, size_t count,
		   loff_t *f_pos);
void moker_trace(enum evt event, struct task_struct *p, int number);
void enable_tracing (unsigned int e);
#endif
