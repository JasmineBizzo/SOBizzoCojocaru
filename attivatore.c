#include "utils.h"
#include "../lib/ipc.c"

//#define SIZE 3 //POI da togliere?? 

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

	int * pid_atomo;
	int * num_atomico;
	//int max_index = SIZE-1; // massimo numero di elementi che si può mettere negli array
	int ind; // indice degli array
	int stat_id;
	int *shm_stat;

	int msqid, msqid_createAtom; // id coda di messaggi
	struct msgbuff my_msg, my_msg_createAtom;

//void endSimulation();

void handle_signal(); // handler
void add_atom();// aggiunge atomi quando riceve SIGUSR1
void add_atom_alim(); 
int check_memory_size(); // controlla se ho spazio per salvare l'atomo
void print(); // stampa per prova
void choose_atom(); // sceglie atomo da scindere
void found_waste(); // trovato scoria
void endLocal();

//Mi serve dichiararle globali, perchè verranno usate sia dal main sia dall'handler e endSimulation
int stat_id, atom_pid_id, atom_num_id;
int *shm_stat, *shm_atom_pid, *shm_atom_num;
int sem_pid_atomi, sem_writeShm, sem_att, sem_saveatom;
//pid_t pidAttivatore, pidAliment; 
/* SERVE??? */
char * args_atomi[]  = {"atomo.c", NULL}; 
    char argv_buffer[50];      /*!!!!!!!!*/
    char ato[] = {"../src/atomo"}; 

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			endLocal();
		break;
		case SIGUSR1: /*salva l'atomo nella tabella */
			printf("ATTIVATORE RICEVE SEGNALE \n");
			add_atom();
		break;
		case SIGUSR2:
			found_waste();
		default:
		break;
	}
}


//Ho pensato che si possa estrarre un numero atomico ad ogni STEP_ATTIVATORE. Ogni atomo con quel numero
//atomico riceve il segnale per effettuare la scissione
int main(int argc, char argv[]) {
	printf("SONO ENTRATO IN ATTIVATORE \n");
    //sem_reserve(sem_attivatore_pronto, 0);
	struct sigaction sa;
	int atom_pid;
	int *shm_atom;
	struct memory * memoriadim; /*!!!!!!!!*/

	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);		// collegare l'area a un puntatore
	
	//	atom_pid = shmget(PID_SHM + 15, sizeof(*memoriadim), 0600 | IPC_CREAT);
		/*QUI PID 15!!!!!*/
	atom_pid = shmget(getppid() + 15, sizeof(int) * N_ATOMI_INIT, 0600 | IPC_CREAT); /*creazione area di memoria condivisa */
	shm_atom = shmat(atom_pid, NULL, 0); /*collega l'area a un puntatore*/

	int t = 0; // secondi che passano 
	srand(time(NULL));
	ind = 0; 

	sprintf(argv_buffer, "%d", getppid());
    args_atomi[1] = argv_buffer;

	pid_atomo = malloc(sizeof(int) * N_ATOMI_INIT);
	num_atomico = malloc(sizeof(int) * N_ATOMI_INIT);
	/*
	for (ind; ind < N_ATOMI_INIT; ind++) {
		pid_atomo[ind] = shm_atom[ind];
		num_atomico[ind] = rand()%(N_ATOM_MAX+1)+1;
	}
	/*Non useremo più questa shm, quindi la chiudiamo
	shmdt(shm_atom);
	shmctl(atom_pid, IPC_RMID, NULL); */

	sem_pid_atomi = semget(getppid() +1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);
	sem_att = semget(getppid() + 3 , 1, 0600 | IPC_CREAT);
	sem_saveatom = semget(getppid() + 4, 1, 0600 | IPC_CREAT);

	
	// printf("ECCOMI\n");
	// creazione / individuazione coda di messaggi
	msqid = msgget(getppid() + 1, IPC_CREAT | IPC_EXCL | 0600); // CODA CON ATOMO PER SCISSIONE
	msqid_createAtom = msgget(getppid() + 2, IPC_CREAT | IPC_EXCL | 0600); // CODA CON ALIMENTAZIONE
	
	struct timespec req;
	// printf("ECCOMI 2\n");
   // sem_release(sem_attivatore_pronto, 0);

	//sem_reserve(sem_attivatore_start, 0);
	int intervallo = STEP_ATTIVATORE;
	// printf("ECCOMI 3\n");
	
	// int prova = semctl(sem_att, 0, GETVAL);
	//sem_setval(sem_att, 0, 0);
	sem_reserve(sem_pid_atomi, 0);
	printf("IN ATTIVATORE DOPO RESERVE - SEMAFORO SEM_PID_ATOMI = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
		
	while(semctl(sem_pid_atomi, 0, GETVAL) != 0);
	//while(semctl(sem_att, 0, GETVAL) != 0);

	printf("ATTIVATORE CONTINUA. \n");


   	//  printf("ATTIVATOREEEEEEE = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	
	//ipotesi = se alcuni processi partono a caso = flag nella condizione del while 
	while(t <= SIM_DURATION){ //&& 
			printf("ECCOMI 2\n");
		while(semctl(sem_att, 0, GETVAL) == 0); //quano è 1 va avanti
	
			if(t == 0 || t == intervallo){
				choose_atom();
				intervallo += STEP_ATTIVATORE;
			}

		sem_release(sem_att, 0);
		t++;
	}


	//free(pid_atomo);
	free(num_atomico);
	//msgctl(msqid, IPC_RMID, NULL);

	return 0;
}

void add_atom(){
	printf("SONO IN ADD ATOMO \n");
	int num_bytes;

	if(!check_memory_size()){
		perror("Insufficient memory \n");
		return;
	}

	num_bytes = msgrcv(msqid_createAtom, &my_msg_createAtom, sizeof(my_msg_createAtom), 0, 0); // coda con ATOMO

	if(num_bytes < 0){
		printf("Messaggio non arrivato ATTIVATORE - ADDATOMO, num_bytes = %d \n", num_bytes);
		return;
	} else {
		printf("Messaggio arrivato ATTIVATORE - ADDATOMO, num_bytes = %d \n", num_bytes);
	} 

	pid_atomo[ind] = my_msg_createAtom.mpid;// getpid(); 

	if (my_msg_createAtom.mnum_atomico == 0) {
		num_atomico[ind]=rand()%(N_ATOM_MAX+1)+1;
	} else {
		num_atomico[ind]= my_msg_createAtom.mnum_atomico;
	}
	printf("HO SALVATO L'ATOMO CON PID: %d \n", my_msg_createAtom.mpid);
	ind++;
	shm_stat[11] += 1; // incremento il numero di atomi presenti
	sem_setval(sem_saveatom, 0, 0); //metto a 0
}



int check_memory_size(){
	if(ind == sizeof(pid_atomo)) { //ind == max_index
		int dim = sizeof(pid_atomo)*2;
			printf("AUMENTO SPAZIO \n");
			pid_atomo = realloc(pid_atomo, sizeof(int) * dim);
			num_atomico = realloc(num_atomico, sizeof(int) * dim);

			if (pid_atomo == NULL || num_atomico == NULL) {
				perror("Insufficient memory");
				return 0;
			}
	} else
		return 1;
}

void choose_atom(){
	while(semctl(sem_writeShm, 0, GETVAL) != 1);
	printf("Attiv198 sem write : %i \n ", semctl(sem_writeShm, 0, GETVAL));
	sem_reserve(sem_writeShm, 0);
	printf("Attiv222 sem write dopo reserve: %i \n ", semctl(sem_writeShm, 0, GETVAL));
	ind = shm_stat[11];

	int n = rand()%ind+0;
	printf("QUA ATT 0\n");
	// num_atomico[n] /= 2; //lo divido già per due 
	printf("\n PRIMA INVIO MESSAGGIO IN ATTIVATORE\n");
	 
	printf("PID ATOMO DA SCINDERE: %d \n", pid_atomo[n]);
	printf("N DA SCINDERE: %d \n", n);
	printf("IND: %d \n", ind);

	my_msg.mtype=1;
	my_msg.mnum_atomico = num_atomico[n];
	my_msg.mpid = pid_atomo[n];
	printf("QUA ATT 1\n");
	msgsnd(msqid, &my_msg, sizeof(my_msg), 0); // NUM_ATOMICO DELL'ATOMO DA SCINDERE
	printf("QUA ATT 2\n");
	printf("\n");

	
	shm_stat[8] = shm_stat[8] + 1; // aumento il contatore delle attivazioni in shm
	shm_stat[9] = shm_stat[9] + 1;
	
	sem_release(sem_writeShm, 0);
	
	printf("%i Attiv212\n ", semctl(sem_writeShm, 0, GETVAL));
	kill(my_msg.mpid, SIGUSR1); // SEGNALE AD ATOMO PER AVVIARE LA SCISSIONE
}


void found_waste() {
	int num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 0, 0);

	if(num_bytes < 0) {
		printf("Messaggio non arrivato ATTIVATORE - FOUND WASTE, num_bytes = %d \n", num_bytes);
		// return;
	} else {
	    while(semctl(sem_writeShm, 0, GETVAL) != 1);
		sem_setval(sem_writeShm, 0, 0);
		shm_stat[4] = shm_stat[4] + 1; // aumento il contatore di scorie in shm
		shm_stat[5] = shm_stat[5] + 1;
		for(int i=0; i<ind; i++) {
			// cerco l'atomo da cancellare dalla tabella usando il pid
			if(pid_atomo[i] == my_msg.mpid) {
				kill(pid_atomo[i], SIGABRT); //termino questo processo
				pid_atomo[i] = -1;
				num_atomico[i] = -1;
				shm_stat[11] = shm_stat[11]-1 ; // decremento il numero di atomi presenti
		//NON so se serve riga sopra
			}
		}
		sem_release(sem_writeShm, 0);
		
	}
}

void endLocal(){
    int i = ind;
    if(pid_atomo[i]!= -1) { // DA GUARDARE
        for (i; i >= 0; i--) {  
            if(pid_atomo[i] != -1){
                kill(pid_atomo[i], SIGABRT);
            }
        }
    }


	//deoalloco memorie, semafori etc!!!!!

	exit(0);
}