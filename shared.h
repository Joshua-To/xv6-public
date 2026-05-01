/*
 * Shared structures and constants between kernel and user space
 * Used for telemetry collection and kernel patching interface
 */

#ifndef SHARED_H
#define SHARED_H

#include "types.h"

// Telemetry constants (duplicated from telemetry.h for user-space availability)
#define TELEMETRY_SYSCALL      0
#define TELEMETRY_PROCESS      1
#define TELEMETRY_MEMORY       2
#define TELEMETRY_IO           3
#define TELEMETRY_ANOMALY      4

// Anomaly types (duplicated from telemetry.h for user-space availability)
#define ANOMALY_EXCESSIVE_SYSCALLS  0
#define ANOMALY_MEMORY_SPIKE        1
#define ANOMALY_IO_STALL            2
#define ANOMALY_CRASH_LOOP          3

// Patch status constants
#define PATCH_UNUSED   0
#define PATCH_VALID    1
#define PATCH_APPLIED  2
#define PATCH_REVERTED 3

// Patch error codes
#define PATCH_ERR_INVALID_ADDR      -1
#define PATCH_ERR_INVALID_CHECKSUM  -2
#define PATCH_ERR_INVALID_FORMAT    -3
#define PATCH_ERR_NO_SLOTS          -4

// Monitor control commands
#define MONITOR_CMD_START           1
#define MONITOR_CMD_STOP            2
#define MONITOR_CMD_STATUS          3
#define MONITOR_CMD_PAUSE           4
#define MONITOR_CMD_RESUME          5

// Monitor control structure
struct monitor_control {
  int cmd;                      // Command (START, STOP, etc)
  int duration_seconds;         // How long to monitor (0 = indefinite)
  int stop_if_patch_applied;    // Boolean: stop when patch is applied
  int active;                   // Boolean: is monitor currently active
  int telemetry_verbose;        // Boolean: show verbose telemetry (not just stats)
  uint32 start_time;            // Uptime when monitor started
  uint32 patches_at_start;      // Number of patches when started
  uint32 patches_at_stop;       // Number of patches when stopped (filled by kernel)
  uint32 monitor_duration;      // How long monitoring was active (seconds)
  uint32 syscalls_sampled;      // Total syscall samples during monitoring
  int avg_syscall_time_us;      // Average syscall time (microseconds)
};

#endif
