#include "ipc.h"
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

/*Sez Semafori*/

int errno;

int sem_setval(int sem_id, int sem_num, int sem_val){
    int ret_value;
    ret_value = semctl(sem_id, sem_num, SETVAL, sem_val);
    if(!errno) {
        return ret_value;
    } else {
        printf("Error in setting semaphore value, error: %s\n", strerror(errno));
    }
}

int sem_reserve(int sem_id, int sem_num){
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_flg = 0;
    sem_op.sem_op = -1;
    return semop(sem_id, &sem_op, 1);
}

int sem_release(int sem_id, int sem_num){
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_flg = 0;
    sem_op.sem_op = 1;
    return semop(sem_id, &sem_op, 1);
}
/*Fine Sez Semafori*/