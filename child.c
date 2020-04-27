#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>

void unit_time(void){
    volatile unsigned long i;
    for(i = 0; i < 1000000UL; i++);
}

void leave_cpu(void){
    struct sched_param para;
    para.sched_priority = 1;
    sched_setscheduler(getpid(), SCHED_FIFO, &para);
}

int main(int argc, char *argv[]){
    assert(argc == 3);
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);
    sched_setaffinity(getpid(), sizeof(cpu_set), &cpu_set);
    int fork_time = atoi(argv[0]);
    int execute_time = atoi(argv[1]);
    printf("%s %d\n", argv[2], getpid());
    int fd = shm_open("share_mem", O_RDWR, 0777);
    if(fd < 0) fprintf(stderr, "child share memory failed");
    int *allocate_time = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    // fprintf(stderr, "%d forked at %d\n", getpid(), fork_time);
    leave_cpu();

    unsigned long long start = syscall(333);
    // fprintf(stderr, "%d start at %llu\n", getpid(), start);
    while(1){
        // fprintf(stderr, "%d remain %d, allocated %d\n", getpid(), execute_time, *allocate_time);
        execute_time -= *allocate_time;
        for(volatile int i = 0; i < *allocate_time; i++) unit_time();
        /*
        while(*allocate_time > 0){
            unit_time();
            *allocate_time--;
        }
        */
        if(execute_time > 0)
            leave_cpu();
        else{
            unsigned long long finish = syscall(333);
            // fprintf(stderr, "%d finish at %llu\n", getpid(), finish);
            syscall(334, getpid(), start, finish);
            exit(0);
        }
    }
    return 0;
}
