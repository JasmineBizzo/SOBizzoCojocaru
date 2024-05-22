 #include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

void endSimulation();
void endLocal();
void choose_atom();

//Mi serve dichiararle globali, perchè verranno usate sia dal main sia dall'handler e endSimulation
int stat_id, atom_pid_id, atom_num_id;
int *shm_stat, *shm_atom_pid, *shm_atom_num;
int sem_pid_atomi, sem_writeShm;
pid_t pidAttivatore, pidAliment; 
struct msgbuff my_msg; 

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			endLocal();
		break;
		default:
		break;
	}
}


int main(int argc, char * argv[]) {
	
	int msqid;
	msqid = msgget(getppid()+4, IPC_CREAT | IPC_EXCL | 0600);
	struct timespec req;
	srand(time(NULL)); //senza a ogni esecuzione del programma viene sempre estratto lo stesso numero

	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler

	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);									// collegare l'area a un puntatore

    char ato[] = {"../src/atomo"}; 
	char * args_atomi[]  = {"atomo.c", NULL}; 
	char argv_buffer[50];
	// SEMAFORO O SEGNALE che parte ogni STEP ALIMENTAZIONE nano secondi!!

	sem_pid_atomi = semget(getppid() +1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);

	int t;
	
	printf("IN ALIMENTAZIONE - SEMAFORO = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	sem_reserve(sem_pid_atomi, 0);
	printf("IN ALIMENTAZIONE DOPO RESERVE - SEMAFORO = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	
	while(semctl(sem_pid_atomi, 0, GETVAL) != 0);

	printf("ALIMENTATORE CONTINUA. \n");

	while ((semctl(sem_pid_atomi, 0, GETVAL) == 0) && t <= SIM_DURATION){
	/*
		req.tv_sec = 0;
		req.tv_nsec = STEP_ALIMENTAZIONE;
		nanosleep(&req, NULL); */
			
		//int i = shm_stat[11]; //i parte dal numero di atomi creati fino a quel momento
		//int new = shm_stat[11] + N_NUOVI_ATOMI; //ed arriva a questo
    	for(int i = 0; i < N_NUOVI_ATOMI; i++) {
			switch (fork()) {
    	        case -1:
        	        printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI IN ALIMENTAZIONE\n");
            	    kill(getpid(), SIGABRT);
            	case 0:
					my_msg.mpid = getpid(); 
                	my_msg.mnum_atomico = rand()%((N_ATOM_MAX+1)+1);
                
        	        msgsnd(msqid, &my_msg, sizeof(my_msg), 0);
    	            kill(shm_stat[13], SIGUSR1);
                
                	execve(ato, args_atomi, NULL); 
                	exit(0);
        	        break;
        	    default:
            	break;
        	}
    }
    	t++;
	}
}

void endLocal(){
	//deoalloco memorie, semafori etc!!!!!

	exit(0);
}
