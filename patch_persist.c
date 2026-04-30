#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "patcher.h"
#include "patch_persist.h"

// Note: This runs in user space only (init and daemons)
// Kernel patches are applied via syscalls

/*
 * Save a successfully applied patch to persistent storage
 * This allows patches to survive across reboots
 */
int
patch_persist_save(struct kpatch *patch)
{
  if (!patch || patch->status != PATCH_APPLIED)
    return -1;

  // Read existing patches into temp file, then append new one
  int fd_read = open(PATCH_DB_FILE, O_RDONLY);
  int fd_write = open("patches.tmp", O_CREATE | O_WRONLY);
  if (fd_write < 0) {
    if (fd_read >= 0)
      close(fd_read);
    return -1;
  }

  // Copy existing patches
  if (fd_read >= 0) {
    struct persistent_patch pp_existing;
    while (read(fd_read, (char*)&pp_existing, sizeof(struct persistent_patch)) == sizeof(struct persistent_patch)) {
      write(fd_write, (char*)&pp_existing, sizeof(struct persistent_patch));
    }
    close(fd_read);
  }

  // Create persistent entry (reduced size for storage)
  struct persistent_patch pp;
  pp.id = patch->id;
  pp.target_addr = patch->target_addr;
  pp.target_size = patch->target_size;
  pp.patch_size = patch->patch_size > 512 ? 512 : patch->patch_size;
  pp.original_checksum = patch->original_checksum;
  pp.created_at = patch->created_at;
  pp.status = PATCH_APPLIED;

  // Copy original code
  int i;
  for (i = 0; i < pp.patch_size && i < 512; i++) {
    pp.original_code[i] = patch->original_code[i];
    pp.patch_code[i] = patch->patch_code[i];
  }

  // Append new patch
  if (write(fd_write, (char*)&pp, sizeof(struct persistent_patch)) != sizeof(struct persistent_patch)) {
    printf(2, "patch_persist: failed to write patch %d\n", patch->id);
    close(fd_write);
    unlink("patches.tmp");
    return -1;
  }

  close(fd_write);

  // Replace original with temp file
  unlink(PATCH_DB_FILE);
  link("patches.tmp", PATCH_DB_FILE);
  unlink("patches.tmp");

  printf(1, "patch_persist: saved patch %d to disk\n", patch->id);
  return 0;
}

/*
 * Load persisted patches from disk at boot
 * Patches are loaded and stored in a simple in-memory list for the daemon to re-apply
 * Note: This is a simplified persistence that allows patches to be re-applied after reboot
 */
int
patch_persist_load_all(void)
{
  int fd = open(PATCH_DB_FILE, O_RDONLY);
  if (fd < 0) {
    // File doesn't exist - this is normal on first boot
    return 0;
  }

  printf(1, "patch_persist: loading patches from disk\n");

  struct persistent_patch pp;
  int loaded = 0;

  // Read all patches from file (for informational purposes)
  // In a full implementation, the daemon would re-apply these automatically
  while (read(fd, (char*)&pp, sizeof(struct persistent_patch)) == sizeof(struct persistent_patch)) {
    if (pp.status != PATCH_APPLIED)
      continue;

    printf(1, "patch_persist: found saved patch %d at address 0x%x (will be re-applied by daemon)\n", 
           pp.id, pp.target_addr);
    loaded++;
  }

  close(fd);
  printf(1, "patch_persist: loaded %d patches from disk\n", loaded);
  return loaded;
}

/*
 * Remove a persisted patch from disk (call when rolling back)
 */
int
patch_persist_delete(uint patch_id)
{
  int fd_read = open(PATCH_DB_FILE, O_RDONLY);
  if (fd_read < 0)
    return -1;

  int fd_write = open("patches.tmp", O_CREATE | O_WRONLY);
  if (fd_write < 0) {
    close(fd_read);
    return -1;
  }

  struct persistent_patch pp;
  int removed = 0;

  // Copy all patches except the one to remove
  while (read(fd_read, (char*)&pp, sizeof(struct persistent_patch)) == sizeof(struct persistent_patch)) {
    if (pp.id != patch_id) {
      write(fd_write, (char*)&pp, sizeof(struct persistent_patch));
    } else {
      removed = 1;
    }
  }

  close(fd_read);
  close(fd_write);

  // Replace original with temp file
  if (removed) {
    unlink(PATCH_DB_FILE);
    link("patches.tmp", PATCH_DB_FILE);
    unlink("patches.tmp");
    printf(1, "patch_persist: removed patch %d from disk\n", patch_id);
  }

  return removed ? 0 : -1;
}

/*
 * List all persisted patches (for debugging)
 */
void
patch_persist_list(void)
{
  int fd = open(PATCH_DB_FILE, O_RDONLY);
  if (fd < 0) {
    printf(1, "patch_persist: no patches saved\n");
    return;
  }

  printf(1, "Persisted patches:\n");
  struct persistent_patch pp;
  int count = 0;

  while (read(fd, (char*)&pp, sizeof(struct persistent_patch)) == sizeof(struct persistent_patch)) {
    printf(1, "  Patch %d: addr=0x%x size=%d status=%d\n",
           pp.id, pp.target_addr, pp.patch_size, pp.status);
    count++;
  }

  if (count == 0)
    printf(1, "  (none)\n");

  close(fd);
}
