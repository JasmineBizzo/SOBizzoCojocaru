#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int stat_id, atom_pid_id, atom_num_id;
int *shm_stat, *shm_atom_pid, *shm_atom_num;
int sem_pid_atomi, sem_writeShm;
//pid_t pidAttivatore, pidAliment; 
struct msgbuff my_msg_alim; 
int msqid_alim;

void read_in();
void endLocal();
//void choose_atom();


void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
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

int main(int argc, char * argv[]) {
	srand(time(NULL)); //senza a ogni esecuzione del programma viene sempre estratto lo stesso numero
	
	//int intervallo = STEP_ALIMENTAZIONE;

	struct timespec req;
	
	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler
	
	char ato[] = {"../src/atomo"}; 
	char * args_atomi[]  = {"atomo.c", NULL}; 
	char argv_buffer[50];
	sprintf(argv_buffer, "%d", (getppid())); // passiamo il pid del master 
	args_atomi[1] = argv_buffer;

	void read_in();
	
	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);									// collegare l'area a un puntatore

	// SEMAFORO O SEGNALE che parte ogni STEP ALIMENTAZIONE nano secondi!!

	sem_pid_atomi = semget(getppid() + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);

	msqid_alim = msgget(getppid() + 2, IPC_CREAT | IPC_EXCL | 0600);



	int t = 0;
	
	printf("IN ALIMENTAZIONE - SEMAFORO = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	sem_reserve(sem_pid_atomi, 0);
	printf("IN ALIMENTAZIONE DOPO RESERVE - SEMAFORO = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	
	while(semctl(sem_pid_atomi, 0, GETVAL) != 0);

	printf("ALIMENTATORE CONTINUA. \n");
	//SIM_DURATION =5;
//	N_NUOVI_ATOMI = 2;
	printf("FERMO WHILE \n"); // altrimenti mi va in loop e non vedo altro - 3000 è troppo poco 
	while(1==0){	//(semctl(sem_pid_atomi, 0, GETVAL) == 0 && t <= SIM_DURATION
		req.tv_sec = 0;
        req.tv_nsec = 3000; //STEP_ALIMENTAZIONE;
        nanosleep(&req, NULL);


		/*printf("ECCOMI Aliment\n");
		printf("SIM_DURATION %d \n", SIM_DURATION);
		printf("N_NUOVI_ATOMI %d \n", N_NUOVI_ATOMI);
		printf("t %d \n", t); */
		printf("PRIMA FOR aliment\n"); 
		int i;
		//if(t == intervallo){
			// N_NUOVI_ATOMI = 3; /*!!!!*/
			// N_ATOM_MAX = 10;
		printf("N_NUOVI_ATOMI: %d \n", N_NUOVI_ATOMI);
			//int i = shm_stat[11]; //i parte dal numero di atomi creati fino a quel momento
			//int new = shm_stat[11] + N_NUOVI_ATOMI; //ed arriva a questo
    		for(i = 0; i < N_NUOVI_ATOMI; i++) {
				/*PIPE alimentazione atomo atomo
				int pipefd[2];
    			pipe(pipefd);

    			//char buf[100];
				int n = 1010;//getppid();
    			int num_bytes;
 				close(pipefd[0]); //chiudi lettura
    			num_bytes = write(pipefd[1], &n, sizeof(n)); //getppid()
				close(pipefd[1]); // chiusa scrittura */
	
	
				switch (fork()) {
    				case -1:
        				printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI IN ALIMENTAZIONE\n");
            			kill(getpid(), SIGABRT);
        			case 0:
					

            			execve(ato, args_atomi, NULL); 
            			exit(0);
        			break;
        		default:
        		break;
        		}
				printf("\n DOPO SEGNALE IL PROCESSO CONTINUA 2 \n");

				sem_reserve(sem_writeShm, 0);
				shm_stat[11] = shm_stat[11] + 1; /*aumento atomi totali*/
				sem_release(sem_writeShm, 0);
    		}
			
		}
		
		t++;
		//intervallo += STEP_ALIMENTAZIONE;
		printf("\n DOPO SEGNALE IL PROCESSO CONTINUA 3 \n");
	//}
}

void endLocal(){
	//deoalloco memorie, semafori etc!!!!!

	exit(0);
}