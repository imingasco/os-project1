#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>

#define MAXN 10000

typedef struct{
    char name[64];
    int R;
    int T;
    pid_t pid;
}Process;

Process process[MAXN + 1];
cpu_set_t cpu_set;
int *arg_ptr;

int cmp(const void *a, const void *b){
    Process *ptr1 = (Process *)a, *ptr2 = (Process *)b;
    if(ptr1->R < ptr2->R) return -1;
    else if(ptr1->R == ptr2->R) return 0;
    else return 1;
}

int min(int a, int b){
    return (a < b)? a : b;
}

void unit_time(void){
    volatile unsigned long i;
    for(i = 0; i < 1000000UL; i++);
}

void run_process(pid_t pid){
    struct sched_param para;
    para.sched_priority = 99;
    sched_setscheduler(pid, SCHED_FIFO, &para);
}

void make_child(int time, int idx){
    pid_t pid = fork();
    if(pid == 0){
        char fork_time[100], execute_time[100];
        sprintf(fork_time, "%d", time);
        sprintf(execute_time, "%d", process[idx].T);
        execlp("./child", fork_time, execute_time, process[idx].name, NULL);
    }
    else{
        process[idx].pid = pid;
        run_process(pid);
    }
}

int next_ready_time(int idx, int now_time){
    for(int i = 0; i < idx; i++)
        if(process[i].R <= now_time && process[i].T > 0)
            return now_time;

    for(int i = now_time; i < process[idx].R; i++) unit_time();
    if(process[idx].R > now_time)
        return process[idx].R;
    else
        return now_time;
}

int fork_child(int idx, int now_time, int N){
    while(process[idx].R <= now_time && idx < N){
        make_child(now_time, idx);
        idx++;
    }
    return idx;
}

int allocate(int idx, int now_idx, int N, int now_time){
    return (idx < N)? min(process[idx].R - now_time, process[now_idx].T) : process[now_idx].T;
}

void FIFO(int N){
    // idx: next to fork, now_idx: next to execute
    int idx = 0, now_idx = 0, now_time = 0;
    while(now_idx < N){
        now_time = next_ready_time(idx, now_time);
        idx = fork_child(idx, now_time, N);

        // Run first process
        int execute_time = allocate(idx, now_idx, N, now_time);
        // fprintf(stderr, "process %d run at %d\n", process[now_idx].pid, now_time);
        memcpy(arg_ptr, &execute_time, sizeof(int));
        now_time += *arg_ptr;
        process[now_idx].T -= *arg_ptr;
        run_process(process[now_idx].pid);
        // fprintf(stderr, "process %d stop at %d\n", process[now_idx].pid, now_time);
        if(process[now_idx].T <= 0)
            now_idx++;
    }
}

void SJF(int N){
    // idx: next to fork, now_idx: ready to run
    int idx = 0, now_idx = 0, now_time = 0;
    while(now_idx < N){
        now_time = next_ready_time(idx, now_time);
        idx = fork_child(idx, now_time, N);
        now_idx = N;
        int shortest = INT_MAX;
        for(int i = 0; i < idx; i++){
            if(process[i].T > 0 && process[i].T < shortest){
                shortest = process[i].T;
                now_idx = i;
            }
        }
        while(process[now_idx].T > 0){
            // Run Shortest job
            // fprintf(stderr, "shorest is %d with %d\n", now_idx, shortest);
            int execute_time = allocate(idx, now_idx, N, now_time);
            // fprintf(stderr, "process %d run at %d\n", process[now_idx].pid, now_time);
            memcpy(arg_ptr, &execute_time, sizeof(int));
            now_time += *arg_ptr;
            process[now_idx].T -= *arg_ptr;

            run_process(process[now_idx].pid);
            // fprintf(stderr, "process %d stop at %d\n", process[now_idx].pid, now_time);
            idx = fork_child(idx, now_time, N);
        }
    }
}

void RR(int N){
    // idx: next to fork
    int idx = 0, now_time = 0, done = 0;
    while(done < N){
        now_time = next_ready_time(idx, now_time);
        idx = fork_child(idx, now_time, N);

        // Round Robin
        for(int i = 0; i < idx; i++){
            int timeout = 500;
            while(process[i].T > 0 && timeout > 0){
                int execute_time = min(allocate(idx, i, N, now_time), timeout);
                // fprintf(stderr, "process %d run at %d\n", process[i].pid, now_time);
                memcpy(arg_ptr, &execute_time, sizeof(int));
                now_time += *arg_ptr;
                process[i].T -= *arg_ptr;
                if(process[i].T <= 0)
                    done++;
                timeout -= execute_time;
                run_process(process[i].pid); 
                // fprintf(stderr, "process %d stop at %d\n", process[i].pid, now_time);
                idx = fork_child(idx, now_time, N);
            }
        }
    }
}

void PSJF(int N){
    // idx: next to fork, now_idx: ready to run
    int idx = 0, now_idx = 0, now_time = 0;
    while(now_idx < N){
        int timeout = 500, shortest = INT_MAX;
        now_time = next_ready_time(idx, now_time);
        idx = fork_child(idx, now_time, N);

        now_idx = N;
        for(int i = 0; i < idx; i++){
            if(process[i].T > 0 && process[i].T < shortest){
                shortest = process[i].T;
                now_idx = i;
            }
        }
        while(process[now_idx].T > 0 && timeout > 0){
            // fprintf(stderr, "process %d run at %d\n", process[now_idx].pid, now_time);
            int execute_time = min(allocate(idx, now_idx, N, now_time), timeout);
            memcpy(arg_ptr, &execute_time, sizeof(int));
            now_time += *arg_ptr;
            process[now_idx].T -= *arg_ptr;
            timeout -= *arg_ptr;
            run_process(process[now_idx].pid);
            // fprintf(stderr, "process %d stop at %d\n", process[now_idx].pid, now_time);
            idx = fork_child(idx, now_time, N);
        }
    }
}

int main(int argc, char *argv[]){
    // create a shared memory
    int fd = shm_open("share_mem", O_RDWR | O_CREAT, 0777);
    if(fd < 0) fprintf(stderr, "share memory fails");
    ftruncate(fd, sizeof(int));
    arg_ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // bind  to single CPU 
    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);
    sched_setaffinity(getpid(), sizeof(cpu_set), &cpu_set);
    struct sched_param para;
    para.sched_priority = 98;
    sched_setscheduler(getpid(), SCHED_FIFO, &para);

    // Read input and sort by ready time
    char policy[10];
    int N;
    scanf("%s", policy);
    scanf("%d", &N);
    for(int i = 0; i < N; i++)
        scanf("%s%d%d", process[i].name, &process[i].R, &process[i].T);
    qsort(process, N, sizeof(Process), cmp);
    
    // Diffrent scheduling policy
    if(strcmp(policy, "FIFO") == 0)
        FIFO(N);
    else if(strcmp(policy, "SJF") == 0)
        SJF(N);
    else if(strcmp(policy, "RR") == 0)
        RR(N);
    else if(strcmp(policy, "PSJF") == 0)
        PSJF(N); 
    
    // Wait all process
    int status[N];
    for(int i = 0; i < N; i++){
        wait(&status[i]);
        //printf("%d exit at %d\n", i, status[i]);
    }
    return 0;
}
