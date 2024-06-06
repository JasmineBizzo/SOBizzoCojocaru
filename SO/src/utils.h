#ifdef UTILS
#else
#define UTILS

#define _GNU_SOURCE

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/msg.h>

struct msgbuff {
	int mtype; 
	int mpid;
} my_msg;

#endif
