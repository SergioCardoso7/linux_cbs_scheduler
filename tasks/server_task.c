#include "defs.h"

// CBS server parameters — edit these to match your server's Q and T.
// SERVER_Q: maximum budget per period (nanoseconds)
// SERVER_T: declared period and relative deadline (nanoseconds)
#define SERVER_Q 2000000000LL // 2s
#define SERVER_T 7000000000LL // 7 s

#define MAX_JOBS 200

// ---------- busy-work loop (same calibration constant as task.c) ----------
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

// ---------- sched_setattr wrapper ----------
int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags)
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

// ---------- helpers reused from launch.c ----------
unsigned int get_u32(char *s)
{
    int i, d = 0;
    unsigned int v = 0;
    for (i = 0; s[i] >= '0' && s[i] <= '9'; i++)
    {
        d = s[i] - '0';
        v = (v * 10) + d;
    }
    return v;
}
unsigned long long get_u64(char *s)
{
    int i, d = 0;
    unsigned long long v = 0;
    for (i = 0; s[i] >= '0' && s[i] <= '9'; i++)
    {
        d = s[i] - '0';
        v = (v * 10) + d;
    }
    return v;
}
// ---------- parse one line: "id,C,O,flag," ----------
static int parse_job_line(char *line, struct server_job *job)
{
    char *s = line, *s1 = line;
    int field = 0;
    while (field < 4)
    {
        if (*s == '\0' || *s == '\n')
            break;
        if (*s == ',')
        {
            *s = '\0';
            switch (field)
            {
            case 0:
                job->id = (int)get_u32(s1);
                break;
            case 1:
                job->C = get_u64(s1);
                break;
            case 2:
                job->O = get_u64(s1);
                break;
            case 3:
                job->flag = get_u64(s1);
                break;
            }
            s1 = s + 1;
            field++;
        }
        s++;
    }
    return field; // returns number of fields successfully parsed
}

// ---------- main ----------
int main(int argc, char **argv)
{
    // argv[1] = server taskset file path
    // argv[2] = time0 (nanoseconds, as printed by launch.c)
    if (argc < 3)
    {
        fprintf(stderr, "Usage: server_task <taskset_file> <time0>\n");
        exit(-1);
    }

    char *file = argv[1];
    unsigned long long time0 = (unsigned long long)atoll(argv[2]);

    // ---- read all jobs from file ----
    struct server_job jobs[MAX_JOBS];
    int njobs = 0;

    FILE *fd = fopen(file, "r");
    if (!fd)
    {
        perror(file);
        exit(-1);
    }

    char buffer[BUF_SIZE];
    while (fgets(buffer, BUF_SIZE, fd) != NULL && njobs < MAX_JOBS)
    {
        if (buffer[0] >= '0' && buffer[0] <= '9')
        {
            if (parse_job_line(buffer, &jobs[njobs]) >= 3)
                njobs++;
        }
        buffer[0] = 0;
    }
    fclose(fd);

    if (njobs == 0)
    {
        fprintf(stderr, "server_task: no jobs found in %s\n", file);
        exit(-1);
    }

    printf("ServerTask(%s): %d job(s) loaded, Q=%lldms T=%lldms\n",
           file, njobs, SERVER_Q / 1000000LL, SERVER_T / 1000000LL);

    // ---- register moker id (use the id from the first job) ----
    // printf("ServerTask(%s): registering moker id %d\n", file, jobs[0].id);
    if (syscall(SYS_MOKER_ID, jobs[0].id) < 0)
        perror("WARNING: SYS_MOKER_ID failed");

    printf("ServerTask(%s): SCHED_CBS settting server task\n", file);
    // ---- execute each job at its arrival time ----
    int i;
    for (i = 0; i < njobs; i++)
    {
        // arrival = time0 + O + OFFSET (same convention as task.c)
        unsigned long long arrival = time0 + jobs[i].O + OFFSET;
        struct timespec r;
        r.tv_sec = arrival / NSEC_PER_SEC;
        r.tv_nsec = arrival % NSEC_PER_SEC;

        // printf("ServerTask(%s): job %d (id=%d) sleeping until %llu\n",
        //        file, i, jobs[i].id, arrival);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &r, NULL);
        // printf("ServerTask(%s): job %d running C=%llu\n",file, i, jobs[i].C);
        if (i == 0)
        {

            // ---- set CBS server policy (flag=1 marks it as a soft server) ----
            struct sched_attr attr = {
                .size = sizeof(struct sched_attr),
                .sched_policy = SCHED_CBS,
                .sched_flags = 1, // server task
                .sched_runtime = SERVER_Q,
                .sched_deadline = SERVER_T,
                .sched_period = SERVER_T,
            };
            if (sched_setattr(0, &attr, 0) < 0)
            {
                perror("server_task: sched_setattr failed");
                exit(-1);
            }
        }
        do_work(jobs[i].C);

        // printf("ServerTask(%s): job %d done\n", file, i);
    }

    printf("ServerTask(%s): all jobs complete\n", file);
    exit(jobs[0].id);
    return 0;
}