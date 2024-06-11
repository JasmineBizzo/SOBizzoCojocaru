#include "utils.h"
#include "../lib/ipc.h"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO, STEP_ATTIVATORE, STEP_ALIMENTAZIONE;
int N_NUOVI_ATOMI, SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;
int stat_id, msqid, msqid_scorie, pid_master, my_num_atom, sem_init, sem_writeShm;
int end_flag = 0;
int * shm_stat;
struct msgbuff my_msg, my_msg_scorie;

void handle_signal(int);
void read_in();
void scissione();
void endLocal();

void handle_signal(int signal) {
	switch (signal) {
		case SIGABRT:
			end_flag = 1;
			break;
		case SIGUSR1:
			scissione();
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
	exit(-1);
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

/* Controllo scorie, divisione del proprio numero atomico e creazione atomo figlio */
void scissione() {
	sigset_t maskBlock;
	sigemptyset(&maskBlock);
	sigaddset(&maskBlock, SIGUSR1);
	sigprocmask(SIG_BLOCK, &maskBlock, NULL);

	/* Controllo non si crei una SCORIA, se il num atomico è troppo basso termino il processo */
	if (my_num_atom <= MIN_N_ATOMICO) {
		sem_reserve(sem_writeShm, 0);
		shm_stat[5] = shm_stat[5] + 1;	/* aumento le scorie */
		sem_release(sem_writeShm, 0);

		my_msg_scorie.mtype = 1;
		my_msg_scorie.mpid = getpid();	
		msgsnd(msqid_scorie, &my_msg_scorie, sizeof(my_msg_scorie.mpid), 0);

		kill(shm_stat[0], SIGUSR1);
		endLocal();
	}

	/* Calcolo del numero atomico del figlio */
	int num_atomico_child;	
	if (my_num_atom % 2 == 0) { 
		my_num_atom /= 2; 
		num_atomico_child = my_num_atom; 	
	} else { 
		my_num_atom /= 2;
		num_atomico_child = my_num_atom + 1;
	}

	/* Creo il figlio della scissione */
	char sprintf_buffer[100];
	char atom[] = {"../bin/atomo"};
	char *args_atomi[]  = {"atomo", NULL, NULL, NULL};
	switch (fork()) {
		case -1:
			printf("MELTDOWN: ERRORE NELLA FORK PER LA SCISSIONE DEGLI ATOMI\n");
			kill(pid_master, SIGABRT);
			break;
		case 0: 
			sprintf(sprintf_buffer, "%d", pid_master);
			args_atomi[1] = strdup (sprintf_buffer);
			sprintf(sprintf_buffer, "%d", num_atomico_child);
			args_atomi[2] = strdup (sprintf_buffer);
			
			execve(atom, args_atomi, NULL);
			exit(0);
			break;
		default:
			break;
	}

	int energy_created = (my_num_atom * num_atomico_child) - num_atomico_child;
	sem_reserve(sem_writeShm, 0);
	shm_stat[3] = shm_stat[3] + energy_created; /* aumento l'energia generata */	
	shm_stat[2] = shm_stat[2] + 1; /* aumento le scissioni */
	sem_release(sem_writeShm, 0);
	
	sigemptyset(&maskBlock);
	sigaddset(&maskBlock, SIGUSR1);
	sigprocmask(SIG_UNBLOCK, &maskBlock, NULL);
}
	
void endLocal() { 
	shmdt(shm_stat);

	semctl(sem_init, 0, IPC_RMID);
	semctl(sem_writeShm, 0, IPC_RMID);

	msgctl(msqid, IPC_RMID, NULL);
	msgctl(msqid_scorie, IPC_RMID, NULL);

	exit(0);
}

int main(int argc, char * argv[]) {
	pid_master = atoi(argv[1]);
	my_num_atom = atoi(argv[2]);

	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = handle_signal; 
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

	read_in();

	stat_id = shmget(pid_master + 1, sizeof(int) * 6, 0600 | IPC_CREAT);
	shm_stat = shmat(stat_id, NULL, 0);

	sem_init = semget(pid_master + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(pid_master + 2, 1, 0600 | IPC_CREAT);

	msqid = msgget(pid_master + 1, IPC_CREAT | 0600);
	msqid_scorie = msgget(pid_master + 2, IPC_CREAT | 0600);

	my_msg.mtype = 1;
	my_msg.mpid = getpid();
	msgsnd(msqid, &my_msg, sizeof(my_msg.mpid), 0);

	/* Solo gli atomi iniziali devono diminuire sem_init */
	if(semctl(sem_init, 0, GETVAL) > 2)
		sem_reserve(sem_init, 0);

	while(!end_flag){
		pause();
	}
	endLocal();
}
