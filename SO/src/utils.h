#ifdef UTILS
#else
#define UTILS

#define _GNU_SOURCE

#include <signal.h> //utilizzo delle funzioni per la gestione dei segnali fra processi
#include <errno.h> //errno, variabile per indicare cosa e' andato storto
#include <stdlib.h>
#include <unistd.h> //costanti e tipi simbolici vari e dichiara funzioni varie.
#include <stdio.h>
#include <time.h>
#include <stdint.h> //SERVE???
#include <sys/types.h> //per pid_t
#include <sys/wait.h>
#include <sys/shm.h> //per shm
#include <sys/sem.h> //per sem
#include <string.h>
#include <sys/ipc.h>

//Vediamo se servirà
struct MsgOp{
    long type; /*= pid del porto se coda tra più porti, = 1 se coda per singolo porto*/
    int operation; /*Tipo di op da effettuare: -1 = no generico, 0 = richiesta e attracco, 1 = scarico, 2 = carico, 3 = info vita, 4 = leave*/
    int extra; /*Campo extra per valori possibili ma non sicuri (es key di ICP o lunghezza array)*/
    pid_t pid_nave; /*Campo contenente pid della nave a cui risponere*/
};
#endif
