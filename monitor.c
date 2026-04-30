/*
 * Monitor command - Control the background AI daemon monitoring
 * 
 * Usage:
 *   monitor start <seconds> [patch] [telemetry]  - Start monitoring for N seconds
 *   monitor stop                                  - Stop monitoring immediately
 *   monitor status                                - Show monitor status
 *   monitor pause                                 - Pause monitoring
 *   monitor resume                                - Resume monitoring
 * 
 * Options for 'start':
 *   patch      - Stop monitoring when a patch is applied
 *   telemetry  - Show verbose telemetry output (not just stats)
 * 
 * Examples:
 *   monitor start 60              - Monitor 60s, show only stats
 *   monitor start 60 telemetry    - Monitor 60s, show detailed telemetry + stats
 *   monitor start 60 patch        - Monitor 60s, stop if patch applied (stats only)
 *   monitor start 0               - Monitor indefinitely (0 = no timeout)
 */

#include "types.h"
#include "stat.h"
#include "user.h"
#include "shared.h"

void
usage(void)
{
  printf(2, "Usage: monitor <command> [args]\n");
  printf(2, "  start <seconds> [patch] [telemetry] - Start monitoring (0=indefinite)\n");
  printf(2, "    patch     - Stop monitoring when a patch is applied\n");
  printf(2, "    telemetry - Show verbose telemetry output (not just stats)\n");
  printf(2, "  stop                     - Stop monitoring\n");
  printf(2, "  status                   - Show monitor status\n");
  printf(2, "  pause                    - Pause monitoring\n");
  printf(2, "  resume                   - Resume monitoring\n");
  exit();
}

int
main(int argc, char *argv[])
{
  struct monitor_control cmd;
  
  if (argc < 2) {
    usage();
  }
  
  // Parse command
  char *command = argv[1];
  
  cmd.cmd = 0;
  cmd.duration_seconds = 0;
  cmd.stop_if_patch_applied = 0;
  cmd.telemetry_verbose = 0;
  cmd.active = 0;
  cmd.start_time = 0;
  cmd.patches_at_start = 0;
  cmd.patches_at_stop = 0;
  cmd.monitor_duration = 0;
  cmd.syscalls_sampled = 0;
  cmd.avg_syscall_time_us = 0;
  
  if (strcmp(command, "start") == 0) {
    if (argc < 3) {
      printf(2, "start requires duration argument\n");
      usage();
    }
    cmd.cmd = MONITOR_CMD_START;
    cmd.duration_seconds = atoi(argv[2]);
    
    // Check for optional "patch" argument
    if (argc >= 4 && strcmp(argv[3], "patch") == 0) {
      cmd.stop_if_patch_applied = 1;
    }
    
    // Check for optional "telemetry" argument to enable verbose output
    if (argc >= 4 && strcmp(argv[3], "telemetry") == 0) {
      cmd.telemetry_verbose = 1;
    }
    if (argc >= 5 && strcmp(argv[4], "telemetry") == 0) {
      cmd.telemetry_verbose = 1;
    }
    
    printf(1, "Monitor: Starting daemon monitoring for %d seconds (stop_on_patch=%d, verbose=%d)\n",
           cmd.duration_seconds, cmd.stop_if_patch_applied, cmd.telemetry_verbose);
  }
  else if (strcmp(command, "stop") == 0) {
    cmd.cmd = MONITOR_CMD_STOP;
  }
  else if (strcmp(command, "status") == 0) {
    cmd.cmd = MONITOR_CMD_STATUS;
  }
  else if (strcmp(command, "pause") == 0) {
    cmd.cmd = MONITOR_CMD_PAUSE;
    printf(1, "Monitor: Pausing daemon monitoring\n");
  }
  else if (strcmp(command, "resume") == 0) {
    cmd.cmd = MONITOR_CMD_RESUME;
    printf(1, "Monitor: Resuming daemon monitoring\n");
  }
  else {
    printf(2, "Unknown command: %s\n", command);
    usage();
  }
  
  // Send command to kernel via syscall
  int result = monitor_control(&cmd);
  
  if (result < 0) {
    printf(2, "Monitor: syscall failed (error %d)\n", result);
    exit();
  }
  
  if (strcmp(command, "status") == 0) {
    printf(1, "Monitor: status active=%d duration=%d\n",
           cmd.active, cmd.duration_seconds);
  } else if (strcmp(command, "stop") == 0) {
    // Display summary after monitoring stops
    printf(1, "\n=== MONITORING SUMMARY ===\n");
    printf(1, "Duration: %d seconds\n", cmd.monitor_duration);
    printf(1, "Patches applied: %d\n", cmd.patches_at_stop - cmd.patches_at_start);
    printf(1, "Total patches: %d\n", cmd.patches_at_stop);
    if (cmd.syscalls_sampled > 0) {
      printf(1, "Syscalls sampled: %d\n", cmd.syscalls_sampled);
      printf(1, "Avg syscall time: %d µs\n", cmd.avg_syscall_time_us);
    }
    printf(1, "==========================\n\n");
  } else {
    printf(1, "Monitor: Command sent successfully\n");
  }
  exit();
}
