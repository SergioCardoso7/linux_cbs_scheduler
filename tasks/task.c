#include "defs.h"

void do_work(unsigned long long exec)
{
	unsigned long long i, ten_ns;
	ten_ns = (unsigned long long)((double)exec / 3.89);
	for (i = 0; i < ten_ns; i++)
	{
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		// 32

		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);

		// 64
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
		asm volatile("nop" ::);
	}
}

int sched_setattr(pid_t pid,
				  const struct sched_attr *attr,
				  unsigned int flags)
{
	return syscall(__NR_sched_setattr, pid, attr, flags);
}

int main(int argc, char **argv)
{
	// struct sched_param param;
	unsigned long long C, T, O, D, flag, time0, release;
	unsigned int task_id, njobs, i = 0;

	struct timespec r;

	task_id = atoi(argv[1]);
	C = (unsigned long long)atoll(argv[2]);
	T = (unsigned long long)atoll(argv[3]);
	O = (unsigned long long)atoll(argv[4]) + OFFSET;
	time0 = (unsigned long long)atoll(argv[5]);
	D = (unsigned long long)atoll(argv[6]);
	njobs = atoi(argv[7]);
	flag = (unsigned long long)atoll(argv[8]);

	// printf("Task(%d,%d): after SCHED_CBS\n", task_id, getpid());

	printf("Task(%d,%d): set ID\n", task_id, getpid());
	if ((syscall(SYS_MOKER_ID, task_id)) < 0)
	{
		perror("ERROR: Setting moker id failed");
		exit(-1);
	}
	printf("Task(%d,%d): before SCHED_CBS\n", task_id, getpid());

	release = time0 + O;

	for (i = 0; i < njobs; i++)
	{
		r.tv_sec = release / NSEC_PER_SEC;
		r.tv_nsec = release % NSEC_PER_SEC;

		// printf("Task(%d,%d,%d): sleeping until release %llu\n", task_id, getpid(), i, release);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &r, NULL);
		// printf("Task(%d,%d,%d): ready for execution\n", task_id, getpid(), i);

		if (i == 0)
		{
			struct sched_attr attr = {
				.size = sizeof(struct sched_attr),
				.sched_policy = SCHED_CBS,
				.sched_flags = flag,
				.sched_runtime = C,	 // max_capacity
				.sched_deadline = D, // relative_deadline
				.sched_period = T,	 // declared_period
			};

			if (sched_setattr(0, &attr, 0) < 0)
			{
				perror("sched_setattr");
				exit(-1);
			}
		}
		do_work(C);

		// computes the next release
		release += T;
	}
	exit(task_id);
	return 0;
}
