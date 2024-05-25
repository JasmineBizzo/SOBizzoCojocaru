#include "utils.h"
#include "../lib/ipc.c"

//#define SIZE 3 //POI da togliere?? 

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int * pid_atomo, * num_atomico;
int stat_id, msqid, ind; // atom_pid; //, atom_num_id;
int *shm_stat;// *shm_atom; //, *shm_atom_num;
int sem_pid_atomi, sem_writeShm, sem_att, sem_saveatom;
//pid_t pidAttivatore, pidAliment; 
struct msgbuff my_msg; //my_msg_createAtom, 

void read_in();
void add_atom();// aggiunge atomi quando riceve SIGUSR1
//void add_atom_alim(); 
int check_memory_size(); // controlla se ho spazio per salvare l'atomo
void choose_atom(); // sceglie atomo da scindere
void found_waste(); // trovato scoria
void endLocal();

/*
char * args_atomi[]  = {"atomo.c", NULL}; 
    char argv_buffer[50];    
    char ato[] = {"../src/atomo"}; */

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			endLocal();
		break;
		case SIGUSR1: /*salva l'atomo nella tabella */
			printf("\nATTIVATORE RICEVE SEGNALE \n");
			add_atom();
		break;
		case SIGUSR2:
			found_waste();
		default:
		break;
	}
}

void read_in(){
	const char *file_path = "../src/input.txt";
    char buffer[1024];
    char *token;
    FILE *file_descriptor = fopen(file_path, "r");

    if (!file_descriptor) {
        perror("Failed to open the file");
    }
    while (fgets(buffer, 1024, file_descriptor)) {
        token = strtok(buffer, " ");

        if (strcmp(token, "ENERGY_DEMAND") == 0){
            token = strtok(NULL, " ");
            ENERGY_DEMAND = atoi(token);
        }
        else if (strcmp(token, "N_ATOMI_INIT") == 0){
            token = strtok(NULL, " ");
            N_ATOMI_INIT = atoi(token);
        }
        else if (strcmp(token, "N_ATOM_MAX") == 0){
            token = strtok(NULL, " ");
            N_ATOM_MAX = atoi(token);
        }
        else if (strcmp(token, "MIN_N_ATOMICO") == 0){
            token = strtok(NULL, " ");
            MIN_N_ATOMICO = atoi(token);
        }
        else if (strcmp(token, "STEP_ATTIVATORE") == 0){
            token = strtok(NULL, " ");
            STEP_ATTIVATORE = atoi(token);
        }
        else if (strcmp(token, "STEP_ALIMENTAZIONE") == 0){
            token = strtok(NULL, " ");
            STEP_ALIMENTAZIONE = atoi(token);
        }
        else if (strcmp(token, "N_NUOVI_ATOMI") == 0){
            token = strtok(NULL, " ");
            N_NUOVI_ATOMI = atoi(token);
        }
        else if (strcmp(token, "SIM_DURATION") == 0){
            token = strtok(NULL, " ");
            SIM_DURATION = atoi(token);
        }
        else if (strcmp(token, "ENERGY_EXPLODE_THRESHOLD") == 0){
            token = strtok(NULL, " ");
            ENERGY_EXPLODE_THRESHOLD = atoi(token);
        }
    }
    fclose(file_descriptor);
}

void add_atom() {
	printf("SONO IN ADD ATOMO \n");
	//ind = shm_stat[11];
	int num_bytes =-1;
	if(!check_memory_size()){
		perror("Insufficient memory \n");
		return;
	}
	num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 0, 0); // coda con ATOMO

	if(num_bytes < 0){
		printf("Messaggio non arrivato ATTIVATORE - ADDATOMO, num_bytes = %d \n", num_bytes);
		//return;
	} else {
		printf("Messaggio arrivato ATTIVATORE - ADDATOMO, num_bytes = %d \n", num_bytes);
		//printf("CONTENUTO DELLA CODA att: %d, %d \n", my_msg.mpid, my_msg.mnum_atomico);
		printf("QUI 115 N ATOM MAX = %d \n", N_ATOM_MAX);
		}

	printf("Riga 117 attivatore\n");
	pid_atomo[ind] = my_msg.mpid;// getpid(); 

	printf("Riga 120 attivatore\n");
	if (my_msg.mnum_atomico == 0) {
		num_atomico[ind]=rand()%N_ATOM_MAX+1;
	} else {
		num_atomico[ind]= my_msg.mnum_atomico;
	}
	printf("HO SALVATO L'ATOMO CON PID: %d , NUMERO ATOMICO %d\n", my_msg.mpid, num_atomico[ind]);

	ind++;
	printf("INDICE NUM ATOMI: %d \n", ind);

	/*qui semaforo??*/
	shm_stat[11] += 1; //Aumento numero di atomi
	sem_release(sem_saveatom, 1); //metto a 1 
}

int check_memory_size() {
	ind = shm_stat[11];
	
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
	printf("MEMORIA INGRANDITA CON SUCCESSO\n");
		return 1;
}

void choose_atom() {
	printf("\nQUA ATT 223 \n");

	while(semctl(sem_writeShm, 0, GETVAL) != 1);
	printf("Attiv198 sem write : %i \n ", semctl(sem_writeShm, 0, GETVAL));
	sem_reserve(sem_writeShm, 0);
	printf("Attiv222 sem write dopo reserve: %i \n ", semctl(sem_writeShm, 0, GETVAL));
	int ind = shm_stat[11];
	int n = rand()%ind+0;

	// num_atomico[n] /= 2; //lo divido già per due 
	printf("PRIMA INVIO MESSAGGIO IN ATTIVATORE\n");
	 
	printf("PID ATOMO DA SCINDERE: %d \n", pid_atomo[n]);
	printf("N DA SCINDERE: %d \n", n);
	printf("IND: %d \n", ind);

	my_msg.mtype = 1;
	my_msg.mpid = pid_atomo[n];
	my_msg.mnum_atomico = num_atomico[n];

	printf("QUA ATT 232\n");
	msgsnd(msqid, &my_msg, sizeof(my_msg), 0); 
	printf("QUA ATT 246\n");

	
	shm_stat[8] = shm_stat[8] + 1; // aumento il contatore delle attivazioni in shm
	shm_stat[9] = shm_stat[9] + 1;
	
	sem_release(sem_writeShm, 0);
	
	printf("Attiv256 %i \n ", semctl(sem_writeShm, 0, GETVAL));
	kill(pid_atomo[n], SIGUSR1); // SEGNALE AD ATOMO PER AVVIARE LA SCISSIONE
	printf("\n");
}

void found_waste() {
	ind = shm_stat[11];

	int num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 0, 0);

	if(num_bytes < 0) {
		printf("Messaggio non arrivato ATTIVATORE - FOUND WASTE, num_bytes = %d \n", num_bytes);
		// return;
	} else {
	    while(semctl(sem_writeShm, 0, GETVAL) != 1);
		sem_setval(sem_writeShm, 0, 0);
		shm_stat[4] = shm_stat[4] + 1; // aumento il contatore di scorie in shm
		shm_stat[5] = shm_stat[5] + 1;
		for(int i = 0; i<ind; i++) {
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

void endLocal() {

    int i = shm_stat[11];
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

int main(int argc, char argv[]) {
	printf("SONO ENTRATO IN ATTIVATORE \n");
    //sem_reserve(sem_attivatore_pronto, 0);

	int t = 0; // secondi che passano
	ind = 0;
	srand(time(NULL));
	struct timespec req;
	struct sigaction sa;

	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	void read_in();
	printf("QUI 253 N ATOM MAX = %d \n", N_ATOM_MAX);

	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);		// collegare l'area a un puntatore

	//sprintf(argv_buffer, "%d", getppid());
    //args_atomi[1] = argv_buffer;

	pid_atomo = malloc(sizeof(int) * N_ATOMI_INIT);
	num_atomico = malloc(sizeof(int) * N_ATOMI_INIT);
	/*
	for (ind; ind < N_ATOMI_INIT; ind++) {
		pid_atomo[ind] = shm_atom[ind];
		num_atomico[ind] = rand()%(N_ATOM_MAX+1)+1;
	}
	//Non useremo più questa shm, quindi la chiudiamo
	shmdt(shm_atom);
	shmctl(atom_pid, IPC_RMID, NULL); */

	sem_pid_atomi = semget(getppid() + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);
	sem_att = semget(getppid() + 3 , 1, 0600 | IPC_CREAT);
	sem_saveatom = semget(getppid() + 4, 1, 0600 | IPC_CREAT);

	printf("creazione / individuazione coda di messaggi\n");
	msqid = msgget(getppid() + 1, IPC_CREAT | 0600); // CODA CON ATOMO 
	///IPC_EXCL | 
	//msqid_createAtom = msgget(getppid() + 2, IPC_CREAT | 0600); // CODA CON ALIMENTAZIONE

	int intervallo = STEP_ATTIVATORE;
	
	//sem_setval(sem_att, 0, 0);
	sem_reserve(sem_pid_atomi, 0);
	printf("IN ATTIVATORE DOPO RESERVE - SEMAFORO SEM_PID_ATOMI 2 = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
		
	/*Per iniziare il suo ciclo, l'Attivatore attentde che tutti gli atomi siano creati*/
	while(semctl(sem_pid_atomi, 0, GETVAL) != 0);
	//while(semctl(sem_att, 0, GETVAL) != 0);

	printf("ATTIVATORE CONTINUA. \n");
	
	/*Al secondo 0 ordino una prima scissione, poi entro nel while per ogni STEP_ATTIVATORE*/
		printf("QUI PRIMA DI RELEASE SEM ATT ATTIVATORE 237 %d\n", semctl(sem_att, 0, GETVAL));
		choose_atom();	
		t++;
		//sem_reserve(sem_att, 0);
		printf("QUI dopo DI RELEASE SEM ATT ATTIVATORE 241 %d\n",semctl(sem_att, 0, GETVAL) );

	
	//ipotesi = se alcuni processi partono a caso = flag nella condizione del while 
	while(t <= SIM_DURATION){ //&& 
		printf("ECCOMI attivatore ciclo while 246 \n");
		req.tv_sec = 1;
        req.tv_nsec = 0;
        nanosleep(&req, NULL);
		if(semctl(sem_att, 0, GETVAL) == 0){//CAMBIATO DA WHILE A IF
			printf("attivatore IN IF 251 \n");
			if(t == 0 || t == intervallo){ //togli t == 0
				choose_atom();
				intervallo += STEP_ATTIVATORE;
			}

		t++;
		//sem_reserve(sem_att, 0); /*master può iniziare la stampa*/ 
		}
	}
	free(pid_atomo);
	free(num_atomico);
	//msgctl(msqid, IPC_RMID, NULL);

	//return 0;
}
