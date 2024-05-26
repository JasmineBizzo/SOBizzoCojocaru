#ifdef UTILS
#else
#define UTILS

#define _GNU_SOURCE

#include <signal.h> /*utilizzo delle funzioni per la gestione dei segnali fra processi*/
#include <errno.h> /*errno, variabile per indicare cosa e' andato storto*/
#include <stdlib.h>
#include <unistd.h> /*costanti e tipi simbolici vari e dichiara funzioni varie.*/
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
	int mtype;  /*message type*/
	int mpid;
} my_msg;

#endif