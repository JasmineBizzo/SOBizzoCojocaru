#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int * pid_atomo, * num_atomi_tot;
int stat_id, msqid, ind;
int *shm_stat;
int sem_pid_atomi, sem_writeShm;
struct msgbuff my_msg; 
int end_flag = 0; /*diventa 1 solo quando il processo può terminare*/

void read_in();
int check_memory_size();
void add_atom();
void choose_atom(); 
void endLocal();

void handle_signal(int signal) {
	switch (signal){
		case SIGABRT:
			end_flag = 1;
			endLocal();
		break;
		case SIGUSR1:
			add_atom();
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
	if(num_atomi_tot[0] == sizeof(pid_atomo)) {
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
	printf("SONO IN ADD ATOMi \n");
	ind = num_atomi_tot[0];
	int num_bytes =-1;
	if(!check_memory_size()){
		perror("Insufficient memory \n");
		return;
	}

	for(int i = 1; i < 15; i++){
		memset(&my_msg, 0, sizeof my_msg); /*SERVE?*/
		/*NOWAIT a caso serve???*/
		if(msgrcv(msqid, &my_msg, sizeof(my_msg.mpid), 0, IPC_NOWAIT) != -1) {
			 /*CODA NON VUOTA, LEGGO E SALVO I PID*/
			printf("Messaggio arrivato ATTIVATORE - ADDATOMO, num_bytes = %d \n", num_bytes);
		
			pid_atomo[ind] = my_msg.mpid;
			ind++;
			num_atomi_tot[0] += 1;
		} else {
			printf("Messaggio non arrivato ATTIVATORE /coda vuota - ADDATOMO, num_bytes = %d \n", num_bytes);
			i = 15; /*esco dal ciclo, non ci sono messaggi*/
		}
	}
}

	/* Sceglie un atomo casuale a cui inviare il segnale di scissione */
void choose_atom() {
	int ind = num_atomi_tot[0];
	int n = rand()%(ind-1)+0;

	kill(pid_atomo[n], SIGUSR1);// SEGNALE AD ATOMO PER AVVIARE LA SCISSIONE

	/*Poi incremento il counter delle attivazioni in shm */
	while(semctl(sem_writeShm, 0, GETVAL) != 1);
		sem_reserve(sem_writeShm, 0);
	printf("Attiv245 sem write dopo reserve 0 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
			
		shm_stat[8] = shm_stat[8] + 1; 
		shm_stat[9] = shm_stat[9] + 1;

		sem_release(sem_writeShm, 0);
	printf("Attiv 232 writeShm 1 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
}

void endLocal() {
	int i = num_atomi_tot[0];
    if(pid_atomo[i]!= -1) { 
        for (i; i >= 0; i--) {  
            if(pid_atomo[i] != -1){
                kill(pid_atomo[i], SIGABRT);
            }
        }
    }

	/* Memoria condivisa */
	shmdt(shm_stat);
	shmctl(stat_id, IPC_RMID, NULL);

	/* Semafori */
	semctl(sem_pid_atomi, 0, IPC_RMID);
	semctl(sem_writeShm, 0, IPC_RMID);
   // semctl(sem_att, 0, IPC_RMID);

	/* Coda di messaggi */
	msgctl(msqid, IPC_RMID, NULL);

	free(pid_atomo);
	free(num_atomi_tot);
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

	read_in();

	stat_id = shmget(getppid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT);
	shm_stat = shmat(stat_id, NULL, 0);

	pid_atomo = malloc(sizeof(int) * N_ATOMI_INIT);
	num_atomi_tot = malloc(sizeof(int));
	/*Memoria locale che contiene il num totale di atomi creati
	La uso per non accedere alla shm in ogni metodo in cui uso ind*/

	num_atomi_tot[0] = 0;
	printf("Num atomi tot in Attivatore 269 %d \n",num_atomi_tot[0]);

	sem_pid_atomi = semget(getppid() + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(getppid() + 2, 1, 0600 | IPC_CREAT);
	//sem_att = semget(getppid() + 3 , 1, 0600 | IPC_CREAT);

	printf("creazione / individuazione coda di messaggi\n");
	msqid = msgget(getppid() + 1, IPC_CREAT | 0600);
	
	//sem_setval(sem_att, 0, 0);
	sem_release(sem_pid_atomi, 0);

	/*Per iniziare il suo ciclo, l'Attivatore attentde che tutti gli atomi siano creati */
	while(semctl(sem_pid_atomi, 0, GETVAL) != (N_ATOMI_INIT + 2));
	//while(semctl(sem_att, 0, GETVAL) != 0);

	printf("ATTIVATORE CONTINUA. 338 fa choose atom \n");
	
	/* Al secondo 0 per introdurre dell'energia, poi entro nel while per ogni STEP_ATTIVATORE */
	for(int l = 0; l < 2; l++) {
		add_atom();
		choose_atom();	
	}
	
	int intervallo = STEP_ATTIVATORE;
	int t = 0; // secondi che passano
	//sem_reserve(sem_att, 0); /*master può iniziare la stampa*/
	
	while(t <= SIM_DURATION && !end_flag) { 
		printf("ECCOMI attivatore ciclo while 384 \n");
		req.tv_sec = 1;
        req.tv_nsec = 0;
        nanosleep(&req, NULL);
		add_atom(); /*Ogni ciclo controllo se ci sono nuovi atomi da salvare*/
			if(t == intervallo){
				choose_atom();
				intervallo += STEP_ATTIVATORE;
			}
		t++;
		//sem_reserve(sem_att, 0); /*master può iniziare la stampa*/ 
	}
}