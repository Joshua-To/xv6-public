#pragma once

#include "types.h"
#include "spinlock.h"

// Telemetry event types
#define TELEMETRY_SYSCALL   0
#define TELEMETRY_PROCESS   1
#define TELEMETRY_MEMORY    2
#define TELEMETRY_IO        3
#define TELEMETRY_ANOMALY   4

// Anomaly types
#define ANOMALY_EXCESSIVE_SYSCALLS  0
#define ANOMALY_MEMORY_SPIKE        1
#define ANOMALY_IO_STALL            2
#define ANOMALY_CRASH_LOOP          3

// Ring buffer for telemetry
struct telemetry_ringbuf {
  struct spinlock lock;
  struct telemetry_sample samples[TELEMETRY_BUFFER_SIZE];
  uint producer;      // Next write position
  uint consumer;      // Next read position
  uint64 total_samples; // Total samples ever recorded (for stats)
};

// Global telemetry state
extern struct telemetry_ringbuf telemetry_buf;
extern struct telemetry_ringbuf anomaly_buf;

// Initialize telemetry subsystem
void telemetry_init(void);

// Record telemetry events
void telemetry_record_syscall(int syscall_id, int pid, uint duration_us);
void telemetry_record_process_state(int pid, uchar new_state);
void telemetry_record_memory_event(int pid, uint64 size, uchar alloc_or_free);
void telemetry_record_io_event(int pid, uint io_bytes, uint duration_us);
void telemetry_record_anomaly(int pid, uchar anomaly_type, uint severity);

// Query telemetry
uint telemetry_get_sample_count(void);
int telemetry_read_samples(struct telemetry_sample *buf, uint max_count);
struct system_state telemetry_get_system_state(void);

// Utilities
timestamp_t telemetry_now(void);
