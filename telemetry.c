#include "types.h"
#include "defs.h"
#include "param.h"
#include "proc.h"
#include "spinlock.h"
#include "telemetry.h"

// Global telemetry ring buffers
struct telemetry_ringbuf telemetry_buf;
struct telemetry_ringbuf anomaly_buf;

// Helper: Get microsecond timestamp from kernel ticks
// xv6 ticks are typically 10ms apart, so we convert to microseconds
timestamp_t
telemetry_now(void)
{
  extern uint ticks;
  // Each tick is typically 10ms = 10,000 microseconds
  return (timestamp_t)ticks * 10000;
}

// Initialize telemetry subsystem
void
telemetry_init(void)
{
  initlock(&telemetry_buf.lock, "telemetry");
  initlock(&anomaly_buf.lock, "anomaly");
  
  telemetry_buf.producer = 0;
  telemetry_buf.consumer = 0;
  telemetry_buf.total_samples = 0;
  
  anomaly_buf.producer = 0;
  anomaly_buf.consumer = 0;
  anomaly_buf.total_samples = 0;
}

// Add sample to ring buffer (must hold lock)
static void
telemetry_add_sample(struct telemetry_ringbuf *buf, struct telemetry_sample *sample)
{
  uint next_producer = (buf->producer + 1) % TELEMETRY_BUFFER_SIZE;
  
  // Overwrite oldest sample if buffer full
  if (next_producer == buf->consumer) {
    buf->consumer = (buf->consumer + 1) % TELEMETRY_BUFFER_SIZE;
  }
  
  buf->samples[buf->producer] = *sample;
  buf->producer = next_producer;
  buf->total_samples++;
}

// Record syscall timing
void
telemetry_record_syscall(int syscall_id, int pid, uint duration_us)
{
  struct telemetry_sample sample;
  
  sample.event_type = TELEMETRY_SYSCALL;
  sample.syscall_id = syscall_id & 0xFFFF;
  sample.pid = pid;
  sample.cpu_id = 0; // TODO: get actual CPU ID
  sample.timestamp = telemetry_now();
  sample.duration_us = duration_us;
  sample.value = 0;
  
  acquire(&telemetry_buf.lock);
  telemetry_add_sample(&telemetry_buf, &sample);
  release(&telemetry_buf.lock);
}

// Record process state change
void
telemetry_record_process_state(int pid, uchar new_state)
{
  struct telemetry_sample sample;
  
  sample.event_type = TELEMETRY_PROCESS;
  sample.pid = pid;
  sample.cpu_id = 0;
  sample.timestamp = telemetry_now();
  sample.syscall_id = new_state; // Reuse field for state type
  sample.duration_us = 0;
  sample.value = 0;
  
  acquire(&telemetry_buf.lock);
  telemetry_add_sample(&telemetry_buf, &sample);
  release(&telemetry_buf.lock);
}

// Record memory allocation/deallocation
void
telemetry_record_memory_event(int pid, uint64 size, uchar alloc_or_free)
{
  struct telemetry_sample sample;
  
  sample.event_type = TELEMETRY_MEMORY;
  sample.pid = pid;
  sample.cpu_id = 0;
  sample.timestamp = telemetry_now();
  sample.syscall_id = alloc_or_free; // 0=alloc, 1=free
  sample.value = size;
  sample.duration_us = 0;
  
  acquire(&telemetry_buf.lock);
  telemetry_add_sample(&telemetry_buf, &sample);
  release(&telemetry_buf.lock);
}

// Record I/O operation
void
telemetry_record_io_event(int pid, uint io_bytes, uint duration_us)
{
  struct telemetry_sample sample;
  
  sample.event_type = TELEMETRY_IO;
  sample.pid = pid;
  sample.cpu_id = 0;
  sample.timestamp = telemetry_now();
  sample.duration_us = duration_us;
  sample.value = io_bytes;
  
  acquire(&telemetry_buf.lock);
  telemetry_add_sample(&telemetry_buf, &sample);
  release(&telemetry_buf.lock);
}

// Record anomaly
void
telemetry_record_anomaly(int pid, uchar anomaly_type, uint severity)
{
  struct telemetry_sample sample;
  
  sample.event_type = TELEMETRY_ANOMALY;
  sample.pid = pid;
  sample.cpu_id = 0;
  sample.timestamp = telemetry_now();
  sample.syscall_id = anomaly_type;
  sample.value = severity;
  sample.duration_us = 0;
  
  acquire(&anomaly_buf.lock);
  telemetry_add_sample(&anomaly_buf, &sample);
  release(&anomaly_buf.lock);
}

// Get number of available samples
uint
telemetry_get_sample_count(void)
{
  uint count;
  acquire(&telemetry_buf.lock);
  if (telemetry_buf.producer >= telemetry_buf.consumer) {
    count = telemetry_buf.producer - telemetry_buf.consumer;
  } else {
    count = TELEMETRY_BUFFER_SIZE - telemetry_buf.consumer + telemetry_buf.producer;
  }
  release(&telemetry_buf.lock);
  return count;
}

// Read samples from buffer (non-destructive, updates consumer)
int
telemetry_read_samples(struct telemetry_sample *buf, uint max_count)
{
  uint count = 0;
  
  acquire(&telemetry_buf.lock);
  while (count < max_count && telemetry_buf.consumer != telemetry_buf.producer) {
    buf[count] = telemetry_buf.samples[telemetry_buf.consumer];
    telemetry_buf.consumer = (telemetry_buf.consumer + 1) % TELEMETRY_BUFFER_SIZE;
    count++;
  }
  release(&telemetry_buf.lock);
  
  return count;
}

// Get current system state snapshot
struct system_state
telemetry_get_system_state(void)
{
  struct system_state state;
  
  state.timestamp = telemetry_now();
  state.running_processes = 0;
  state.sleeping_processes = 0;
  state.total_memory_used = 0;
  state.free_memory = 0;
  state.context_switches_total = 0;
  state.syscalls_per_sec = 0;
  state.io_ops_per_sec = 0;
  
  return state;
}
