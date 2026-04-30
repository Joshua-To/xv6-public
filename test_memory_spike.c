/*
 * Test 2: Memory Spike Generator
 * Triggers MEMORY_SPIKE_THRESHOLD (256KB)
 * Allocates memory to trigger memory anomaly detection
 */

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  printf(1, "test_memory_spike: Allocating memory to trigger anomaly detection\n");
  printf(1, "Expected: AI daemon should detect memory spikes (>256KB)\n");
  printf(1, "Result: Patches may be generated and applied\n\n");

  // Allocate large chunk to trigger memory spike threshold
  char *buf = malloc(512 * 1024);  // 512KB > 256KB threshold
  if (!buf) {
    printf(1, "test_memory_spike: malloc failed\n");
    exit();
  }

  printf(1, "test_memory_spike: Allocated 512KB\n");

  // Touch the memory to ensure it's committed
  int i;
  for (i = 0; i < 512 * 1024; i += 4096) {
    buf[i] = 'X';
  }

  printf(1, "test_memory_spike: Memory touched, telemetry should show spike\n");

  // Release and wait for telemetry collection
  free(buf);
  sleep(2);

  printf(1, "test_memory_spike: Done\n");
  exit();
}
