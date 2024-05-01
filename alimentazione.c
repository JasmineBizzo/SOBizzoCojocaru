#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shared-memory.h"

int main() {
	srand(time(NULL)); //senza di questo a ogni esecuzione del programma viene sempre estratto lo stesso numero
	int i, m_id, s_id, N_NUOVI_ATOMI = (rand() % 5) + 1; //crea atomi da 1 a 5
	struct stat_data * shm_stat;
        struct shared_data * shm_data;

	s_id = shmget(getpid() + 1, sizeof(*shm_stat), 0600 | IPC_CREAT);
        shm_stat = shmat(s_id, NULL, 0);
        m_id = shmget(getpid() + 2, sizeof(*shm_data), 0600 | IPC_CREAT);
        shm_data = shmat(m_id, NULL, 0);

	shm_data->pid[0] = getpid();
	shm_data->id[0] = 0;
	shm_data->num_atomico[0] = 118;
	shm_data->j = 0;

	for(i = 1; i < N_NUOVI_ATOMI + 1; i++) {
		if(i >= 2) {
			shm_data->pid[shm_data->j+=1] = getpid();
			shm_data->id[shm_data->j] = shm_data->j;
                        shm_data->num_atomico[shm_data->j] = (rand() % 118) +1;
			printf("Ho creato una altro atomo!\n");
			printf("ID: %d \n",shm_data->id[shm_data->j]);
                        printf("PID: %d \n",shm_data->pid[shm_data->j]);
                        printf("NUM_ATOMICO: %d \n",shm_data->num_atomico[shm_data->j]);
                        printf("\n");
		}
		else {
			shm_data->pid[shm_data->j+=1] = getpid();
                        shm_data->id[shm_data->j] = shm_data->j;
                        shm_data->num_atomico[shm_data->j] = (rand() % 118) + 1;
			printf("Ho creato un atomo!\n");
			printf("ID: %d \n",shm_data->id[shm_data->j]);
                        printf("PID: %d \n",shm_data->pid[shm_data->j]);
                        printf("NUM_ATOMICO: %d \n",shm_data->num_atomico[shm_data->j]);
                        printf("\n");
		}
		sleep(2);
	}
	printf("In totale ho creato %d atomi\n", N_NUOVI_ATOMI);
}
