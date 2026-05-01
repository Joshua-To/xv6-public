/*
 * Test 1: Slow Syscall Generator
 * Triggers SLOW_SYSCALL_THRESHOLD (2000µs)
 * This makes the AI daemon detect slow syscalls and generate patches
 */

#include "types.h"
#include "stat.h"
#include "user.h"

// Simulate slow work by calling expensive syscall many times
int
main(void)
{
  int i;
  int batch = 0;
  
  printf(1, "test_slow_syscalls: Generating slow syscalls to trigger anomaly detection\n");
  printf(1, "Expected: AI daemon should detect >20%% of samples as slow (>1000µs)\n");
  printf(1, "Result: Patches may be generated and applied\n\n");
  
  // Make stat() syscalls continuously for 10 seconds to ensure daemon catches them
  // This gives multiple 3-second analysis windows
  for (batch = 0; batch < 50; batch++) {
    for (i = 0; i < 100; i++) {
      struct stat st;
      // Call stat on root directory - this is slow due to inode lookup
      stat("/", &st);
    }
    printf(1, "test_slow_syscalls: batch %d done\n", batch);
    sleep(1);  // Sleep to allow daemon to read telemetry between batches
  }
  
  printf(1, "test_slow_syscalls: Done\n");
  exit();
}
