/*
 * Test 3: Excessive Syscalls Generator
 * Triggers EXCESSIVE_CALLS_THRESHOLD (500 calls per interval)
 * Makes many syscalls in rapid succession
 */

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  printf(1, "test_excessive_syscalls: Making many syscalls rapidly\n");
  printf(1, "Expected: AI daemon should detect >500 syscalls in interval\n");
  printf(1, "Result: Patches may be generated and applied\n\n");

  // Make many syscalls rapidly
  int i;
  for (i = 0; i < 600; i++) {
    // Simple syscalls that are fast but generate telemetry
    getpid();
  }

  printf(1, "test_excessive_syscalls: Made 600 getpid() syscalls\n");
  
  // Give daemon time to collect telemetry
  sleep(1);

  printf(1, "test_excessive_syscalls: Done\n");
  exit();
}
