// calibrate.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NSEC_PER_SEC 1000000000ULL
#define TEST_LOOPS 50000000ULL // Run 50 million iterations for a stable sample

void do_work_calibration(unsigned long long iterations)
{
    unsigned long long i;
    for (i = 0; i < iterations; i++) {
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

int main()
{
    struct timespec start, end;
    
    printf("Calibrating busy loop... please wait.\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    do_work_calibration(TEST_LOOPS);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    unsigned long long elapsed_ns = (end.tv_sec - start.tv_sec) * NSEC_PER_SEC + (end.tv_nsec - start.tv_nsec);
    
    double ns_per_iteration = (double)elapsed_ns / TEST_LOOPS;
    
    printf("Ran %llu iterations.\n", TEST_LOOPS);
    printf("Elapsed time: %llu ns (%.3f seconds)\n", elapsed_ns, (double)elapsed_ns / NSEC_PER_SEC);
    printf("Time per iteration: %.6f ns/iteration\n", ns_per_iteration);
    printf("\n--- CALIBRATION FACTOR ---\n");
    printf("To execute for X nanoseconds, loop count should be: X / %.4f\n", ns_per_iteration);
    
    return 0;
}