#pragma once
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef long          int64;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef unsigned long addr_t;

typedef addr_t pde_t;
typedef addr_t pml4e_t;
typedef addr_t pdpe_t;

// Telemetry types
typedef uint64 timestamp_t;  // Microsecond timestamp

// Telemetry event sample
struct telemetry_sample {
  uchar event_type;       // 0=syscall, 1=process, 2=memory, 3=io, 4=anomaly
  ushort syscall_id;      // Syscall number (if event_type==0)
  int pid;                // Process ID
  uint cpu_id;            // CPU that recorded event
  timestamp_t timestamp;  // When event occurred
  uint duration_us;       // Duration in microseconds (for syscalls, I/O)
  uint64 value;           // Generic value (mem size, io bytes, etc)
  char data[16];          // Additional context (process name snippet, etc)
};

// Process behavior profile
struct process_profile {
  int pid;
  uint syscall_counts[32];  // Count per syscall (up to 32 syscalls)
  uint64 total_syscalls;
  uint64 memory_peak;
  uint context_switches;
  timestamp_t first_seen;
  timestamp_t last_seen;
};

// System-wide metrics
struct system_state {
  timestamp_t timestamp;
  uint running_processes;
  uint sleeping_processes;
  uint64 total_memory_used;
  uint64 free_memory;
  uint context_switches_total;
  uint syscalls_per_sec;
  uint io_ops_per_sec;
};

// Anomaly report
struct anomaly_report {
  uchar anomaly_type;      // 0=excessive_syscalls, 1=mem_spike, 2=io_stall, 3=crash_loop
  int pid;
  timestamp_t detected_at;
  uint severity;           // 1=low, 2=medium, 3=critical
  char description[32];
};

// Kernel patch structure for self-evolution
struct kpatch {
  uint id;                    // Unique patch ID
  uint64 target_addr;         // Function address to patch
  uint32 target_size;         // Original code size
  uint32 patch_size;          // New code size
  uchar original_code[1024];  // Saved original code
  uchar patch_code[1024];     // New code to inject
  uint32 original_checksum;   // CRC32 of original code before patch
  timestamp_t applied_at;     // When patch was applied
  timestamp_t created_at;     // When patch was created
  uchar status;               // Patch status
  int applied_by_pid;         // Which process applied this patch
};
