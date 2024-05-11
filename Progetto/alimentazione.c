#define _GNU_SOURCE // per signal handler

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

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

//void handle_signal();
//void check_in();
//void secPrint(int *, int);
void endSimulation();

//Mi serve dichiararle globali, perchè verranno usate sia dal main sia dall'handler e endSimulation
int stat_id, atom_pid_id, atom_num_id;
int *shm_stat, *shm_atom_pid, *shm_atom_num;
int sem_config_id;
pid_t pidAttivatore, pidAliment; 

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			endSimulation();
		break;
		default:
		break;
	}
}


int main() {
	srand(time(NULL)); //senza a ogni esecuzione del programma viene sempre estratto lo stesso numero
	
	char argv_buffer[50];
	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler

	stat_id = shmget(getpid() + 1, sizeof(int) * 13, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);									// collegare l'area a un puntatore
	shm_stat[12] = N_ATOMI_INIT + (N_NUOVI_ATOMI * (SIM_DURATION / STEP_ALIMENTAZIONE));

	atom_pid_id = shmget(getpid() + 2, sizeof(int) * shm_stat[12], 0600 | IPC_CREAT); /// size????
	shm_atom_pid = shmat(atom_pid_id, NULL, 0);

	atom_num_id = shmget(getpid() + 3, sizeof(int) * shm_stat[12], 0600 | IPC_CREAT); // size?
	shm_atom_num = shmat(atom_num_id, NULL, 0);


	int i = shm_stat[11]; //i parte dal numero di atomi creati fino a quel momento
	int new = shm_stat[11] + N_NUOVI_ATOMI; //ed arriva a questo
    for(i; i < new; i++){
       char * args_atomi[]  = {"atomo.c", NULL}; // array di stringhe terminante con NULL da passare in execve()
        switch (shm_atom_pid[i] = fork()){
            case -1:
                printf("MELTDOWN: ERRORE NEL FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
                kill(getpid(), SIGABRT);
                // printf("MELTDOWN: %s", strerror(errno));
                // exit(-1);
            case 0:
                shm_atom_pid[i] = getpid();
                shm_atom_num[i] = rand()%N_ATOM_MAX+1;
                
                sprintf(argv_buffer, "%d", (i)); // prima era i+1 provo a cambiare
                args_atomi[1] = argv_buffer;
                execve("../SO/atomo", args_atomi, NULL); 
                exit(0); 
                break;
            default:
            break;
        }
    }
    shm_stat[11] = i; //aggiorno il num di atomi totali
}
