#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

addr_t
sys_sbrk(void)
{
  addr_t addr;
  addr_t n;

  argaddr(0, &n);
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// TEST: Simple list search - generates telemetry
int
test_list_search(int target)
{
  int list[100];
  int i, count = 0;
  for(i = 0; i < 100; i++) {
    list[i] = i * 7;
  }
  for(i = 0; i < 100; i++) {
    if(list[i] == target) count++;
  }
  return count;
}

// Syscall wrappers for test functions
int
sys_test_list_search(void)
{
  int target;
  if(argint(0, &target) < 0)
    return -1;
  return test_list_search(target);
}

int
sys_test_process_scan(void)
{
  extern int test_process_list_scan(void);
  return test_process_list_scan();
}

int
sys_test_trap_scan(void)
{
  extern int test_trap_dispatch_scan(void);
  return test_trap_dispatch_scan();
}
