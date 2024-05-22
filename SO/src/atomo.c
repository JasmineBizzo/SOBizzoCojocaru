 #include "utils.h"
#include "../lib/ipc.c"

//Cancella quelli non usati
int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

void endLocal();
void scissione();

	
//Mi serve dichiararle globali, perchè verranno usate sia dal main sia dall'handler e endSimulation
int stat_id, atom_pid_id, atom_num_id;
int *shm_stat, *shm_atom_pid, *shm_atom_num;
int sem_pid_atomi, sem_writeShm;
int msqid;
struct msgbuff my_msg;
//pid_t pidAttivatore, pidAliment; // process identificator dei process id
    // struct msgbuff; /* */

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			endLocal();
		break;
		case SIGUSR1:
			scissione();
		break;
		default:
		break;
	}
}

int main(int argc, char * argv[]) {printf("QUI 4 \n");
	
	int pipefd[2]; // 0 read, 1 write
    pipe(pipefd);

    char buf[100];
    int num_bytes;
    close(pipefd[1]); // chiusa scrittura
    num_bytes = read(pipefd[1], buf, sizeof(buf));
	close(pipefd[0]); //chiuse lettura

		/*FUNGE???*/
	stat_id = shmget(atoi(buf) + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);									// collegare l'area a un puntatore
	printf("QUI 6 \n");
	// int pid_master = shm_stat[12];

	msqid = msgget(shm_stat[12] + 1, IPC_CREAT | IPC_EXCL | 0600);

    printf("QUI 5 \n");
	srand(time(NULL));
	
	sem_pid_atomi = semget(shm_stat[12] + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(shm_stat[12] + 2, 1, 0600 | IPC_CREAT);

	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler
	sigaction(SIGUSR1, &sa, NULL);


	printf("IN ATOMO 54 - SEMAFORO = %i \n", semctl(sem_pid_atomi, 0, GETVAL));	
	sem_reserve(sem_pid_atomi, 0);

	printf("ATOMO CONTINUA \n");
}

void scissione() {
	int num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 1, 0); //ricevo il messaggio

	if(num_bytes < 0){
		printf("Messaggio non arrivato, num_bytes = %d \n", num_bytes);
		// return;
	}
	int num_atomico = my_msg.mnum_atomico;
	int pid_atomo = my_msg.mpid;

	//Per prima cosa controllo che non si crei una SCORIA
	if (num_atomico <= MIN_N_ATOMICO) {
		my_msg.mnum_atomico = num_atomico;
		my_msg.mpid = pid_atomo;
		msgsnd(msqid, &my_msg, sizeof(my_msg), 0);
		kill(shm_stat[13], SIGUSR2);
		exit(0); //termina il processo chiamante
	} 
		
	/*PIPE atomo genitore atomo figlio*/
	int pipefd[2];
    pipe(pipefd);

    char buf[100];
    // int num_bytes2;
    close(pipefd[0]); //chiudi lettura
    //num_bytes2 = 
	write(pipefd[1], shm_stat[12], sizeof(buf));
	close(pipefd[1]); // chiusa scrittura

	
	if (fork() == -1) { // se fork fallisce -> termina simulazione
		printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
        kill(shm_stat[12], SIGABRT); /* INVIO UN SEGNALE AL MASTER E l'App termina*/
	} else if (fork() == 0) {
		my_msg.mpid = getpid(); //scrivo nella queue il pid del figlio appena creato

 		char * args_atomi[]  = {"atomo.c", NULL}; 
    	char argv_buffer[50];      /*!!!!!!!!*/
    	char ato[] = {"../src/atomo"}; 

		// int pid_master = (atoi) argv[1];
 	// sprintf(argv_buffer, "%d", (getppid()));
   // 	args_atomi[1] = argv_buffer;
		execve(ato, args_atomi, NULL);
     	exit(0);
	}
	
		/**Assegnazione del num atomico al figlio
		 * Se num atom del padre = 20
		 * nuovo num atom padre -> 10 e num atom filgio ->10
		 * 
		 * Se num atom del padre = 21
		 * nuovo num atom padre -> 10 e num atom figlio -> 11
		*/	
		if (num_atomico % 2 == 0) { //se il num atomico del padre è pari
			num_atomico /= 2; //dimezzo il num atomico del padre
			my_msg.mnum_atomico = num_atomico; //salvo num atomico del filgio quello del figlio
					
		} else { //se il num atomico del padre è dispari
			num_atomico /= 2; //dimezzo il num atomico del padre
			my_msg.mnum_atomico = num_atomico + 1; //salvo quello del figlio (che sarà maggiore di 1)
		}

        while(semctl(sem_writeShm, 0, GETVAL) != 1);
		sem_reserve(sem_writeShm, 0);
		printf(" Sem atomo 104 %i\n ", semctl(sem_writeShm, 0, GETVAL));

			//Incremento il counter dell'energia liberata ad ogni scissione
			//Posso sottrarre sempre il num atom del figlio perchè o sono uguali o questo è maggiore
			int energy_created = (num_atomico * my_msg.mnum_atomico) - my_msg.mnum_atomico;
			shm_stat[0] = shm_stat[0] + energy_created; /* aumento il contatore dell'energia in shm */
			shm_stat[1] = shm_stat[1] + energy_created;
			shm_stat[10] = shm_stat[10] + energy_created;
			shm_stat[6] = shm_stat[6] + 1; /* aumento il contatore delle scissioni in shm */
			shm_stat[7] = shm_stat[7] + 1;

			sem_release(sem_writeShm, 0);
			printf(" Sem atomo 116 %i\n ", semctl(sem_writeShm, 0, GETVAL));

			

			msgsnd(msqid, &my_msg, sizeof(my_msg), 0); //INVIO IL MESSAGGIO
			kill(shm_stat[14], SIGUSR1); // SEGNALE AD ATTIVATORE PER REGISTRARE L'ATOMO
			exit(0);
	}
	

void endLocal(){ 
	//deoalloco memorie, semafori etc!!!!!
	//shmdt(shm_stat);
	//shmctl(stat_id, IPC_RMID, NULL);
	exit(0);
}