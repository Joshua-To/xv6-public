/*
 * AI Daemon - User-space process for self-evolving xv6
 * 
 * This daemon:
 * 1. Reads telemetry from kernel via syscall
 * 2. Analyzes patterns and detects anomalies
 * 3. Generates optimization patches
 * 4. Applies patches to kernel code
 * 5. Monitors effectiveness and rolls back if needed
 */

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "shared.h"

#define daemon_printf(fd, ...) do { if (telemetry_verbose_enabled()) printf(fd, __VA_ARGS__); } while(0)
#define daemon_stats_printf(fd, ...) do { if (monitor_enabled()) printf(fd, __VA_ARGS__); } while(0)

int
monitor_enabled(void)
{
  struct monitor_control cmd;
  cmd.cmd = MONITOR_CMD_STATUS;
  cmd.duration_seconds = 0;
  cmd.stop_if_patch_applied = 0;
  cmd.telemetry_verbose = 0;
  cmd.active = 0;
  cmd.start_time = 0;
  cmd.patches_at_start = 0;
  int active = monitor_control(&cmd);
  return active == 1;
}

int
telemetry_verbose_enabled(void)
{
  struct monitor_control cmd;
  cmd.cmd = MONITOR_CMD_STATUS;
  cmd.duration_seconds = 0;
  cmd.stop_if_patch_applied = 0;
  cmd.telemetry_verbose = 0;
  cmd.active = 0;
  cmd.start_time = 0;
  cmd.patches_at_start = 0;
  monitor_control(&cmd);
  return cmd.telemetry_verbose == 1;
}

// Maximum number of samples to analyze per iteration
#define TELEMETRY_BATCH_SIZE 32  // Reduced from 128 to fit on stack cleanly (2KB instead of 8KB)

// Patch generation thresholds
#define SLOW_SYSCALL_THRESHOLD  2000      // microseconds (lowered from 5000)
#define EXCESSIVE_CALLS_THRESHOLD 500    // calls per period (lowered from 1000)
#define MEMORY_SPIKE_THRESHOLD  (256*1024) // bytes (lowered from 512KB)
#define IO_STALL_THRESHOLD      5000     // microseconds (lowered from 10000)

// Patch evolution constants
#define PATCHES_PER_EPOCH       2         // Max patches per analysis cycle (lowered from 4)
#define MIN_INTERVAL_BETWEEN_PATCHES 20  // iterations (lowered from 100)
#define ROLLBACK_THRESHOLD      -5        // % performance degradation before rollback

// Global state
uint64 last_patch_time = 0;
uint patch_count = 0;
uint rollback_count = 0;
struct kpatch generated_patches[16];
uint generated_patch_count = 0;

/*
 * Simple pattern analyzer that looks for repeating optimization patterns
 * Returns 1 if pattern detected, 0 otherwise
 */
int
detect_optimization_pattern(struct telemetry_sample *samples, uint count)
{
  if (count < 10)
    return 0;
  
  // Count slow syscalls
  uint slow_syscall_count = 0;
  uint excessive_io_count = 0;
  uint memory_spike_count = 0;
  
  uint i;
  for (i = 0; i < count; i++) {
    if (samples[i].event_type == TELEMETRY_SYSCALL && 
        samples[i].duration_us > SLOW_SYSCALL_THRESHOLD) {
      slow_syscall_count++;
    }
    else if (samples[i].event_type == TELEMETRY_IO && 
             samples[i].duration_us > IO_STALL_THRESHOLD) {
      excessive_io_count++;
    }
    else if (samples[i].event_type == TELEMETRY_MEMORY &&
             samples[i].value > MEMORY_SPIKE_THRESHOLD) {
      memory_spike_count++;
    }
  }
  
  // Pattern threshold: at least 20% of samples show anomaly (lowered from 30% for easier detection)
  uint anomaly_count = slow_syscall_count + excessive_io_count + memory_spike_count;
  if (anomaly_count > (count / 5)) {
    return 1;
  }
  
  return 0;
}

/*
 * Generate a simple optimization patch based on detected anomalies
 * This is a placeholder that generates mock patches for demonstration
 * In a real system, this would use ML models to generate actual code patches
 */
int
generate_optimization_patch(struct telemetry_sample *samples, uint count,
                            struct kpatch *patch)
{
  if (!patch || generated_patch_count >= 16)
    return -1;
  
  // Simple heuristic: if we detect slow syscalls, generate a generic optimization patch
  uint i;
  uint slow_syscall_count = 0;
  uint slow_syscall_id = 0;
  uint slow_syscall_duration = 0;
  
  for (i = 0; i < count; i++) {
    if (samples[i].event_type == TELEMETRY_SYSCALL) {
      if (samples[i].duration_us > slow_syscall_duration) {
        slow_syscall_duration = samples[i].duration_us;
        slow_syscall_id = samples[i].syscall_id;
      }
      if (samples[i].duration_us > SLOW_SYSCALL_THRESHOLD) {
        slow_syscall_count++;
      }
    }
  }
  
  // Generate patch only if we found a clear slow syscall pattern
  if (slow_syscall_count > 0 && slow_syscall_id > 0) {
    patch->id = generated_patch_count;
    patch->target_addr = 0xFFFFFF8000000000 + (slow_syscall_id * 0x100);
    patch->target_size = 16;
    patch->patch_size = 16;
    patch->created_at = uptime();
    patch->status = PATCH_VALID;
    
    // Simple mock patch: NOPs with a marker
    uint j;
    for (j = 0; j < 16; j++) {
      patch->patch_code[j] = 0x90;  // x86 NOP
    }
    
    generated_patch_count++;
    return patch->id;
  }
  
  return -1;
}

/*
 * Evaluate patch effectiveness by comparing telemetry before/after
 * Returns 1 if patch improved performance, 0 if degraded or neutral
 */
int
evaluate_patch_effectiveness(struct telemetry_sample *before, uint before_count,
                             struct telemetry_sample *after, uint after_count)
{
  if (before_count == 0 || after_count == 0)
    return 1;  // Can't evaluate, assume OK
  
  // Calculate average syscall duration before and after
  uint64 before_total = 0, after_total = 0;
  uint before_syscalls = 0, after_syscalls = 0;
  
  uint i;
  for (i = 0; i < before_count; i++) {
    if (before[i].event_type == TELEMETRY_SYSCALL) {
      before_total += before[i].duration_us;
      before_syscalls++;
    }
  }
  
  for (i = 0; i < after_count; i++) {
    if (after[i].event_type == TELEMETRY_SYSCALL) {
      after_total += after[i].duration_us;
      after_syscalls++;
    }
  }
  
  if (before_syscalls == 0 || after_syscalls == 0)
    return 1;
  
  uint64 before_avg = before_total / before_syscalls;
  uint64 after_avg = after_total / after_syscalls;
  
  // If performance improved (lower avg duration), patch is good
  if (after_avg < before_avg) {
    return 1;
  }
  
  return 0;
}

/*
 * Main AI daemon event loop - Full implementation with evolution
 * Continuously:
 * 1. Reads telemetry from kernel
 * 2. Detects performance anomalies
 * 3. Generates optimization patches
 * 4. Applies patches and monitors effectiveness
 * 5. Rolls back if degradation detected
 */
static struct telemetry_sample telemetry_samples[TELEMETRY_BATCH_SIZE];
static struct telemetry_sample before_patch_samples[TELEMETRY_BATCH_SIZE];

void
ai_daemon_loop(void)
{
  uint iteration = 0;
  uint last_patch_iteration = 0;
  uint before_patch_count = 0;
  
  daemon_stats_printf(1, "XV6 AI Daemon: Autonomous Evolution Mode ENABLED\n");
  daemon_stats_printf(1, "AI Daemon: Type 'monitor start <seconds> [patch]' to enable monitoring\n");
  daemon_stats_printf(1, "AI Daemon: Waiting for system load to generate telemetry...\n\n");
  
  while (1) {
    iteration++;
    
    // Use monitor activation to gate output and analysis
    int monitoring_active = monitor_enabled();
    
    if (monitoring_active) {
      // AI daemon runs autonomously, analyzing telemetry and applying patches
      
      // Read telemetry from kernel
      int sample_count = telemetry_read(telemetry_samples, TELEMETRY_BATCH_SIZE);
      
      if (sample_count > 0) {
        // Periodically analyze telemetry for patterns
        if (iteration % 3 == 0) {  // Check every 3 seconds
          daemon_printf(1, "AI Daemon: [TELEMETRY] Read %d samples, analyzing...\n", sample_count);
          
          // Detect optimization patterns
          if (detect_optimization_pattern(telemetry_samples, sample_count)) {
            daemon_printf(1, "AI Daemon: [PATTERN DETECTED] Anomalies found in telemetry\n");
          
          // Check interval throttling (don't patch too frequently)
          if ((iteration - last_patch_iteration) >= MIN_INTERVAL_BETWEEN_PATCHES) {
            daemon_printf(1, "AI Daemon: [PATCH ELIGIBLE] Interval check passed (%d iterations since last patch)\n",
                   iteration - last_patch_iteration);
            
            // Save telemetry snapshot before patch
            uint i;
            for (i = 0; i < sample_count && i < TELEMETRY_BATCH_SIZE; i++) {
              before_patch_samples[i] = telemetry_samples[i];
            }
            before_patch_count = sample_count;
            
            // Generate optimization patch
            struct kpatch new_patch;
            if (generate_optimization_patch(telemetry_samples, sample_count, &new_patch) >= 0) {
              daemon_printf(1, "AI Daemon: [PATCH GENERATED] Patch %d generated (target: 0x%x, size: %d)\n",
                     new_patch.id, new_patch.target_addr, new_patch.patch_size);
              
              // Apply the patch via syscall
              int patch_id = patch_apply(&new_patch, sizeof(struct kpatch));
              if (patch_id >= 0) {
                patch_count++;
                daemon_printf(1, "AI Daemon: [SUCCESS] Patch %d applied successfully!\n", patch_id);
                daemon_printf(1, "AI Daemon: [MONITOR] Monitoring patch effectiveness...\n");
                
                // Wait a bit and read new telemetry to evaluate effectiveness
                sleep(2);
                
                // Read new telemetry after patch
                int after_count = telemetry_read(telemetry_samples, TELEMETRY_BATCH_SIZE);
                if (after_count > 0) {
                  // Evaluate patch effectiveness
                  int effective = evaluate_patch_effectiveness(before_patch_samples, before_patch_count,
                                                              telemetry_samples, after_count);
                  
                  if (effective) {
                    daemon_printf(1, "AI Daemon: [EFFECTIVE] Patch %d improved performance!\n", patch_id);
                  } else {
                    daemon_printf(1, "AI Daemon: [INEFFECTIVE] Patch %d did not improve performance, rolling back...\n", patch_id);
                    if (patch_rollback(patch_id) == 0) {
                      daemon_printf(1, "AI Daemon: [ROLLBACK SUCCESS] Patch %d rolled back\n", patch_id);
                      rollback_count++;
                    }
                  }
                }
                
                last_patch_iteration = iteration;
              } else {
                daemon_printf(1, "AI Daemon: [ERROR] Failed to apply patch %d\n", new_patch.id);
              }
            } else {
              daemon_printf(1, "AI Daemon: [NO PATCH] Could not generate patch for current anomalies\n");
            }
          } else {
            daemon_printf(1, "AI Daemon: [RATE LIMITED] Waiting %d more iterations before next patch\n",
                   MIN_INTERVAL_BETWEEN_PATCHES - (iteration - last_patch_iteration));
          }
        }
        }
        
        // Print system statistics
        daemon_stats_printf(1, "AI Daemon: [STATS] Iteration: %d, Patches applied: %d, Rollbacks: %d\n",
               iteration, patch_count, rollback_count);
      }  // End if (sample_count > 0)
    }  // End if (monitoring_active)
    else if (iteration % 10 == 0) {
      // Print periodic heartbeat when no monitoring
      daemon_printf(1, "AI Daemon: [HEARTBEAT] %d iterations, %d patches applied, waiting for load...\n", 
             iteration, patch_count);
    }
    
    // Sleep briefly to allow other processes to run
    sleep(1);
  }
}

/*
 * Main entry point for AI daemon
 */
int
main(void)
{
  daemon_stats_printf(1, "XV6 AI Daemon: Initializing self-evolving kernel\n");
  daemon_stats_printf(1, "AI Daemon: Configuration:\n");
  daemon_stats_printf(1, "  - Slow syscall threshold: %d us\n", SLOW_SYSCALL_THRESHOLD);
  daemon_stats_printf(1, "  - Excessive calls threshold: %d\n", EXCESSIVE_CALLS_THRESHOLD);
  daemon_stats_printf(1, "  - Memory spike threshold: %d KB\n", MEMORY_SPIKE_THRESHOLD / 1024);
  daemon_stats_printf(1, "  - I/O stall threshold: %d us\n", IO_STALL_THRESHOLD);
  daemon_stats_printf(1, "AI Daemon: Starting autonomous optimization loop\n");
  
  // Run the main event loop
  ai_daemon_loop();
  
  exit();
}

