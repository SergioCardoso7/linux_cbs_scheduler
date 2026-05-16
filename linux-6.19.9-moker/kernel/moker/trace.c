#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/sched/clock.h>
#include "trace.h"
#include "../sched/sched.h"
unsigned int enabled = 0; // 0 - means disabled, !0 - means enabled

struct trace_evt_buffer trace;

static void increment(int *item)
{
	*item = *item + 1;
	if(*item >= TRACE_BUFFER_SIZE) {
		*item = 0;
	}
}

static int is_empty(int r, int w)
{
	return !(r ^ w); //xor
}

static int is_full(int r, int w)
{
	int write = w;

	increment(&write);

	return write == r;
}

static int dequeue (char *buffer)
{
	unsigned long flags;
	int remaining = TRACE_STRING_BUFFER_SIZE;
	int ret = 0;
	int len = 0;
	char evt[10];

	spin_lock_irqsave(&trace.lock, flags);

	if(!is_empty(trace.read_item,trace.write_item)) {
		switch((int)trace.events[trace.read_item].event) {
			case MISC_EVT:
				strcpy(evt,"MC_EVT");
				break;
			case SCHED_TICK:
				strcpy(evt,"SCH_TK");
				break;
			case SWITCH_AWAY:
				strcpy(evt,"SWT_AY");
				break;
			case SWITCH_TO:
				strcpy(evt,"SWT_TO");
				break;
			case ENQUEUE_RQ:
				strcpy(evt,"ENQ_RQ");
				break;
			case DEQUEUE_RQ:
				strcpy(evt,"DEQ_RQ");
				break; 
			default:
				strcpy(evt,"UK_EVT");
		}
	
		len += snprintf(buffer + len, remaining - len, "%llu,",
				trace.events[trace.read_item].time);
		len += snprintf(buffer + len, remaining - len, "%s,",
				evt);
		len += snprintf(buffer + len, remaining - len, "%d,", trace.events[trace.read_item].id);
		len += snprintf(buffer + len, remaining - len, "*%d*,",
				trace.events[trace.read_item].number);
		len += snprintf(buffer + len, remaining - len, "%d,",
				trace.events[trace.read_item].policy);
		len += snprintf(buffer + len, remaining - len, "%d,",
				trace.events[trace.read_item].prio);
		len += snprintf(buffer + len, remaining - len, "%d,",
				trace.events[trace.read_item].pid);
		len += snprintf(buffer + len, remaining - len, "%d,",
				trace.events[trace.read_item].state);
		len += snprintf(buffer + len, remaining - len, "%s\n",
				trace.events[trace.read_item].comm);
	
		increment(&trace.read_item);
	
		ret = 1;
		
	}

	spin_unlock_irqrestore(&trace.lock, flags);
	return ret;
}

static int enqueue (enum evt event, unsigned long long time,
		    int number, struct task_struct *p)
{
	int id = -1;
#ifdef CONFIG_MOKER_SCHED_LIFO_POLICY
	if(lf_policy(p->policy)){
		id = p->lf.id;
	}
#endif

#ifdef CONFIG_MOKER_SCHED_CBS_POLICY
	if(cbs_policy(p->policy)){
		id = p->cbs.id;
	}
#endif

	unsigned long flags;
	spin_lock_irqsave(&trace.lock, flags);

	if(is_full(trace.read_item, trace.write_item)) {
		increment(&trace.read_item);
	}
		
	trace.events[trace.write_item].number = number;
	trace.events[trace.write_item].event  = event;
	trace.events[trace.write_item].time   = time;
	trace.events[trace.write_item].pid    = p->pid;
	trace.events[trace.write_item].state  = READ_ONCE(p->__state);
	trace.events[trace.write_item].prio   = p->prio;
	trace.events[trace.write_item].policy = p->policy;
	trace.events[trace.write_item].id	  = id;
	strncpy(trace.events[trace.write_item].comm, p->comm, TASK_COMM_LEN - 1);
	trace.events[trace.write_item].comm[TASK_COMM_LEN - 1] = '\0';

	increment(&trace.write_item);

	spin_unlock_irqrestore(&trace.lock, flags);

	return 1;
}

ssize_t trace_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	char buffer[TRACE_STRING_BUFFER_SIZE];
	int ret = 0, len = 0;

	if(!dequeue(buffer)) {
		return 0;
	}

	len = strlen(buffer);

	if(len <= 0) {
		return -EFAULT;
	}

	if(count < len) {
		return -EFAULT;
	}

	ret = copy_to_user(buf, buffer, len);
	if(ret != 0) {
		return -EFAULT;
	}

	return len;
}


static const struct proc_ops trace_ops = {
	.proc_read = trace_read,
};


static int __init proc_trace_init(void)
{
	proc_create(TRACE_ENTRY_NAME, 0444, NULL, &trace_ops);
	printk("MOKER:/proc/%s created\n", TRACE_ENTRY_NAME);
	spin_lock_init(&trace.lock);
	trace.write_item = 0;
	trace.read_item = 0;
	return 0;
}

module_init(proc_trace_init);


void moker_trace(enum evt event, struct task_struct *p, int number)
{
	unsigned long long time;

	if (enabled) {

		time = ktime_to_ns(ktime_get());
	
		enqueue(event, time, number, p);
	}

}

void enable_tracing(unsigned int e){
	enabled = e;
}

