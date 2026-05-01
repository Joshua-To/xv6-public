#pragma once
#include "types.h"
struct stat;
struct rtcdate;
struct monitor_control;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(uint64);
int sleep(int);
int uptime(void);
int telemetry_read(struct telemetry_sample*, int);
int telemetry_subscribe(void);
int patch_apply(struct kpatch*, int);
int patch_rollback(uint);
int monitor_control(struct monitor_control*);
int test_list_search(int);
int test_process_scan(void);
int test_trap_scan(void);

// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
