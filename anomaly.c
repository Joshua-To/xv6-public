#include "types.h"
#include "defs.h"
#include "param.h"
#include "proc.h"
#include "spinlock.h"
#include "telemetry.h"

// Simple anomaly detection heuristics
// These can be enhanced with ML models in future phases

// Detect if syscall is unusually slow
int
detect_slow_syscall(struct telemetry_sample *sample)
{
  if (sample->event_type != TELEMETRY_SYSCALL)
    return 0;
  
  // Threshold: syscall takes > 5000 microseconds (5ms)
  if (sample->duration_us > 5000) {
    return 1;  // Anomaly detected
  }
  
  return 0;
}

// Detect excessive syscalls from a process
int
detect_excessive_syscalls(int pid, uint32 syscall_count)
{
  // Threshold: > 1000 syscalls per sample period
  if (syscall_count > 1000) {
    return 1;
  }
  
  return 0;
}

// Detect memory spike
int
detect_memory_spike(struct telemetry_sample *sample)
{
  if (sample->event_type != TELEMETRY_MEMORY)
    return 0;
  
  // Threshold: single allocation > 512KB
  if (sample->value > (512 * 1024)) {
    return 1;
  }
  
  return 0;
}

// Detect I/O stall
int
detect_io_stall(struct telemetry_sample *sample)
{
  if (sample->event_type != TELEMETRY_IO)
    return 0;
  
  // Threshold: I/O takes > 10,000 microseconds (10ms)
  if (sample->duration_us > 10000) {
    return 1;
  }
  
  return 0;
}

// Analyze telemetry sample and check for anomalies
void
analyze_telemetry_sample(struct telemetry_sample *sample)
{
  if (!sample)
    return;
  
  int anomaly = 0;
  uchar anomaly_type = 0;
  
  switch (sample->event_type) {
  case TELEMETRY_SYSCALL:
    if (detect_slow_syscall(sample)) {
      anomaly = 1;
      anomaly_type = ANOMALY_EXCESSIVE_SYSCALLS;
    }
    break;
    
  case TELEMETRY_MEMORY:
    if (detect_memory_spike(sample)) {
      anomaly = 1;
      anomaly_type = ANOMALY_MEMORY_SPIKE;
    }
    break;
    
  case TELEMETRY_IO:
    if (detect_io_stall(sample)) {
      anomaly = 1;
      anomaly_type = ANOMALY_IO_STALL;
    }
    break;
    
  case TELEMETRY_PROCESS:
    // Could detect crash loops here
    break;
    
  default:
    break;
  }
  
  if (anomaly) {
    telemetry_record_anomaly(sample->pid, anomaly_type, 2);
    cprintf("anomaly: detected type %d for PID %d\n", anomaly_type, sample->pid);
  }
}

// Batch analyze multiple samples
void
analyze_telemetry_batch(struct telemetry_sample *samples, uint count)
{
  uint i;
  for (i = 0; i < count; i++) {
    analyze_telemetry_sample(&samples[i]);
  }
}

// Simple performance optimization opportunity detector
int
should_optimize_syscall(int syscall_id, uint32 avg_duration_us, uint32 call_count)
{
  // If syscall is called frequently and slowly, it's a candidate for optimization
  // Threshold: called > 100 times with avg duration > 1000us
  
  if (call_count > 100 && avg_duration_us > 1000) {
    cprintf("anomaly: syscall %d is candidate for optimization (calls=%d, avg=%d us)\n",
            syscall_id, call_count, avg_duration_us);
    return 1;
  }
  
  return 0;
}
