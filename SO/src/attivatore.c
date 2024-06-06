#include "utils.h"
#include "../lib/ipc.h"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO, STEP_ATTIVATORE, STEP_ALIMENTAZIONE;
int N_NUOVI_ATOMI, SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;
int stat_id, msqid, msqid_scorie, sem_init, sem_writeShm;
int end_flag = 0, ind = 0, size = 100;
int * pid_atomo, * shm_stat;
struct msgbuff my_msg, my_msg_scorie;

void handle_signal(int);
void read_in();
void double_memory_size();
void add_atom();
void choose_atom();
void found_waste();
void endLocal();

void handle_signal(int signal) {
	switch (signal) {
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

void read_in() {
	const char *file_path = "../src/input.txt";
	char buffer[1024];
	char *token;
	FILE *file_descriptor = fopen(file_path, "r");

	if (!file_descriptor) {
		printf("FALLIMENTO NELL'APERTURA DEL FILE INPUT.\n");
		kill(getppid(), SIGABRT);
	}

	while (fgets(buffer, 1024, file_descriptor)) {
		token = strtok(buffer, " ");

		if (strcmp(token, "ENERGY_DEMAND") == 0) {
			token = strtok(NULL, " ");
			ENERGY_DEMAND = atoi(token);
		}
		else if (strcmp(token, "N_ATOMI_INIT") == 0) {
			token = strtok(NULL, " ");
			N_ATOMI_INIT = atoi(token);
		}
		else if (strcmp(token, "N_ATOM_MAX") == 0) {
			token = strtok(NULL, " ");
			N_ATOM_MAX = atoi(token);
		}
		else if (strcmp(token, "MIN_N_ATOMICO") == 0) {
			token = strtok(NULL, " ");
			MIN_N_ATOMICO = atoi(token);
		}
		else if (strcmp(token, "STEP_ATTIVATORE") == 0) {
			token = strtok(NULL, " ");
			STEP_ATTIVATORE = atoi(token);
		}
		else if (strcmp(token, "STEP_ALIMENTAZIONE") == 0) {
			token = strtok(NULL, " ");
			STEP_ALIMENTAZIONE = atoi(token);
		}
		else if (strcmp(token, "N_NUOVI_ATOMI") == 0) {
			token = strtok(NULL, " ");
			N_NUOVI_ATOMI = atoi(token);
		}
		else if (strcmp(token, "SIM_DURATION") == 0) {
			token = strtok(NULL, " ");
			SIM_DURATION = atoi(token);
		}
		else if (strcmp(token, "ENERGY_EXPLODE_THRESHOLD") == 0) {
			token = strtok(NULL, " ");
			ENERGY_EXPLODE_THRESHOLD = atoi(token);
		}
	}
	fclose(file_descriptor);
}

/* Aumenta la dimensione di pid_atomo se questo è pieno */
void double_memory_size() {
	if(ind == size) {
		size = size * 2;
		int * ptr = realloc(pid_atomo, sizeof(int) * size);
		if (ptr != NULL) {
			pid_atomo = ptr;
			ptr = NULL;
		} else {
			printf("REALLOC FALLITA IN ATTIVATORE.\n");
			kill(getppid(), SIGABRT);
		}
	}
}

/* Riceve i messaggi da msqid e salva i nuovi atomi */
void add_atom() {
	for(int i = 0; i < 100; i++) {
		memset(&my_msg, 0, sizeof my_msg.mpid);
		if(msgrcv(msqid, &my_msg, sizeof(my_msg.mpid), 0, 0) != -1) {
			double_memory_size();
			ind++;
			pid_atomo[ind-1] = my_msg.mpid;
		} else {
			i = 100;
		}
	}
}

/* Sceglie atomi casuali a cui inviare il segnale di scissione */
void choose_atom() {
	if(ind > 0) {
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

/* Elimina le scorie dall'array pid_atomo, che contiene il pid di tutti gli atomi */
void found_waste() {
	for(int i = 0; i < ind; i++) {
		memset(&my_msg_scorie, 0, sizeof my_msg_scorie.mpid); 
		if(msgrcv(msqid_scorie, &my_msg_scorie, sizeof(my_msg_scorie.mpid), 0, 0) != -1) {
			for(int j = 0; j < ind; j++){
				if(pid_atomo[j] == my_msg_scorie.mpid)
					pid_atomo[j] = -1;
			}
		} else {
			i = ind;
		}
	}
}

void endLocal() {
	for (int i = 0; i < ind; i++) {
		if(pid_atomo[i] != -1) {
			kill(pid_atomo[i], SIGABRT);
		}
	}

	for(int i = 0; i < size*10; i++) {
		memset(&my_msg, 0, sizeof my_msg.mpid);
		if(msgrcv(msqid, &my_msg, sizeof(my_msg.mpid), 0, 0) != -1) {
			kill(my_msg.mpid, SIGABRT);
		} else {
			i = size*10;
		}
	}

	shmdt(shm_stat);

	semctl(sem_init, 0, IPC_RMID);
	semctl(sem_writeShm, 0, IPC_RMID);

	msgctl(msqid, IPC_RMID, NULL);
	msgctl(msqid_scorie, IPC_RMID, NULL);

	free(pid_atomo);
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

	read_in();

	stat_id = shmget(getppid() + 1, sizeof(int) * 6, 0600 | IPC_CREAT);
	shm_stat = shmat(stat_id, NULL, 0);

	/* Array dei pid di tutti gli atomi creati */
	pid_atomo = malloc(sizeof(int) * size);

	sem_init = semget(getppid() + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);

	msqid = msgget(getppid() + 1, IPC_CREAT | 0600);
	msqid_scorie = msgget(getppid() + 2, IPC_CREAT | 0600);

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
