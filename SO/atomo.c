#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int * num_atom;
int stat_id, msqid; 
int *shm_stat; 
int sem_pid_atomi, sem_writeShm; 
struct msgbuff my_msg;
int end_flag = 0;

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

/** Dimezza il proprio numero atomico, fa controllo scorie e poi crea atomo figlio */
void scissione() {
	int num_atomico_child;

	/* Calcolo del num atomico al figlio */
	if (num_atom[0] % 2 == 0) { 
		num_atom[0] /= 2; 
		num_atomico_child = num_atom[0]; 	
	} else { 
		num_atom[0] /= 2; 
		num_atomico_child = num_atom[0] + 1;
	}

	/**Poi controllo che non si crei una SCORIA
	 * Se questo atomo ha numero atomico troppo basso lo termino e non creo il figlio
	 */
	if (num_atom[0] <= MIN_N_ATOMICO) {
		kill(getpid(), SIGABRT);
	} 

	/* Creo il figlio della scissione */
	char sprintf_buffer[100];
 	char * args_atomi[]  = {"atomo.c", NULL};
    char ato[] = {"../src/atomo"};
	if (fork() == -1) {
		printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
        kill(shm_stat[12], SIGABRT);
	} else if (fork() == 0) {
		sprintf(sprintf_buffer, "%d", num_atom[1]);
        args_atomi[1] = strdup (sprintf_buffer);
        sprintf(sprintf_buffer, "%d", num_atomico_child);
		args_atomi[2] = strdup (sprintf_buffer);

        printf("VEDIAMO ARGS_ATOMI: %d %d\n", atoi(args_atomi[1]), atoi(args_atomi[2]));
		execve(ato, args_atomi, NULL);
     	exit(0);
	} 	

	/* Incremento i counter nella shm */
	while(semctl(sem_writeShm, 0, GETVAL) != 1);
		sem_reserve(sem_writeShm, 0);
		printf("Attiv245 sem write dopo reserve 0 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
		
		int energy_created = (num_atom[0] * num_atomico_child) - num_atomico_child;
		shm_stat[0] = shm_stat[0] + energy_created; /* aumento il contatore dell'energia */
		shm_stat[1] = shm_stat[1] + energy_created;
		shm_stat[10] = shm_stat[10] + energy_created;
		shm_stat[6] = shm_stat[6] + 1; /* aumento il contatore delle scissioni */
		shm_stat[7] = shm_stat[7] + 1;	

	sem_release(sem_writeShm, 0);
	printf("Attiv 232 writeShm 1 = %d \n ", semctl(sem_writeShm, 0, GETVAL));
}
	
void endLocal(){ 
	/* Memoria condivisa */
	shmdt(shm_stat);
	shmctl(stat_id, IPC_RMID, NULL);

	/* Semafori */
	semctl(sem_pid_atomi, 0, IPC_RMID);
	semctl(sem_writeShm, 0, IPC_RMID);
    //semctl(sem_att, 0, IPC_RMID);

	/* Coda di messaggi */
	msgctl(msqid, IPC_RMID, NULL);
	
	free(num_atom);
	printf("ATOMO MUORE \n");
	exit(0);
}

int main(int argc, char * argv[]) {
	num_atom = malloc(sizeof(int)*2); 							
	num_atom[0] = atoi(argv[2]);/*Salvo il num atomico di questo atomo in pos 0*/
	num_atom[1] = atoi(argv[1]);/*In pos 1 c'è il pid del master */
	srand(time(NULL));

	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler
	sigaction(SIGUSR1, &sa, NULL);

	read_in();

	stat_id = shmget(num_atom[1] + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);			// collegare l'area a un puntatore
 
 	sem_pid_atomi = semget(num_atom[1] + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(num_atom[1] + 2, 1, 0600 | IPC_CREAT);
	//sem_att = semget(num_atom[1] + 3 , 1, 0600 | IPC_CREAT);
	
	msqid = msgget(num_atom[1] + 1, IPC_CREAT | 0600);
	
	/* Invio messaggio ad Attivatore che salva il pid dell'atomo */
	printf("DEVO ESSERE SALVATO, QUINDI INVIO MESSAGGIO AD ATTIVATORE \n");
	my_msg.mtype = 1;
	my_msg.mpid = getpid();
	msgsnd(msqid, &my_msg, sizeof(my_msg.mpid), 0);
	
	/* ANCHE GLI ATOMI CREATI DURANTE LA SIMULAZIONE LO AUMENTANO, MODO PER RISOLVERE? :( */
	sem_release(sem_pid_atomi, 0);
	printf("IN ATOMO 230 dopo pid atomi - SEM pid atomi = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	printf("ATOMO CONTINUA \n");
	while(!end_flag);
	/*Esce dal loop per chiamare signal handler quando riceve un segnale da attivatore*/
}