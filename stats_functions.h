#include <stdio.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <string.h>
#include <utmp.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#define MAX_STR 1024
#define RETRY_COUNT 5

void reqUsers(int* pipe, int tdelay, int samples);
void reqCPU(int* pipe, int tdelay, int samples, int graphics, int sequential);
void reqMem(int* pipe, int tdelay, int samples, int graphics);