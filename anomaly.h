#pragma once

#include "types.h"

// Anomaly detection functions

// Individual detectors
int detect_slow_syscall(struct telemetry_sample *sample);
int detect_excessive_syscalls(int pid, uint32 syscall_count);
int detect_memory_spike(struct telemetry_sample *sample);
int detect_io_stall(struct telemetry_sample *sample);

// Analysis functions
void analyze_telemetry_sample(struct telemetry_sample *sample);
void analyze_telemetry_batch(struct telemetry_sample *samples, uint count);
int should_optimize_syscall(int syscall_id, uint32 avg_duration_us, uint32 call_count);
