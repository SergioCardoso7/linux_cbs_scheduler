# MOKER framework makefile

obj-$(CONFIG_MOKER_TRACING) += trace.o

obj-y += syscalls.o

obj-$(CONFIG_MOKER_SCHED_LIFO_POLICY) += sched.o lf_rq.o

obj-$(CONFIG_MOKER_SCHED_CBS_POLICY) += sched_cbs.o cbs_rq.o cbs_utils.o
