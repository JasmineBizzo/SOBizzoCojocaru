#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int * shm_stat;
int stat_id, msqid, msqid_scorie, pid_master, my_num_atom, sem_init, sem_writeShm; 
int end_flag = 0;
struct msgbuff my_msg, my_msg_scorie;

void read_in();
void scissione();
void endLocal();

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			end_flag = 1;
			endLocal();
		break;
		case SIGUSR1:
			scissione();
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

/* Controllo scorie, divisione del proprio numero atomico e creazione atomo figlio */
void scissione() {
	/* INIZIO SEZIONE CRITICA */
	sigset_t maskBlock; /*Per le signal masks del segnale*/
	sigemptyset(&maskBlock);
	sigaddset(&maskBlock, SIGUSR1);
    sigprocmask(SIG_BLOCK, &maskBlock, NULL);

	/* Controllo non si crei una SCORIA, se il num atomico è troppo basso termino il processo */
	if (my_num_atom <= MIN_N_ATOMICO){
		sem_reserve(sem_writeShm, 0);
		shm_stat[4] = shm_stat[4] + 1; /* aumento il contatore delle scorie */
		shm_stat[5] = shm_stat[5] + 1;	
		sem_release(sem_writeShm, 0);

		my_msg_scorie.mpid = getpid();
		my_msg_scorie.mpid = 1;
		msgsnd(msqid_scorie, &my_msg_scorie, sizeof(my_msg_scorie.mpid), 0);

		kill(shm_stat[11], SIGUSR1); // segnale ad attivatore 
		kill(getpid(), SIGABRT); // segnale a se stesso per terminare
	}

	/* Calcolo del num atomico al figlio */
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
	char * args_atomi[]  = {"atomo.c", NULL};
	char ato[] = {"../src/atomo"};
	if (fork() == -1) {
		printf("MELTDOWN: ERRORE NELLA FORK PER LA SCISSIONE DEGLI ATOMI \n");
		kill(pid_master, SIGABRT);
	} else if (fork() == 0) {
		sprintf(sprintf_buffer, "%d", pid_master);
		args_atomi[1] = strdup (sprintf_buffer);
		sprintf(sprintf_buffer, "%d", num_atomico_child);
		args_atomi[2] = strdup (sprintf_buffer);

      	execve(ato, args_atomi, NULL);
     	exit(0);
	} 	
	int energy_created = (my_num_atom * num_atomico_child) - num_atomico_child;
	printf("ENERGIA CREATA: %d \n", energy_created);
	/* Incremento i counter nella shm */
	sem_reserve(sem_writeShm, 0);
	//printf("Attiv245 sem write dopo reserve 0 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
	shm_stat[0] = shm_stat[0] + energy_created; /* aumento il contatore dell'energia */
	shm_stat[1] = shm_stat[1] + energy_created;
	shm_stat[10] = shm_stat[10] + energy_created;
	shm_stat[6] = shm_stat[6] + 1; /* aumento il contatore delle scissioni */
	shm_stat[7] = shm_stat[7] + 1;	
	sem_release(sem_writeShm, 0);
	//printf("Attiv 232 writeShm 1 = %d \n ", semctl(sem_writeShm, 0, GETVAL));

	sigemptyset(&maskBlock);
	sigaddset(&maskBlock, SIGUSR1);
	sigprocmask(SIG_UNBLOCK, &maskBlock, NULL);
	/* FINE SEZIONE CRITICA */
}
	
void endLocal(){ 
	/* Memoria condivisa */
	shmdt(shm_stat);
	shmctl(stat_id, IPC_RMID, NULL);

	/* Semafori */
	semctl(sem_init, 0, IPC_RMID);
	semctl(sem_writeShm, 0, IPC_RMID);

	/* Coda di messaggi */
	msgctl(msqid, IPC_RMID, NULL);
	
	printf("ATOMO MUORE \n");
	exit(0);
}

int main(int argc, char * argv[]) {
	pid_master = atoi(argv[1]);
	my_num_atom = atoi(argv[2]); 

	srand(time(NULL));
	struct sigaction sa;

	bzero(&sa, sizeof(sa));
	sa.sa_handler = handle_signal; 
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

	stat_id = shmget(pid_master + 1, sizeof(int) * 13, 0600 | IPC_CREAT);
	shm_stat = shmat(stat_id, NULL, 0);	
 
 	sem_init = semget(pid_master + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(pid_master + 2, 1, 0600 | IPC_CREAT);

	msqid = msgget(pid_master + 1, IPC_CREAT | 0600);
	msqid_scorie = msgget(getppid() + 2, IPC_CREAT | 0600);

	my_msg.mtype = 1;
	my_msg.mpid = getpid();
	msgsnd(msqid, &my_msg, sizeof(my_msg.mpid), 0);

	read_in();
	
	/* Solo gli atomi iniziali devono diminuire sem_init -> lascio a 2 per alim e attiv */
	if(semctl(sem_init, 0, GETVAL) > 2)
		sem_reserve(sem_init, 0);
	
	while(!end_flag);
	/*Esce dal loop per chiamare l'handler quando riceve un segnale da attivatore*/
}