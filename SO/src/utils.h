#ifdef UTILS
#else
#define UTILS

#define _GNU_SOURCE

#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct msgbuff {
	int mtype;
	int mpid;
} my_msg;

#endif
