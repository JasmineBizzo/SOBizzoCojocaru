#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int stat_id, sem_pid_atomi;
int *shm_stat;
int end_flag = 0;

void read_in();
void endLocal();

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			end_flag = 1;
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

void endLocal(){
	/* Memoria condivisa */
	shmdt(shm_stat);
	shmctl(stat_id, IPC_RMID, NULL);

	/* Semafori */
	semctl(sem_pid_atomi, 0, IPC_RMID);

	printf("ALIMENTAZIONE MUORE \n");
	exit(0);
}

int main(int argc, char * argv[]) {
	srand(time(NULL));
	struct timespec req;

	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGABRT, &sa, NULL);

	read_in();
	
	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT);
	shm_stat = shmat(stat_id, NULL, 0);									

	sem_pid_atomi = semget(getppid() + 1, 1, 0600 | IPC_CREAT);
	sem_release(sem_pid_atomi, 0);
	
	//while(semctl(sem_pid_atomi, 0, GETVAL) != (N_ATOMI_INIT + 2));
		
	char ato[] = {"../src/atomo"};
	char * args_atomi[]  = {"atomo.c", NULL};
	char sprintf_buffer[100];
	int pid_master = getppid();
	while(!end_flag){
		sleep(3); /**********TOGLI************/
		req.tv_sec = 0;
        req.tv_nsec = STEP_ALIMENTAZIONE;
        nanosleep(&req, NULL);
		
    	for(int i = 0; i < N_NUOVI_ATOMI; i++) {
			switch (fork()) {
    			case -1:
        			printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI IN ALIMENTAZIONE\n");
            		kill(getpid(), SIGABRT);
        		case 0:
					int num_atomico = rand()%N_ATOM_MAX+1;
					sprintf(sprintf_buffer, "%d", pid_master);
           			args_atomi[1] = strdup (sprintf_buffer);
           			sprintf(sprintf_buffer, "%d", num_atomico);
		    		args_atomi[2] = strdup (sprintf_buffer);

            		execve(ato, args_atomi, NULL); 
            		exit(0);
        		break;
        	default:
        	break;
        	}
			printf("\n DOPO SEGNALE IL PROCESSO alimentazione CONTINUA 2 \n");
    	}
	}
}
