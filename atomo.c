 #define _GNU_SOURCE // per signal handler

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>
#include <sys/msg.h>

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

void handle_signal();
//void check_in();
//void secPrint(int *, int);
void endSimulation();
void scissione();

//Mi serve dichiararle globali, perchè verranno usate sia dal main sia dall'handler e endSimulation
int stat_id, atom_pid_id, atom_num_id;
int *shm_stat, *shm_atom_pid, *shm_atom_num;
int sem_config_id;

pid_t pidAttivatore, pidAliment; // process identificator dei process id

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			endSimulation();
		break;
		case SIGUSR1: // scissione 
			scissione();
		default:
		break;
	}
}

int msqid; //id della coda

struct msgbuff {
	int mtype; //message type
	int mnum_atomico; //message data
	int mpid; 
} my_msg;


int main() {

	
	// creazione / individuazione coda di messaggi
	msqid = msgget(getppid()+4, IPC_CREAT | IPC_EXCL | 0600);


	srand(time(NULL));
	///////
	int sem_config_id; // qui passo lo stesso sem del master, ma non so ancora gestirlo
	int sem_reserve(int sem_id, int sem_num);

	sem_config_id = semget(getppid(), 1, 0600 | IPC_CREAT);
	sem_reserve(sem_config_id, 0);
	///////

	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler

	stat_id = shmget(getpid() + 1, sizeof(int) * 13, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);									// collegare l'area a un puntatore
	
	// shm_stat[11] = numero di atomi totali  
	// shm_stat[13] = pid del attivatore
    // shm_stat[14] = pid di atomo

/*
	

	/*credo basti lo faccia il master
	shmdt(shm_stat);
	//shmctl(s_id, IPC_RMID, NULL); // deallocare la shared memory, viene cancellato dopo però
	shmdt(shm_data);
	//shmctl(m_id, IPC_RMID, NULL);*/

	//return 0; //??
	
}

void scissione() {

	int num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 1, 0); //ricevo il messaggio

	if( num_bytes < 0){
		printf("Messaggio non arrivato, num_bytes = %d \n", num_bytes);
		// return;
	}
	int num_atomico = my_msg.mnum_atomico;

	//Per prima cosa controllo che non si crei una SCORIA
	if (num_atomico <= MIN_N_ATOMICO) { 
		shm_stat[4] = shm_stat[4] +1; //aumento il contatore in shm
		shm_stat[5] = shm_stat[5] +1;

		msgsnd(msqid, &my_msg, sizeof(my_msg), 0); //INVIO IL MESSAGGIO
		kill(shm_stat[13], SIGUSR2); 
		
		// kill(getpid(), SIGTERM); //termino questo processo
		// exit();
	} else if ( fork() == -1) { // se fork fallisce -> termina simulazione
			printf("MELTDOWN: ERRORE NEL FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
			kill(getpid(), SIGABRT);

	} else if (fork() == 0) {// children processes - atomo i
			// sops.sem_op = -1;
			// semop(s_id, &sops, 1);
			my_msg.mpid = getpid(); //scrivo nella queue il pid del figlio appena creato
			
			// assegnazione del numero atomico al figlio
			if (num_atomico % 2 == 0) { //se il num atomico del padre è pari
				num_atomico /= 2; //dimezzo il num atomico del padre
				my_msg.mnum_atomico = num_atomico; //salvo num atomico del filgio quello del figlio
					
			} else { //se il num atomico del padre è dispari
				num_atomico /= 2; //dimezzo il num atomico del padre
				my_msg.mnum_atomico = num_atomico + 1; //salvo quello del figlio (che sarà maggiore di 1)
				}
			/**Se num atom del padre = 20
			 * nuovo num atom padre -> 10 e num atom filgio ->10
			 * 
			 * Se num atom del padre = 21
			 * nuovo num atom padre -> 10 e num atom figlio -> 11
			*/

			//Incremento il counter dell'energia liberata ad ogni scissione
			//Posso sottrarre sempre il num atom del figlio perchè o sono uguali o questo è maggiore
			int energy_created = num_atomico*my_msg.mnum_atomico - my_msg.mnum_atomico;
			shm_stat[0] = shm_stat[0] + energy_created;
			shm_stat[1] = shm_stat[1] + energy_created; //DEVO capire come tener conto dei secondi che passano
			
			msgsnd(msqid, &my_msg, sizeof(my_msg), 0); //INVIO IL MESSAGGIO
			kill(shm_stat[14], SIGUSR1); // SEGNALE AD ATTIVATORE PER REGISTRARE L'ATOMO
			exit(0); //Lascioo???
		} 

}
