#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int stat_id, atom_pid_id, atom_num_id, num_atomico;
int *shm_stat, *shm_atom_pid, *shm_atom_num;
int sem_pid_atomi, sem_writeShm, sem_saveatom;
//pid_t pidAttivatore, pidAliment; 
struct msgbuff my_msg_alim; 
int msqid_init;
int end_flag = 0; /*diventa 1 solo quando il processo può terminare*/

void read_in();
void endLocal();
//void choose_atom();

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

int main(int argc, char * argv[]) {
	srand(time(NULL)); //senza a ogni esecuzione del programma viene sempre estratto lo stesso numero
	
	int pid_master = getppid();
	struct timespec req;
	
	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler
	
	char ato[] = {"../src/atomo"}; 
	char * args_atomi[]  = {"atomo.c", NULL}; 
	char sprintf_buffer[100];

	read_in();
	
	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);									// collegare l'area a un puntatore

	// SEMAFORO O SEGNALE che parte ogni STEP ALIMENTAZIONE nano secondi!!

	sem_pid_atomi = semget(getppid() + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);
	//sem_saveatom = semget(getpid() + 4, 1, 0600 | IPC_CREAT);

	//msqid_init = msgget(getppid() + 2, IPC_CREAT | 0600);


	//int t = 0;
	
	// printf("IN ALIMENTAZIONE - SEMAFORO = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	sem_release(sem_pid_atomi, 0);
	// printf("IN ALIMENTAZIONE DOPO RESERVE - SEMAFORO = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	
	while(semctl(sem_pid_atomi, 0, GETVAL) != (N_ATOMI_INIT + 2));

	printf("ALIMENTATORE CONTINUA. \n");
	//SIM_DURATION =5;
//	N_NUOVI_ATOMI = 2;
	// printf("FERMO WHILE \n"); // altrimenti mi va in loop e non vedo altro - 3000 è troppo poco 
	while(!end_flag){	//(semctl(sem_pid_atomi, 0, GETVAL) == (N_ATOMI_INIT + 2) && t <= SIM_DURATION
		sleep(10); /*!!!!!!!*/
		req.tv_sec = 0;
        req.tv_nsec = STEP_ALIMENTAZIONE;
        nanosleep(&req, NULL);

		/*printf("ECCOMI Aliment\n");
		printf("SIM_DURATION %d \n", SIM_DURATION);
		printf("N_NUOVI_ATOMI %d \n", N_NUOVI_ATOMI);
		printf("t %d \n", t); */
		// printf("PRIMA FOR aliment\n");
			//int i = shm_stat[11]; //i parte dal numero di atomi creati fino a quel momento
			//int new = shm_stat[11] + N_NUOVI_ATOMI; //ed arriva a questo
    	for(int i = 0; i < N_NUOVI_ATOMI; i++) {
			int pid_master = getppid();

			switch (fork()) {
    			case -1:
        			printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI IN ALIMENTAZIONE\n");
            		kill(getpid(), SIGABRT);
        		case 0:
					num_atomico = rand()%N_ATOM_MAX+1;
					/*sem_reserve(sem_saveatom, 0); //atomo quando inizia a crearsi mette saveatom a 0

	       			printf("DEVO ESSERE SALVATO, QUINDI INVIO MESSAGGIO AD ATTIVATORE \n");
	       			my_msg.mtype = 1;
	        		my_msg.mpid = getpid();
            
	        		printf("CONTENUTO DELLA CODA: %d, %d \n", my_msg.mpid, my_msg.mnum_atomico);
	       	 		msgsnd(msqid_init, &my_msg, sizeof(my_msg), 0);
	        		kill(shm_stat[13], SIGUSR2); // SEGNALE AD ATTIVATORE per save atomo
            		while(semctl(sem_saveatom, 0, GETVAL) == 0); /*aspetto si salvi*/

					sprintf(sprintf_buffer, "%d", pid_master);
           			args_atomi[1] = strdup (sprintf_buffer);
            // printf("VEDIAMO IL BUFFER: %s \n", argv_buffer);
            // sprintf(argv_buffer + j + 1, "%d", rand()%(N_ATOM_MAX+1)+1);
           			sprintf(sprintf_buffer, "%d", num_atomico);
		    		args_atomi[2] = strdup (sprintf_buffer);



            		execve(ato, args_atomi, NULL); 
            		exit(0);
        		break;
        	default:
        	break;
        	}
			printf("ALIMENTATORE HA CREATO ATOMI \n");
			// printf("\n DOPO SEGNALE IL PROCESSO alimentazione CONTINUA 2 \n");

			/*sem_reserve(sem_writeShm, 0);
			shm_stat[11] = shm_stat[11] + 1; //aumento atomi totali
			sem_release(sem_writeShm, 0);*/
    		}
			
		}
		
		//t++;
		//intervallo += STEP_ALIMENTAZIONE;
		// printf("\n DOPO SEGNALE IL PROCESSO CONTINUA ALIMENTAZIONE 177\n");
	//}
}

void endLocal(){
	//deoalloco memorie, semafori etc!!!!!
	// memoria condivisa 
	shmdt(shm_stat);
	shmctl(stat_id, IPC_RMID, NULL);

	// semafori
	semctl(sem_pid_atomi, 0, IPC_RMID);
	semctl(sem_writeShm, 0, IPC_RMID);
    semctl(sem_saveatom, 0, IPC_RMID);

	// coda di messaggi
	// msgctl(msqid, IPC_RMID, NULL);
	printf("ALIMENTAZIONE MUORE \n");
	exit(0);
}