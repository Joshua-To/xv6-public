#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       1000  // size of file system in blocks

// Telemetry system parameters
#define TELEMETRY_BUFFER_SIZE  8192   // Max telemetry samples in ring buffer
#define TELEMETRY_SAMPLE_SIZE  64     // Bytes per sample (struct telemetry_sample)
#define MAX_PROCESS_PROFILES   128    // Max tracked process profiles
#define MAX_AI_PATCHES         16     // Max kernel patches that can be applied
#define ANOMALY_BUFFER_SIZE    256    // Max anomaly reports
#define TELEMETRY_FLUSH_TICKS  10     // Flush telemetry every N timer ticks

