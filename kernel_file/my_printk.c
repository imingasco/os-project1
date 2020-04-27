#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage void sys_printtime(int pid, unsigned long long start, unsigned long long finish){
	unsigned long long divisor = 1000000000;
	printk("[Project1] %d %llu.%09llu %llu.%09llu\n", pid, start / divisor, start % divisor, finish / divisor, finish % divisor);
}
