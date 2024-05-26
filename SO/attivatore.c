#include "utils.h"
#include "../lib/ipc.c"

//#define SIZE 3 //POI da togliere?? 

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int * pid_atomo, * num_atomi_tot;
int stat_id, msqid, msqid_init, ind; // atom_pid; //, atom_num_id;
int *shm_stat;// *shm_atom; //, *shm_atom_num;
int sem_pid_atomi, sem_writeShm, sem_att, sem_saveatom;
//pid_t pidAttivatore, pidAliment; 
struct msgbuff my_msg; //my_msg_createAtom, 
int end_flag = 0; /*diventa 1 solo quando il processo può terminare*/

void read_in();
void add_atom();// aggiunge atomi quando riceve SIGUSR1
//void add_atom_init();
int check_memory_size(); // controlla se ho spazio per salvare l'atomo
void choose_atom(); // sceglie atomo da scindere
//void found_waste(); // trovato scoria
void endLocal();

/*
char * args_atomi[]  = {"atomo.c", NULL}; 
    char argv_buffer[50];    
    char ato[] = {"../src/atomo"}; */

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			end_flag = 1;
			endLocal();
		break;
		case SIGUSR1: /*salva l'atomo nella tabella */
			printf("\nATTIVATORE RICEVE SEGNALE 1 add atom\n");
			add_atom();
		break;
		/*case SIGUSR2: /*Lo useremo con atimo nuovi
			printf("\nATTIVATORE RICEVE SEGNALE 2 add atom INIT\n");
			add_atom_init();
		break; */
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

	/*Riceve i messaggi da msquid e salva i nuovi atomi*/
void add_atom() {
	printf("SONO IN ADD ATOMi \n");
	ind = num_atomi_tot[0];
	int num_bytes =-1;
	if(!check_memory_size()){
		perror("Insufficient memory \n");
		return;
	}

	for(int i = 1; i < 15; i++){/*DEVO SCORRERE*/
		memset(&my_msg, 0, sizeof my_msg); /*SERVE?*/
		/*NOWAIT a caso serve???*/
		if(msgrcv(msqid, &my_msg, sizeof(my_msg.mpid), 0, IPC_NOWAIT) != -1) { /*CODA NON VUOTA, LEGGO E SALVO I PID*/
			
			printf("Messaggio arrivato ATTIVATORE - ADDATOMO, num_bytes = %d \n", num_bytes);
		
			pid_atomo[ind] = my_msg.mpid;
	
			printf("HO SALVATO L'ATOMO NUOVO CON PID: %d \n", pid_atomo[ind]);
			ind++;
			num_atomi_tot[0] += 1; //Aumento numero di atomi
		} else {
			printf("Messaggio non arrivato ATTIVATORE /coda vuota - ADDATOMO, num_bytes = %d \n", num_bytes);
			i = 15; /*esco dal ciclo, non ci sono messaggi*/
		}
	}
}

/*void add_atom_init(){
	printf("SONO IN ADD ATOMO INIT \n");
	ind = num_atomi_tot[0];
	int num_bytes =-1;
	if(!check_memory_size()){
		perror("Insufficient memory \n");
		return;
	}
	num_bytes = msgrcv(msqid_init, &my_msg, sizeof(my_msg), 0, 0); //Coda con atomo o master

	if(num_bytes < 0){
		printf("Messaggio non arrivato ATTIVATORE - ADDATOMO, num_bytes = %d \n", num_bytes);
		//return;
	} else {
		printf("Messaggio arrivato ATTIVATORE - ADDATOMO, num_bytes = %d \n", num_bytes);
	}	
	pid_atomo[ind] = my_msg.mpid;
	/*L'atomo è nuovo, Genero numero atomico random
	num_atomico[ind] = rand()%N_ATOM_MAX+1;
	
	printf("HO SALVATO L'ATOMO NUOVO CON PID: %d , NUMERO ATOMICO %d\n", pid_atomo[ind], num_atomico[ind]);

	ind++;
	num_atomi_tot[0] += 1; //Aumento numero di atomi

	//if(ind <= N_ATOMI_INIT) //serve per atomi iniziali e non
	sem_release(sem_saveatom, 0); //metto a 1, così che atomo continui
}*/


int check_memory_size() { 
	ind = num_atomi_tot[0];
	
	if(ind == sizeof(pid_atomo)) {
		printf("AUMENTO SPAZIO \n");
		pid_atomo = realloc(pid_atomo, sizeof(int) * 2);

		if (pid_atomo == NULL) {
			perror("Insufficient memory");
			return 0;
		}
	} else
		return 1;
}

void choose_atom() {
	printf("\nCHOOSE ATOM att 180\n");
	//int num_atomico_child;
	int ind = num_atomi_tot[0];
	int n = rand()%(ind-1)+0;

	printf("\nPRIMA dell'INVIO MESSAGGIO IN ATTIVATORE\n");

	//my_msg.mtype = 1;
	//my_msg.mpid = pid_atomo[n]; /*Il proprio pid non gli serve e lo sa già
	//my_msg.mnum_atomico = num_atomico_child; /*Passo il num atomico del figlio della scissione già calcolato*/
	
	//printf("PID ATOMO DA SCINDERE: %d \n", pid_atomo[n]);
	//printf("NUM ATOMICO FIGLIO: %d \n", num_atomico_child);
	printf("N DA SCINDERE: %d \n", n);
	printf("IND: %d \n", ind);

	//printf("QUA ATT 178\n");
	//msgsnd(msqid, &my_msg, sizeof(my_msg), 0);
	kill(pid_atomo[n], SIGUSR1);// SEGNALE AD ATOMO PER AVVIARE LA SCISSIONE


	/**Poi incremento i counter nella shm */
			
	while(semctl(sem_writeShm, 0, GETVAL) != 1);	
	printf("Attiv240 sem write 1 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
		sem_reserve(sem_writeShm, 0);
	printf("Attiv245 sem write dopo reserve 0 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
			
		shm_stat[8] = shm_stat[8] + 1; /*aumento il contatore delle attivazioni in shm*/
		shm_stat[9] = shm_stat[9] + 1;
		//shm_stat[11] = shm_stat[11] + 1; /*SERVE???? aumento atomi totali*/

	printf("QUA ATTiv 230 ENERGIA SHM_STaT[0] %d\n", shm_stat[0]);
	sem_release(sem_writeShm, 0);
	printf("Attiv 232 writeShm 1 = %d \n ", semctl(sem_writeShm, 0, GETVAL));

	printf("\n");
}

/*void found_waste() {
	ind =  num_atomi_tot[0];

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
				//shm_stat[11] = shm_stat[11]-1 ; // decremento il numero di atomi presenti
		//NON so se serve riga sopra
			}
		}
		sem_release(sem_writeShm, 0);
		
	}
} */

void endLocal() {

    int i = num_atomi_tot[0];
    if(pid_atomo[i]!= -1) { // DA GUARDARE
        for (i; i >= 0; i--) {  
            if(pid_atomo[i] != -1){
                kill(pid_atomo[i], SIGABRT);
            }
        }
    }

	free(pid_atomo);
	//free(num_atomico);
	//msgctl(msqid, IPC_RMID, NULL);

	//return 0;
	//deoalloco memorie, semafori etc!!!!!

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

	read_in();
	printf("QUI 253 N ATOM MAX = %d \n", N_ATOM_MAX);

	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);		// collegare l'area a un puntatore

	//sprintf(argv_buffer, "%d", getppid());
    //args_atomi[1] = argv_buffer;

	pid_atomo = malloc(sizeof(int) * N_ATOMI_INIT);
	//num_atomico = malloc(sizeof(int) * N_ATOMI_INIT);
	num_atomi_tot = malloc(sizeof(int));
	/*Memoria locale che contiene il num totale di atomi creati
	La uso per non accedere alla shm in ogni metodo in cui uso ind*/

	num_atomi_tot[0] = 0;
	printf("Num atomi tot in Attivatore 269 %d \n",num_atomi_tot[0]);

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

	//msqid_init = msgget(getppid() + 2, IPC_CREAT | 0600);
	///IPC_EXCL | 
	//msqid_createAtom = msgget(getppid() + 2, IPC_CREAT | 0600); // CODA CON ALIMENTAZIONE

	int intervallo = STEP_ATTIVATORE;
	
	//sem_setval(sem_att, 0, 0);
	sem_release(sem_pid_atomi, 0);
	printf("IN ATTIVATORE DOPO RESERVE - SEMAFORO SEM_PID_ATOMI %i \n", semctl(sem_pid_atomi, 0, GETVAL));
		
	/*Per iniziare il suo ciclo, l'Attivatore attentde che tutti gli atomi siano creati*/
	while(semctl(sem_pid_atomi, 0, GETVAL) != (N_ATOMI_INIT + 2));
	//while(semctl(sem_att, 0, GETVAL) != 0);

	printf("ATTIVATORE CONTINUA. 338 fa choose atom \n");
	
	/**Al secondo 0 faccio 2scissioni, per introdurre dell'energia,
	 * poi entro nel while per ogni STEP_ATTIVATORE
	 * */
	for(int l = 0; l < 2; l++)
		choose_atom();	
		
	t++;
	sem_reserve(sem_att, 0); /*master può iniziare la stampa*/
	
	//ipotesi = se alcuni processi partono a caso = flag nella condizione del while 
	while(t <= SIM_DURATION && !end_flag) { 
		printf("ECCOMI attivatore ciclo while 384 \n");
		req.tv_sec = 1;
        req.tv_nsec = 0;
        nanosleep(&req, NULL);
		add_atom(); /*Ogni ciclo controllo se ci sono nuovi atomi da salvare*/
		if(semctl(sem_att, 0, GETVAL) == 0){//CAMBIATO DA WHILE A IF
			printf("attivatore IN IF 309 \n");
			if(t == 0 || t == intervallo){ //togli t == 0!!!
				choose_atom();
				intervallo += STEP_ATTIVATORE;
			}
		}
		t++;
		sem_reserve(sem_att, 0); /*master può iniziare la stampa*/ 
	}
}