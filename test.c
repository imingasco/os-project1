#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>

int main(){
    printf("%llu\n", syscall(333));
    syscall(334, getpid(), 123812931210, 12930124801);
    return 0;
}
