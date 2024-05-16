#include "utils.h"

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

pid_t pidAttivatore, pidAliment; // process identificator dei process id

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
	shm_stat[12] = N_ATOMI_INIT + (N_NUOVI_ATOMI * (SIM_DURATION / STEP_ALIMENTAZIONE));

	atom_pid_id = shmget(getpid() + 2, sizeof(int) * shm_stat[12], 0600 | IPC_CREAT); /// size????
	shm_atom_pid = shmat(atom_pid_id, NULL, 0);

	atom_num_id = shmget(getpid() + 3, sizeof(int) * shm_stat[12], 0600 | IPC_CREAT); // size?
	shm_atom_num = shmat(atom_num_id, NULL, 0);


	//int my_pos = shm_atom_pid //posizione di questo processo in shm_atom_pid e _num
	if (1==1/*ricevo segnale dall'attivatore di scindermi*/) {
		int i, p, argv[1];
		int ultima_pos = shm_stat[11]; //ultima posizione occupata dell'array
		int pos_padre = atoi(argv[1]); //PROVO argv[1] è l'argomento che gli passo dal master con execve()
								//ovvero i, la sua posizione in shm_atom_pid e num (per trovare pid e num atom)

		//Per prima cosa controllo che non si crei una SCORIA
		if (shm_atom_num[pos_padre] <= MIN_N_ATOMICO) { 
			shm_atom_pid[pos_padre] = -1; //rimuovo questo processo dalla shm
			shm_atom_num[pos_padre] = -1;
			shm_stat[4] = shm_stat[4] +1; //aumento il contatore in shm
			shm_stat[5] = shm_stat[5] +1;
			kill(getpid(), SIGTERM); //termino questo processo
			// exit();
		}

		if (p = fork() == -1) { // se fork fallisce -> termina simulazione
			printf("MELTDOWN: ERRORE NEL FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
			kill(getpid(), SIGABRT);

		} else if (p = fork() == 0) {// children processes - atomo i
			// sops.sem_op = -1;
			// semop(s_id, &sops, 1);
			shm_atom_pid[ultima_pos+1] = getpid(); //salvo il pid del figlio appena creato
			
		// assegnazione del numero atomico al figlio
			if (shm_atom_num[pos_padre] % 2 == 0) { //se il num atomico del padre è pari
				shm_atom_num[pos_padre] /= 2; //dimezzo il num atomico del padre
				shm_atom_num[ultima_pos+1] = shm_atom_num[pos_padre]; //salvo quello del figlio
					
			} else { //se il num atomico del padre è dispari
				shm_atom_num[pos_padre] /= 2; //dimezzo il num atomico del padre
				shm_atom_num[ultima_pos+1] = shm_atom_num[pos_padre] + 1; //salvo quello del figlio (che sarà maggiore di 1)
				}
			/**Se num atom del padre = 20
			 * nuovo num atom padre -> 10 e num atom filgio ->10
			 * 
			 * Se num atom del padre = 21
			 * nuovo num atom padre -> 10 e num atom figlio -> 11
			*/

			//Incremento il counter dell'energia liberata ad ogni scissione
			//Posso sottrarre sempre il num atom del figlio perchè o sono uguali o questo è maggiore
			int energy_created = (shm_atom_num[pos_padre] * shm_atom_num[ultima_pos+1]) - shm_atom_num[ultima_pos+1];
			shm_stat[0] = shm_stat[0] + energy_created;
			shm_stat[1] = shm_stat[1] + energy_created; //DEVO capire come tener conto dei secondi che passano
			
			exit(0); //Lascioo???
		} 

	/*credo basti lo faccia il master
	shmdt(shm_stat);
	//shmctl(s_id, IPC_RMID, NULL); // deallocare la shared memory, viene cancellato dopo però
	shmdt(shm_data);
	//shmctl(m_id, IPC_RMID, NULL);*/

	//return 0; //??
	}
}
