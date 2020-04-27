#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/timekeeping.h>

asmlinkage unsigned long long sys_nstime(void){
	struct timespec time;
	getnstimeofday(&time);
	return (unsigned long long)time.tv_sec * 1000000000 + (unsigned long long)time.tv_nsec; 
}

