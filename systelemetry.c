#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "telemetry.h"
#include "patcher.h"
#include "shared.h"

#define MONITOR_TICKS_PER_SECOND 100

static struct monitor_control monitor_state;
static struct spinlock monitor_lock;

static void
monitor_check_timeout(void)
{
  if (!monitor_state.active || monitor_state.duration_seconds <= 0)
    return;

  uint64 elapsed_ticks = (uint64)ticks - monitor_state.start_time;
  uint64 timeout_ticks = (uint64)monitor_state.duration_seconds * MONITOR_TICKS_PER_SECOND;
  if (elapsed_ticks >= timeout_ticks) {
    monitor_state.active = 0;
    monitor_state.cmd = MONITOR_CMD_STOP;
  }
}

// sys_telemetry_read: Read available telemetry samples into user buffer
// Arguments: user_buf (addr), max_count (int)
// Returns: number of samples copied, or -1 on error
addr_t
sys_telemetry_read(void)
{
  addr_t user_buf;
  int max_count;
  
  if (argaddr(0, &user_buf) < 0 || argint(1, &max_count) < 0)
    return -1;
  
  // Validate user buffer is in valid user address space
  if (user_buf < PGSIZE || user_buf >= proc->sz)
    return -1;
  
  if (max_count <= 0 || max_count > 32)
    max_count = 32;  // Limit to reasonable size (matches TELEMETRY_BATCH_SIZE)
  
  // Read samples from kernel telemetry ring buffer
  int samples_read = telemetry_read_samples((struct telemetry_sample *)user_buf, max_count);
  
  return samples_read;
}

// sys_telemetry_subscribe: Register for async telemetry notifications
// Arguments: notification_level (int) - only report when buffer reaches N% full
addr_t
sys_telemetry_subscribe(void)
{
  int notification_level;
  
  if (argint(0, &notification_level) < 0)
    return -1;
  
  if (notification_level < 0 || notification_level > 100)
    notification_level = 50;  // Default: notify at 50% buffer usage
  
  // For now, just validate and return success
  // Full implementation would register process for async notifications
  
  return 0;
}

// sys_patch_apply: Apply a kernel code patch
// Arguments: patch_data (addr), patch_size (int)
addr_t
sys_patch_apply(void)
{
  addr_t patch_data;
  int patch_size;
  
  if (argaddr(0, &patch_data) < 0 || argint(1, &patch_size) < 0)
    return -1;
  
  // Validate user buffer and size
  if (patch_data < PGSIZE || patch_data >= proc->sz)
    return -1;
  
  if (patch_size <= 0 || patch_size > MAX_PATCH_SIZE)
    return -1;  // Patches limited to MAX_PATCH_SIZE
  
  // Find an available patch slot
  struct kpatch *new_patch = 0;
  int i;
  acquire(&patch_history.lock);
  for (i = 0; i < MAX_AI_PATCHES; i++) {
    if (patch_history.patches[i].status == PATCH_UNUSED) {
      new_patch = &patch_history.patches[i];
      break;
    }
  }
  release(&patch_history.lock);
  
  if (!new_patch) {
    cprintf("systelemetry: no available patch slots\n");
    return -1;
  }
  
  // For now, just log and validate basic patch structure
  // Actual patch data would be parsed from user buffer
  cprintf("Patch request from PID %d: %d bytes\n", proc->pid, patch_size);
  
  // Initialize patch slot (simplified - normally would copy data from user space)
  new_patch->status = PATCH_VALID;
  new_patch->patch_size = patch_size;
  new_patch->created_at = telemetry_now();
  new_patch->applied_by_pid = proc->pid;
  
  // Record telemetry
  telemetry_record_anomaly(proc->pid, ANOMALY_EXCESSIVE_SYSCALLS, 1);
  
  cprintf("systelemetry: patch %d queued for potential application\n", new_patch->id);

  // If monitoring is configured to stop once a patch is applied, deactivate it.
  acquire(&monitor_lock);
  if (monitor_state.active && monitor_state.stop_if_patch_applied) {
    monitor_state.active = 0;
    monitor_state.cmd = MONITOR_CMD_STOP;
  }
  release(&monitor_lock);

  return new_patch->id;  // Return patch ID
}

// sys_patch_rollback: Rollback a previously applied patch
// Arguments: patch_id (int)
addr_t
sys_patch_rollback(void)
{
  int patch_id;
  
  if (argint(0, &patch_id) < 0)
    return -1;
  
  if (patch_id < 0 || patch_id >= MAX_AI_PATCHES)
    return -1;
  
  // Call patcher to rollback
  int result = patch_rollback(patch_id);
  
  if (result == 0) {
    cprintf("systelemetry: rollback successful for patch %d\n", patch_id);
  } else {
    cprintf("systelemetry: rollback failed for patch %d\n", patch_id);
  }
  
  return result;  // 0 on success
}

// Initialize monitor lock (call from somewhere at startup)
static int monitor_initialized = 0;

static void
monitor_ensure_initialized(void)
{
  if (!monitor_initialized) {
    initlock(&monitor_lock, "monitor");
    monitor_initialized = 1;
  }
}

// Kernel uptime in ticks (extern from proc.c)
extern uint ticks;

// sys_monitor_control: Control background monitoring daemon
// Arguments: monitor_cmd (addr to struct monitor_control)
// Returns: status (1=active, 0=inactive) or -1 on error
addr_t
sys_monitor_control(void)
{
  addr_t monitor_cmd_addr;
  struct monitor_control *user_cmd;
  struct monitor_control cmd;
  
  // Get the address
  if (argaddr(0, &monitor_cmd_addr) < 0)
    return -1;
  
  // Validate the buffer bounds
  if (monitor_cmd_addr < PGSIZE || monitor_cmd_addr >= proc->sz)
    return -1;
  
  if (monitor_cmd_addr + sizeof(struct monitor_control) > proc->sz)
    return -1;
  
  user_cmd = (struct monitor_control*)monitor_cmd_addr;
  cmd = *user_cmd;
  
  // Ensure lock is initialized
  monitor_ensure_initialized();
  
  acquire(&monitor_lock);
  monitor_check_timeout();
  
  switch (cmd.cmd) {
    case MONITOR_CMD_START:
      // Count patches at start
      uint patch_count_start = 0;
      acquire(&patch_history.lock);
      int j;
      for (j = 0; j < MAX_AI_PATCHES; j++) {
        if (patch_history.patches[j].status != PATCH_UNUSED) {
          patch_count_start++;
        }
      }
      release(&patch_history.lock);
      
      monitor_state.cmd = MONITOR_CMD_START;
      monitor_state.duration_seconds = cmd.duration_seconds;
      monitor_state.stop_if_patch_applied = cmd.stop_if_patch_applied;
      monitor_state.telemetry_verbose = cmd.telemetry_verbose;
      monitor_state.active = 1;
      monitor_state.start_time = ticks;
      monitor_state.patches_at_start = patch_count_start;
      monitor_state.patches_at_stop = 0;
      monitor_state.monitor_duration = 0;
      monitor_state.syscalls_sampled = 0;
      monitor_state.avg_syscall_time_us = 0;
      *user_cmd = monitor_state;
      release(&monitor_lock);
      return 1;
      
    case MONITOR_CMD_STOP:
      // Calculate duration
      uint64 elapsed_ticks = (uint64)ticks - monitor_state.start_time;
      monitor_state.monitor_duration = elapsed_ticks / MONITOR_TICKS_PER_SECOND;
      
      // Count current patches
      uint patch_count = 0;
      acquire(&patch_history.lock);
      int i;
      for (i = 0; i < MAX_AI_PATCHES; i++) {
        if (patch_history.patches[i].status != PATCH_UNUSED) {
          patch_count++;
        }
      }
      release(&patch_history.lock);
      monitor_state.patches_at_stop = patch_count;
      
      // Calculate average syscall time from telemetry
      // Scan entire buffer for syscall samples
      uint total_syscall_time = 0;
      uint syscall_count = 0;
      acquire(&telemetry_buf.lock);
      
      // Scan the entire buffer for TELEMETRY_SYSCALL entries
      for (int i = 0; i < TELEMETRY_BUFFER_SIZE; i++) {
        if (telemetry_buf.samples[i].event_type == TELEMETRY_SYSCALL) {
          total_syscall_time += telemetry_buf.samples[i].duration_us;
          syscall_count++;
          if (syscall_count >= 100) break;
        }
      }
      release(&telemetry_buf.lock);
      
      monitor_state.syscalls_sampled = syscall_count;
      if (syscall_count > 0) {
        monitor_state.avg_syscall_time_us = total_syscall_time / syscall_count;
      } else {
        monitor_state.avg_syscall_time_us = 0;
      }
      
      monitor_state.active = 0;
      *user_cmd = monitor_state;
      release(&monitor_lock);
      return 0;
      
    case MONITOR_CMD_PAUSE:
      monitor_state.cmd = MONITOR_CMD_PAUSE;
      *user_cmd = monitor_state;
      release(&monitor_lock);
      return monitor_state.active;
      
    case MONITOR_CMD_RESUME:
      monitor_state.cmd = MONITOR_CMD_START;
      monitor_state.start_time = ticks;
      monitor_state.active = 1;
      *user_cmd = monitor_state;
      release(&monitor_lock);
      return 1;
      
    case MONITOR_CMD_STATUS:
      *user_cmd = monitor_state;
      release(&monitor_lock);
      return monitor_state.active;
      
    default:
      release(&monitor_lock);
      return -1;
  }
}

// Get current monitor state (for kernel-space use only)
struct monitor_control*
get_monitor_state(void)
{
  return &monitor_state;
}
