#pragma once

#include "types.h"
#include "patcher.h"
#include "param.h"

// Patch persistence file
#define PATCH_DB_FILE "patches.db"

// Persistent patch entry (simplified for storage)
struct persistent_patch {
  uint id;                    // Patch ID
  addr_t target_addr;         // Target address
  uint target_size;           // Original code size
  uint patch_size;            // Patch code size
  uchar original_code[512];   // Saved original code (reduced for storage)
  uchar patch_code[512];      // Patch code (reduced for storage)
  uint32 original_checksum;   // Checksum of original
  timestamp_t created_at;     // When created
  uchar status;               // PATCH_APPLIED, etc
};

// Persist successfully applied patch to disk
int patch_persist_save(struct kpatch *patch);

// Load persisted patches from disk at boot
int patch_persist_load_all(void);

// Remove persisted patch from disk
int patch_persist_delete(uint patch_id);

// List all persisted patches
void patch_persist_list(void);
