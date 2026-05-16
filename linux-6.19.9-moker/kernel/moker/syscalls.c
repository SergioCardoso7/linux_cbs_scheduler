#include <linux/syscalls.h>
#include "trace.h"
#include "../sched/sched.h"
SYSCALL_DEFINE1(moker_tracing, unsigned int, enable)
{
	printk("MOKER: moker_tracing:[%d][%d]\n", (int) enable, current->pid);
	return do_moker_tracing(enable);
}

int do_moker_tracing (unsigned int enable) {
#ifdef CONFIG_MOKER_TRACING
	printk("MOKER: sys_moker_tracing:[%d][%d]\n", (int) enable, current->pid);
	enable_tracing(enable);
#endif
	return 0;
}

SYSCALL_DEFINE1(moker_id, int, id)
{
	printk("MOKER: sys_moker_id:[%d][%d]\n", id, current->pid);
	return do_moker_id(id);
}

int do_moker_id(int id){
	int ret_id = -1;
#if defined(CONFIG_MOKER_SCHED_LIFO_POLICY) || defined(CONFIG_MOKER_SCHED_CBS_POLICY)
	printk("MOKER: do_moker_id:[%d][%d]\n", id, current->pid);
	#ifdef CONFIG_MOKER_SCHED_LIFO_POLICY
	if(lf_policy(current->policy)){
		current->lf.id = id;
		ret_id = current->lf.id;
	}
	return current->lf.id;	
	#endif 

	#ifdef CONFIG_MOKER_SCHED_CBS_POLICY
	if(cbs_policy(current->policy)){
		current->cbs.id = id;
		ret_id = current->cbs.id;
	}
	return current->cbs.id;	
	#endif
#endif
	return ret_id;
}
