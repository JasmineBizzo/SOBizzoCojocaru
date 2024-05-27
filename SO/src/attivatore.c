#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int * pid_atomo, * shm_stat;
int stat_id, msqid, sem_init, sem_writeShm;
int end_flag = 0;
int ind = 0; /* Counter del numero di atomi creati*/
/*ORA LA POSIZIONE 0 DELL'ARRAY RIMANE VUOTA, PER IL RESTO POSSIAMO USARE IND INVECE DI IND-1
(VEDI ADD ATOM) PRIMA AUMENTIAMO IND E POI SALVIAMO*/
struct msgbuff my_msg; 

void read_in();
int check_memory_size();
void add_atom();
void choose_atom(); 
void endLocal();

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			end_flag = 1;
			add_atom();
			endLocal();
		break;
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

	/* Aumenta la dimensione di pid_atomo se questo è pieno */
int check_memory_size() {
	if(pid_atomo[ind] == sizeof(pid_atomo)) {
		pid_atomo = realloc(pid_atomo, sizeof(int) * 2);
		if (pid_atomo == NULL) {
			perror("Insufficient memory");
			return 0;
		}
	} else
		return 1;
}

	/* Riceve i messaggi da msqid e salva i nuovi atomi */
void add_atom() {
	if(!check_memory_size()){
		perror("Insufficient memory \n");
		return;
	}

	for(int i = 1; i <= 30; i++){ /***CERCA MODO PER SALVARE TUTTI QUELLI NELLA CODA*/
		memset(&my_msg, 0, sizeof my_msg.mpid); 
	/*NOWAIT a caso serve???*/
		if(msgrcv(msqid, &my_msg, sizeof(my_msg.mpid), 0, 0) != -1) {//IPC_NOWAIT
			ind++;
			pid_atomo[ind] = my_msg.mpid;  
			//printf("ATT 109 HO SALVATO L'ATOMO %d\n", pid_atomo[ind]);
		} else {
			i = 15; /* Esco dal ciclo, non ci sono messaggi*/
		}
	}
}

	/* Sceglie un atomo casuale a cui inviare il segnale di scissione */
void choose_atom() {
	if(ind > 0) {
		int n = rand()%ind+1;

		kill(pid_atomo[ind], SIGUSR1); /*ATOMI MORTI PER SCISSIONE!!!!*/

	/* Incremento il counter delle attivazioni in shm */
		sem_reserve(sem_writeShm, 0);
	//printf("Attiv135 sem write dopo reserve 0 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
		shm_stat[8] = shm_stat[8] + 1; 
		shm_stat[9] = shm_stat[9] + 1;
		sem_release(sem_writeShm, 0);
	//printf("Attiv 140 writeShm 1 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
	}
}

void endLocal() {
    for (int i = ind; i > 0; i--) {
        if(pid_atomo[i] != -1){
            kill(pid_atomo[i], SIGABRT);
			printf("ATT 137 HO TERMINATO L'ATOMO %d\n", pid_atomo[i] );
        }
    }
    
	/* Memoria condivisa */
	shmdt(shm_stat);
	shmctl(stat_id, IPC_RMID, NULL);

	/* Semafori */
	semctl(sem_init, 0, IPC_RMID);
	semctl(sem_writeShm, 0, IPC_RMID);

	/* Coda di messaggi */
	msgctl(msqid, IPC_RMID, NULL);

	free(pid_atomo);
	printf("ATTIVATORE MUORE \n");
	exit(0);
}

int main(int argc, char argv[]) {
	srand(time(NULL));
	struct timespec req;
	
	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGABRT, &sa, NULL);

	read_in();

	stat_id = shmget(getppid() + 1, sizeof(int) * 13, 0600 | IPC_CREAT);
	shm_stat = shmat(stat_id, NULL, 0);

	/* array dei pid di tutti gli atomi creati */
	pid_atomo = malloc(sizeof(int) * N_ATOMI_INIT);

	sem_init = semget(getppid() + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);

	msqid = msgget(getppid() + 1, IPC_CREAT | 0600);
	
	//add_atom(); /* Aspetto che gli atomi si creino con il while e poi lo faccio nel for*/

	sem_reserve(sem_init, 0);
	
	/* Per introdurre dell'energia, poi entro nel while per ogni STEP_ATTIVATORE */
	for(int l = 0; l < 2; l++) {
		add_atom();
		choose_atom();	
	}
	
	while(!end_flag) { 
		req.tv_sec = STEP_ATTIVATORE;
        req.tv_nsec = 0;
        nanosleep(&req, NULL);
		
		add_atom(); /* Ad ogni ciclo controllo se ci sono nuovi atomi da salvare*/
		choose_atom();
	}
}