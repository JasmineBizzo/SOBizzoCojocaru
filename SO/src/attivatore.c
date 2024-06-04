#include "utils.h"
#include "../lib/ipc.h"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int * pid_atomo, * shm_stat;
int stat_id, msqid, msqid_scorie, sem_init, sem_writeShm, sem_atoms;
int end_flag = 0, ind = 0, size = 100;
struct msgbuff my_msg, my_msg_scorie; 

void read_in();
void check_memory_size();
void add_atom();
void choose_atom();
void found_waste();
void endLocal();

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			end_flag = 1;
		break;
		case SIGUSR1:
			found_waste();
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
void check_memory_size() {
	if(ind == size) {
		size = size * 2;
		int * ptr = realloc(pid_atomo, sizeof(int) * size);
		if (ptr != NULL) {
			pid_atomo = ptr;
			ptr = NULL;
		} else {
			printf("Realloc failed. \n");
			free(pid_atomo);
			exit(-1);
		}
	}
}

	/* Riceve i messaggi da msqid e salva i nuovi atomi */
void add_atom() {
	
//printf("QUI 3 \n");
	for(int i = 0; i < 100; i++){
		//sem_reserve(sem_atoms, 0);
		memset(&my_msg, 0, sizeof my_msg.mpid);
		if(msgrcv(msqid, &my_msg, sizeof(my_msg.mpid), 0, 0) != -1) {

			check_memory_size();
			ind++;
			pid_atomo[ind-1] = my_msg.mpid;
			
			//printf("ATT 109 HO SALVATO L'ATOMO %d\n", pid_atomo[ind-1]);

		} else {
			i = 100;
		}
		//sem_release(sem_atoms, 0);
	}
	
	//printf("QUI 4\n");
}

	/* Sceglie atomi casuali a cui inviare il segnale di scissione */
void choose_atom() {
	if(ind > 0) {
		int i = rand()%3+1; /* Ogni step attivatore si ordinano da 1 a 3 scissioni */
		for(int l = 0; l < i; l++){	
			sigset_t maskBlock;
			sigemptyset(&maskBlock);
			sigaddset(&maskBlock, SIGUSR1);
			sigprocmask(SIG_BLOCK, &maskBlock, NULL);

			int n = rand()%ind;
			sem_reserve(sem_writeShm, 0);
			shm_stat[1] = shm_stat[1] + 1; /* aumento le attivazioni */
			sem_release(sem_writeShm, 0);
			kill(pid_atomo[n], SIGUSR1);
			
			sigemptyset(&maskBlock);
			sigaddset(&maskBlock, SIGUSR1);
			sigprocmask(SIG_UNBLOCK, &maskBlock, NULL);
		}
	}
}

	/* Elimina le scorie dall'array che contiene il pid di tutti gli atomi*/
void found_waste() {
//printf("TROVATO SCORIE\n");
	for(int i = 0; i < ind; i++) {
		memset(&my_msg_scorie, 0, sizeof my_msg_scorie.mpid); 
		if(msgrcv(msqid_scorie, &my_msg_scorie, sizeof(my_msg_scorie), 0, 0) != 1) {
			for(int j = 0; j < ind; j++){
				if(pid_atomo[j] == my_msg_scorie.mpid)
					pid_atomo[j] = -1;
			}
		}
	}
}

void endLocal() {
    for (int i = 0; i < ind; i++) {
        if(pid_atomo[i] != -1) {
            kill(pid_atomo[i], SIGABRT);
			//sem_release(sem_writeShm, 0); /***PROVA*/
			//printf("IND %d , ATT 173 HO TERMINATO L'ATOMO %d\n",ind, pid_atomo[i]);
        }
    }

	//sem_reserve(sem_atoms, 0);
	for(int i = 0; i < size*10; i++) { 
		sem_release(sem_writeShm, 0); /***PROVA*/
		sem_release(sem_atoms, 0); /**PROVA*/

		memset(&my_msg, 0, sizeof my_msg.mpid);
		if(msgrcv(msqid, &my_msg, sizeof(my_msg.mpid), 0, 0) != -1) {
			kill(my_msg.mpid, SIGABRT);
			
		//printf("ATT 157 HO TERMINATO L'ATOMO %d\n", my_msg.mpid);
		} else
			i = size*10;
	}
    
	/* Memoria condivisa */
	shmdt(shm_stat);

	/* Semafori */
	semctl(sem_init, 0, IPC_RMID);
	semctl(sem_writeShm, 0, IPC_RMID);
	semctl(sem_atoms, 0, IPC_RMID);

	/* Coda di messaggi */
	msgctl(msqid, IPC_RMID, NULL);
	msgctl(msqid_scorie, IPC_RMID, NULL);

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
	sigaction(SIGUSR1, &sa, NULL);

	stat_id = shmget(getppid() + 1, sizeof(int) * 6, 0600 | IPC_CREAT);
	shm_stat = shmat(stat_id, NULL, 0);

	/* array dei pid di tutti gli atomi creati */
	pid_atomo = malloc(sizeof(int) * size);

	sem_init = semget(getppid() + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);


sem_atoms = semget(getppid() + 3, 1, 0600 | IPC_CREAT);


	msqid = msgget(getppid() + 1, IPC_CREAT | 0600);
	msqid_scorie = msgget(getppid() + 2, IPC_CREAT | 0600);
	
	read_in();
	add_atom();

	sem_reserve(sem_init, 0);
	
	/* Per introdurre dell'energia, poi entro nel while per ogni STEP_ATTIVATORE */
	choose_atom();

	while(!end_flag) { 
		req.tv_sec = STEP_ATTIVATORE;
        req.tv_nsec = 0;
        nanosleep(&req, NULL);
		
		add_atom();
		choose_atom();
	}

	endLocal();
}