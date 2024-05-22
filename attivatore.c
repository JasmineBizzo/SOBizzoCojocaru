#include "utils.h"
#include "../lib/ipc.c"

//#define SIZE 3 //POI da togliere?? 

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

	int * pid_atomo;
	int * num_atomico;
	//int max_index = SIZE-1; // massimo numero di elementi che si può mettere negli array
	int ind = 0; // indice degli array
	int stat_id;
	int *shm_stat;

	int msqid; // id coda di messaggi
	struct msgbuff my_msg;

void endSimulation();

void handle_signal(); // handler
void add_atom(); // aggiunge atomi quando riceve SIGUSR1
int check_memory_size(); // controlla se ho spazio per salvare l'atomo
void print(); // stampa per prova
void choose_atom(); // sceglie atomo da scindere
void found_waste(); // trovato scoria
void endLocal();

//Mi serve dichiararle globali, perchè verranno usate sia dal main sia dall'handler e endSimulation
int stat_id, atom_pid_id, atom_num_id;
int *shm_stat, *shm_atom_pid, *shm_atom_num;
int sem_pid_atomi, sem_writeShm, sem_att;
pid_t pidAttivatore, pidAliment; 
    struct msgbuff; /* SERVE??? */

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			endLocal();
		break;
		case SIGUSR1: /*salva l'atomo nella tabella */
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
    //sem_reserve(sem_attivatore_pronto, 0);
	struct sigaction sa;
	int atom_pid;
	int *shm_atom;
	struct memory * memoriadim;

	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	atom_pid = shmget(getppid() + 15, sizeof(*memoriadim), 0600 | IPC_CREAT); /*creazione area di memoria condivisa */
	shm_atom = shmat(atom_pid, NULL, 0); /*collega l'area a un puntatore*/

	int t = 0; // secondi che passano 
	srand(time(NULL));

	pid_atomo = malloc(sizeof(int) * N_ATOMI_INIT);
	num_atomico = malloc(sizeof(int) * N_ATOMI_INIT);

	for (ind; ind < N_ATOMI_INIT; ind++) {
		pid_atomo[ind] = memoriadim->pid[ind];
		num_atomico[ind] = rand()%(N_ATOM_MAX+1)+1;
	}

	sem_pid_atomi = semget(getppid() +1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);
	sem_att = semget(getppid() +3, 1, 0600 | IPC_CREAT);

	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);		// collegare l'area a un puntatore
	
	printf("ECCOMI\n");
	// creazione / individuazione coda di messaggi
	msqid = msgget((getppid()+1), IPC_CREAT | IPC_EXCL | 0600);
	
	struct timespec req;
	printf("ECCOMI 2\n");
   // sem_release(sem_attivatore_pronto, 0);

	//sem_reserve(sem_attivatore_start, 0);
	int intervallo = STEP_ATTIVATORE;
	printf("ECCOMI 3\n");
	
	printf("CIao \n");
	// int prova = semctl(sem_att, 0, GETVAL);
	//sem_setval(sem_att, 0, 0);
	sem_reserve(sem_pid_atomi, 0);
	printf("IN ATTIVATORE DOPO RESERVE - SEMAFORO = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	while(semctl(sem_pid_atomi, 0, GETVAL) != 0);

	printf("ATTIVATORE CONTINUA. \n");

	int fedeindio = semctl(sem_pid_atomi, 0, GETVAL);
    printf("ATTIVATOREEEEEEE = %i \n", fedeindio);
	//while(semctl(sem_attivatore_start, 0, GETVAL) != 1);
/*NON VA UN CAZZO QUI*/
	printf("\n");	    
	while(fedeindio != 0); 
	printf("ATTIVATORE = %i", fedeindio);
//	if(fedeindio == 0) ){
			printf("ECCOMI 33\n");
	while (t <= SIM_DURATION){ //&& 
			printf("ECCOMI 2\n");
		if(t == intervallo){
			choose_atom(); // SERVE DOPO - lo commento ora perchè l'array è ancora vuoto e quindi da errore
			intervallo += STEP_ATTIVATORE;
		}
		/*req.tv_sec = 0;
		req.tv_nsec = STEP_ATTIVATORE;
		nanosleep(&req, NULL);*/

		sem_release(sem_att, 0);
		t++;
	}
//	}

	//free(pid_atomo);
	free(num_atomico);
	//msgctl(msqid, IPC_RMID, NULL);

	return 0;
}

void add_atom(){
	int num_bytes;

	if(!check_memory_size()){
		perror("Insufficient memory \n");
		return;
	}

	num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 1, 0);

	if(num_bytes < 0){
		printf("Messaggio non arrivato, num_bytes = %d \n", num_bytes);
		// return;
	}

	printf("Messaggio arrivato, num_bytes = %d \n", num_bytes);

	switch (fork()){
        case -1:
            printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE IN ATTIVATORE\n");
            //exit(-1); 
            kill(shm_stat[12], SIGABRT); /* INVIO UN SEGNALE AL MASTER E l'App termina*/
        case 0:
			pid_atomo[ind] = my_msg.mpid;// getpid(); 

			if (my_msg.mnum_atomico == 0) {
				num_atomico[ind]=rand()%(N_ATOM_MAX+1)+1;
			} else {
				num_atomico[ind]= my_msg.mnum_atomico;
				}

			exit(0); 
            break;
        default:
            break;
    }
	ind++;
	shm_stat[11] += 1; // incremento il numero di atomi presenti
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
        //while(semctl(sem_writeShm, 0, GETVAL) != 1){
	sem_setval(sem_writeShm, 0, 0);

	int n = rand()%ind+0;

	num_atomico[n] /= 2; //lo divido già per due 
	my_msg.mnum_atomico=num_atomico[n]; 
	my_msg.mpid = pid_atomo[n];
	msgsnd(msqid, &my_msg, sizeof(my_msg), 0); // NUM_ATOMICO DELL'ATOMO DA SCINDERE
	kill(shm_stat[14], SIGUSR1); // SEGNALE AD ATOMO PER AVVIARE LA SCISSIONE

	shm_stat[8] = shm_stat[8] + 1; // aumento il contatore delle attivazioni in shm
	shm_stat[9] = shm_stat[9] + 1;

	sem_release(sem_writeShm, 0);
		//}
}
/*
void print(){
	for (int i = 0; i<ind; i++) {
		printf("ID: %d \n", i);
		printf("PID: %d \n", pid_atomo[i]);
		printf("NUM_ATOMICO: %d \n", num_atomico[i]);
		printf("INDEX: %d \n", ind);
	//	printf("MAX_INDEX: %d \n", max_index);
		printf("\n");
	}
}*/

void found_waste() {
	int num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 1, 0);

	if(num_bytes < 0) {
		printf("Messaggio non arrivato, num_bytes = %d \n", num_bytes);
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
