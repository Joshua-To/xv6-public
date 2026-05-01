/*
 * AI Evolution Test Utility
 * Calls kernel test functions repeatedly to generate telemetry
 * that should trigger the AI daemon's optimization loop
 */

#include "types.h"
#include "stat.h"
#include "user.h"

// Forward declarations for test functions
int test_list_search(int target);
int test_process_scan(void);
int test_trap_scan(void);

void
usage(void)
{
  printf(2, "Usage: ai_test [duration]\n");
  printf(2, "  duration: seconds to run (default: 30)\n");
  exit();
}

int
main(int argc, char *argv[])
{
  int duration = 30;
  int start_time = uptime();
  int elapsed = 0;
  int iteration = 0;
  int result;
  
  if (argc > 1) {
    duration = atoi(argv[1]);
    if (duration <= 0) duration = 30;
  }
  
  printf(1, "AI Evolution Test: Starting %d second workload\n", duration);
  printf(1, "This will generate telemetry for the AI daemon to analyze\n\n");
  
  while (elapsed < duration) {
    iteration++;
    
    // Call test functions in kernel to generate syscalls
    // Each call creates a syscall event in telemetry
    
    if (iteration % 3 == 0) {
      result = test_list_search(42);
      if (iteration % 30 == 0)
        printf(1, "Iteration %d: list_search result = %d\n", iteration, result);
    }
    
    if (iteration % 3 == 1) {
      result = test_process_scan();
      if (iteration % 30 == 0)
        printf(1, "Iteration %d: process_scan result = %d\n", iteration, result);
    }
    
    if (iteration % 3 == 2) {
      result = test_trap_scan();
      if (iteration % 30 == 0)
        printf(1, "Iteration %d: trap_scan result = %d\n", iteration, result);
    }
    
    // Update elapsed time
    elapsed = uptime() - start_time;
    
    // Print progress every 10 seconds
    if (iteration % 10 == 0) {
      printf(1, "[%d/%d sec] Completed %d iterations\n", elapsed, duration, iteration);
    }
    
    sleep(1);
  }
  
  printf(1, "\nAI Evolution Test: Completed\n");
  printf(1, "Total iterations: %d\n", iteration);
  printf(1, "Watch the AI daemon output for [PATTERN DETECTED] messages\n");
  printf(1, "The AI should have attempted optimizations\n");
  
  exit();
}
