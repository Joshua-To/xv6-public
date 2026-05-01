#pragma once

#include "types.h"
#include "spinlock.h"
#include "param.h"

// Patch status
#define PATCH_UNUSED      0
#define PATCH_VALID       1
#define PATCH_APPLIED     2
#define PATCH_REVERTED    3

// Patch errors
#define PATCH_ERR_INVALID_ADDR     -1
#define PATCH_ERR_INVALID_CHECKSUM -2
#define PATCH_ERR_SIZE_MISMATCH    -3
#define PATCH_ERR_VERIFICATION_FAIL -4

// Maximum patch size (1 KB)
#define MAX_PATCH_SIZE     1024

// Trampoline size (JMP instruction = 13 bytes for 64-bit)
#define TRAMPOLINE_SIZE    16

// Kernel patch structure
struct kpatch {
  uint id;                    // Unique patch ID
  addr_t target_addr;         // Function address to patch
  uint target_size;           // Original code size
  uint patch_size;            // New code size
  uchar original_code[MAX_PATCH_SIZE];  // Saved original code
  uchar patch_code[MAX_PATCH_SIZE];     // New code to inject
  uint32 original_checksum;   // CRC32 of original code before patch
  timestamp_t applied_at;     // When patch was applied
  timestamp_t created_at;     // When patch was created
  uchar status;               // PATCH_* status
  int applied_by_pid;         // Which process applied this patch
};

// Patch history entry
struct patch_history {
  struct spinlock lock;
  struct kpatch patches[MAX_AI_PATCHES];  // Array of patches
  uint active_patches;                    // Number of currently applied patches
  uint total_applied;                     // Total patches ever applied
  uint total_rolled_back;                 // Total patches rolled back
};

// Global patch history
extern struct patch_history patch_history;

// Initialize patcher subsystem
void patcher_init(void);

// Patch validation functions
int patch_validate_checksum(struct kpatch *patch);
int patch_validate_bounds(struct kpatch *patch);
int patch_validate_format(struct kpatch *patch);

// Patch application/rollback
int patch_apply(struct kpatch *patch, int pid);
int patch_rollback(uint patch_id);
int patch_verify_applied(uint patch_id);

// Patch queries
struct kpatch* patch_get(uint patch_id);
int patch_get_status(uint patch_id);
int patch_get_active_count(void);

// Utility functions
uint32 crc32(uchar *data, uint len);
void patch_save_original(struct kpatch *patch);
void dump_patch_history(void);
