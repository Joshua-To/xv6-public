#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "proc.h"
#include "spinlock.h"
#include "telemetry.h"
#include "patcher.h"

// Global patch history
struct patch_history patch_history;

// Simple CRC32 implementation for checksum
uint32
crc32(uchar *data, uint len)
{
  uint32 crc = 0xFFFFFFFF;
  uint i, j;
  
  for (i = 0; i < len; i++) {
    crc ^= data[i];
    for (j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc = crc >> 1;
    }
  }
  
  return crc ^ 0xFFFFFFFF;
}

// Initialize patcher subsystem
void
patcher_init(void)
{
  initlock(&patch_history.lock, "patch_history");
  
  patch_history.active_patches = 0;
  patch_history.total_applied = 0;
  patch_history.total_rolled_back = 0;
  
  // Initialize all patch slots as unused
  int i;
  for (i = 0; i < MAX_AI_PATCHES; i++) {
    patch_history.patches[i].id = i;
    patch_history.patches[i].status = PATCH_UNUSED;
    patch_history.patches[i].target_addr = 0;
  }
  
  cprintf("patcher: initialized with max %d patches\n", MAX_AI_PATCHES);
}

// Save original code before patching
void
patch_save_original(struct kpatch *patch)
{
  if (!patch || !patch->target_addr)
    return;
  
  // Copy original code from kernel memory
  uchar *src = (uchar *)patch->target_addr;
  uint i;
  
  for (i = 0; i < patch->target_size && i < MAX_PATCH_SIZE; i++) {
    patch->original_code[i] = src[i];
  }
  
  // Calculate checksum
  patch->original_checksum = crc32(patch->original_code, patch->target_size);
}

// Validate patch checksum
int
patch_validate_checksum(struct kpatch *patch)
{
  if (!patch || patch->target_size == 0)
    return PATCH_ERR_INVALID_ADDR;
  
  // Current code at target address
  uchar *target = (uchar *)patch->target_addr;
  uchar current_code[MAX_PATCH_SIZE];
  uint i;
  
  for (i = 0; i < patch->target_size && i < MAX_PATCH_SIZE; i++) {
    current_code[i] = target[i];
  }
  
  uint32 current_checksum = crc32(current_code, patch->target_size);
  
  // Verify current code matches original before applying
  if (current_checksum != patch->original_checksum) {
    cprintf("patcher: checksum mismatch! expected %x, got %x\n", 
            patch->original_checksum, current_checksum);
    return PATCH_ERR_INVALID_CHECKSUM;
  }
  
  return 0;
}

// Validate patch address is within kernel space
int
patch_validate_bounds(struct kpatch *patch)
{
  if (!patch)
    return PATCH_ERR_INVALID_ADDR;
  
  // Ensure address is in kernel space
  if (patch->target_addr < KERNBASE || 
      patch->target_addr >= KERNBASE + 0x10000000) {
    return PATCH_ERR_INVALID_ADDR;
  }
  
  // Ensure patch size is reasonable
  if (patch->patch_size == 0 || patch->patch_size > MAX_PATCH_SIZE)
    return PATCH_ERR_INVALID_ADDR;
  
  // Ensure target size is set
  if (patch->target_size == 0 || patch->target_size > MAX_PATCH_SIZE)
    return PATCH_ERR_INVALID_ADDR;
  
  return 0;
}

// Validate patch format and content
int
patch_validate_format(struct kpatch *patch)
{
  if (!patch)
    return PATCH_ERR_INVALID_ADDR;
  
  // Check patch code is not all zeros
  uint i, zero_count = 0;
  for (i = 0; i < patch->patch_size; i++) {
    if (patch->patch_code[i] == 0)
      zero_count++;
  }
  
  // Entire patch can't be zeros (invalid code)
  if (zero_count == patch->patch_size)
    return PATCH_ERR_INVALID_CHECKSUM;
  
  return 0;
}

// Apply patch (simple direct code replacement)
// Note: This is a simplified implementation - production would need:
// - Instruction boundary detection
// - Trampoline setup for preserving control flow
// - Atomic patching with interrupts disabled
int
patch_apply(struct kpatch *patch, int pid)
{
  if (!patch)
    return PATCH_ERR_INVALID_ADDR;
  
  // Validate patch before application
  int err = 0;
  
  err = patch_validate_bounds(patch);
  if (err)
    return err;
  
  err = patch_validate_format(patch);
  if (err)
    return err;
  
  // Save original code first
  patch_save_original(patch);
  
  // Verify current code matches original
  err = patch_validate_checksum(patch);
  if (err) {
    cprintf("patcher: code doesn't match original, cannot apply patch safely\n");
    return err;
  }
  
  // Disable interrupts during code replacement
  pushcli();
  
  // Copy new code to target address
  uchar *target = (uchar *)patch->target_addr;
  uint i;
  for (i = 0; i < patch->patch_size && i < MAX_PATCH_SIZE; i++) {
    target[i] = patch->patch_code[i];
  }
  
  // Mark as applied
  patch->status = PATCH_APPLIED;
  patch->applied_at = telemetry_now();
  patch->applied_by_pid = pid;
  
  popcli();
  
  cprintf("patcher: patch %d applied by PID %d (%d bytes at %x)\n",
          patch->id, pid, patch->patch_size, patch->target_addr);
  
  telemetry_record_anomaly(pid, ANOMALY_EXCESSIVE_SYSCALLS, 1);
  
  return patch->id;  // Return patch ID
}

// Rollback patch (restore original code)
int
patch_rollback(uint patch_id)
{
  if (patch_id >= MAX_AI_PATCHES)
    return -1;
  
  struct kpatch *patch = &patch_history.patches[patch_id];
  
  if (patch->status != PATCH_APPLIED) {
    cprintf("patcher: patch %d not currently applied\n", patch_id);
    return -1;
  }
  
  if (patch->target_addr == 0)
    return -1;
  
  // Disable interrupts during rollback
  pushcli();
  
  // Restore original code
  uchar *target = (uchar *)patch->target_addr;
  uint i;
  for (i = 0; i < patch->target_size && i < MAX_PATCH_SIZE; i++) {
    target[i] = patch->original_code[i];
  }
  
  patch->status = PATCH_REVERTED;
  
  popcli();
  
  cprintf("patcher: patch %d rolled back (restored %d bytes)\n",
          patch_id, patch->target_size);
  
  patch_history.total_rolled_back++;
  
  return 0;
}

// Verify that a patch was applied correctly
int
patch_verify_applied(uint patch_id)
{
  if (patch_id >= MAX_AI_PATCHES)
    return 0;
  
  struct kpatch *patch = &patch_history.patches[patch_id];
  
  if (patch->status != PATCH_APPLIED)
    return 0;
  
  // Verify patch code is at target address
  uchar *target = (uchar *)patch->target_addr;
  uint i;
  
  for (i = 0; i < patch->patch_size && i < MAX_PATCH_SIZE; i++) {
    if (target[i] != patch->patch_code[i]) {
      cprintf("patcher: verification failed at offset %d\n", i);
      return 0;  // Verification failed
    }
  }
  
  return 1;  // Verification successful
}

// Get patch by ID
struct kpatch*
patch_get(uint patch_id)
{
  if (patch_id >= MAX_AI_PATCHES)
    return 0;
  
  return &patch_history.patches[patch_id];
}

// Get patch status
int
patch_get_status(uint patch_id)
{
  if (patch_id >= MAX_AI_PATCHES)
    return -1;
  
  return patch_history.patches[patch_id].status;
}

// Get count of active patches
int
patch_get_active_count(void)
{
  int count = 0;
  int i;
  
  acquire(&patch_history.lock);
  for (i = 0; i < MAX_AI_PATCHES; i++) {
    if (patch_history.patches[i].status == PATCH_APPLIED)
      count++;
  }
  release(&patch_history.lock);
  
  return count;
}

// Dump patch history to console (debug)
void
dump_patch_history(void)
{
  cprintf("\n=== Patch History ===\n");
  cprintf("Active patches: %d\n", patch_get_active_count());
  cprintf("Total applied: %d\n", patch_history.total_applied);
  cprintf("Total rolled back: %d\n", patch_history.total_rolled_back);
  cprintf("===================\n\n");
  
  int i;
  for (i = 0; i < MAX_AI_PATCHES; i++) {
    struct kpatch *p = &patch_history.patches[i];
    if (p->status != PATCH_UNUSED) {
      cprintf("Patch %d: status=%d, addr=%x, size=%d\n",
              p->id, p->status, p->target_addr, p->patch_size);
    }
  }
}
