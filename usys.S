#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .global name; \
  name: \
    mov $SYS_ ## name, %rax; \
    mov %rcx, %r10 ;\
    syscall		  ;\
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(telemetry_read)
SYSCALL(telemetry_subscribe)
SYSCALL(patch_apply)
SYSCALL(patch_rollback)
SYSCALL(monitor_control)
SYSCALL(test_list_search)
SYSCALL(test_process_scan)
SYSCALL(test_trap_scan)
