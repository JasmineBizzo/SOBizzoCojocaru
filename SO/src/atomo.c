#include "utils.h"
#include "../lib/ipc.c"

//Cancella quelli non usati
int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int stat_id, msqid, pid_master; //, atom_pid_id, atom_num_id;
int *shm_stat; //, *shm_atom_pid, *shm_atom_num;
int sem_pid_atomi, sem_writeShm, sem_att, sem_saveatom;
struct msgbuff my_msg; //, my_msg_createAtom;
//pid_t pidAttivatore, pidAliment; // process identificator dei process id
    // struct msgbuff; /* */
int end_flag = 0; /*diventa 1 solo quando il processo può terminare*/

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
			printf("\n ATOMO RICEVE SEGNALE SCISSIONE \n");
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

void scissione() {
	int num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 0, 0); //ricevo il messaggio

	if(num_bytes < 0){
		printf("Messaggio non arrivato in ATOMO - SCISSIONE  num_bytes = %d \n", num_bytes);
		//return;
	} else {
		printf("Messaggio arrivato in ATOMO - SCISSIONE \n");
	}

	/*Gli viene passato il num atomico dell'atomo che nasce dalla scissione, 
	già diviso. Lui lo ripasserà ad attivatore che lo aggiungerà al suo 
	array locale con add_atom()*/
	int num_atomico_child = my_msg.mnum_atomico;

	/*ORA IN ATTIVATORE
	//Per prima cosa controllo che non si crei una 
	if (num_atomico <= MIN_N_ATOMICO) {
		printf("IN ATOMO - TROVATA SCORIA \n");
		my_msg.mtype=1;
		my_msg.mpid = pid_atomo;
		my_msg.mnum_atomico = num_atomico;
		msgsnd(msqid, &my_msg, sizeof(my_msg), 0);
		kill(shm_stat[13], SIGUSR2);
		//exit(0); //termina il processo chiamante
	} */

	if (fork() == -1) {
		printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
        kill(shm_stat[12], SIGABRT); /* INVIO UN SEGNALE AL MASTER E termin0 simulazione*/
	} else if (fork() == 0) {
		sem_reserve(sem_saveatom, 0); //atomo quando inizia a crearsi mette saveatom a 0

		printf("DEVO ESSERE SALVATO, QUINDI INVIO MESSAGGIO AD ATTIVATORE \n");
		my_msg.mtype = 1;
		my_msg.mpid = getpid();
		my_msg.mnum_atomico = num_atomico_child; /*Qui metto quello del figlio della scissione*/
		
		printf("CONTENUTO DELLA CODA: %d, %d \n", my_msg.mpid, my_msg.mnum_atomico);
		msgsnd(msqid, &my_msg, sizeof(my_msg), 0);
		kill(shm_stat[13], SIGUSR1); // SEGNALE AD ATTIVATORE per save atomo figlio

   		char argv_buffer[50];
 		char * args_atomi[]  = {"atomo.c", NULL};
    	char ato[] = {"../src/atomo"}; 

		sprintf(argv_buffer, "%d", pid_master);
    	args_atomi[1] = argv_buffer;
		printf("ATOMO 135 VEDIAMO ARGS_ATOMI 1: %d \n", atoi(args_atomi[1]));

		// int pid_master = (atoi) argv[1];
 		// sprintf(argv_buffer, "%d", (getppid()));
   		// 	args_atomi[1] = argv_buffer;
		execve(ato, args_atomi, NULL);
     	exit(0);
	} 
		

/*ANCHE QUESTO ORA È IN ATTIVATORE
        while(semctl(sem_writeShm, 0, GETVAL) != 1);
		sem_reserve(sem_writeShm, 0);
		printf(" Sem atomo 104 %i\n ", semctl(sem_writeShm, 0, GETVAL));

			//Incremento il counter dell'energia liberata ad ogni scissione
			//Posso sottrarre sempre il num atom del figlio perchè o sono uguali o questo è maggiore
			int energy_created = (num_atomico * my_msg.mnum_atomico) - my_msg.mnum_atomico;
			shm_stat[0] = shm_stat[0] + energy_created; /* aumento il contatore dell'energia in shm 
			shm_stat[1] = shm_stat[1] + energy_created;
			shm_stat[10] = shm_stat[10] + energy_created;
			shm_stat[6] = shm_stat[6] + 1; /* aumento il contatore delle scissioni in shm 
			shm_stat[7] = shm_stat[7] + 1;
			//shm_stat[11] = shm_stat[11] + 1; /*SERVE???? aumento atomi totali

			sem_release(sem_writeShm, 0);
			printf(" Sem atomo 116 %i\n ", semctl(sem_writeShm, 0, GETVAL)); */


			//sem_reserve(sem_att, 0); /*master può iniziare la stampa*/ 
			// my_msg.mtype=1;
			// msgsnd(msqid, &my_msg, sizeof(my_msg), 0); //INVIO IL MESSAGGIO
			// kill(shm_stat[14], SIGUSR1); // SEGNALE AD ATTIVATORE PER REGISTRARE L'ATOMO
			// exit(0);
	}
	
void endLocal(){ 
	//deoalloco memorie, semafori etc!!!!!
	//shmdt(shm_stat);
	//shmctl(stat_id, IPC_RMID, NULL);
	exit(0);
}

int main(int argc, char * argv[]) {
	printf("SONO IN ATOMO \n");
	pid_master = atoi(argv[1]);
	// int num_atomico = atoi(argv[2]);

	printf("PID DEL MASTER: %d \n", pid_master);
	printf("PID DELL'ATOMO CREATO: %d \n", getpid());
	// printf("NUM_ATOMICO DA ARGV: %d \n", num_atomico);
	srand(time(NULL));

	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGABRT, &sa, NULL); // set del signal handler
	sigaction(SIGUSR1, &sa, NULL);

	read_in();

	stat_id = shmget(pid_master + 1, sizeof(int) * 15, 0600 | IPC_CREAT); // creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);			// collegare l'area a un puntatore
 
 	sem_pid_atomi = semget(pid_master + 1, 1, 0600 | IPC_CREAT);
	sem_writeShm = semget(pid_master + 2, 1, 0600 | IPC_CREAT);
	sem_att = semget(pid_master + 3 , 1, 0600 | IPC_CREAT);
	sem_saveatom = semget(pid_master + 4, 1, 0600 | IPC_CREAT); // mi serve per aspettare che attivatore salvi gli atomi
	
	printf ("valore SEM_PID ATOMI: %d \n", semctl(sem_pid_atomi, 0, GETVAL)); 
	// printf ("SEMAPHORE SEM_WRITESHM: %d \n", semctl(sem_writeShm, 0, GETVAL)); 
	
	// IPC_EXCL | 
	msqid = msgget(pid_master + 1, IPC_CREAT | 0600);
	//msqid_createAtom = msgget(pid_master + 2, IPC_CREAT | 0600); // coda per salvare gli atomi

	/*	QUESTO LO SPOSTO NEL MASTER e IN METODO SCISSIONE
	sem_reserve(sem_saveatom, 0); //atomo quando inizia a crearsi mette saveatom a 0

	printf("DEVO ESSERE SALVATO, QUINDI INVIO MESSAGGIO AD ATTIVATORE \n");
	my_msg.mtype = 1;
	my_msg.mpid = getpid();

	printf("CONTENUTO DELLA CODA: %d, %d \n", my_msg.mpid, my_msg.mnum_atomico);
	msgsnd(msqid, &my_msg, sizeof(my_msg), 0);
	kill(shm_stat[13], SIGUSR1); // SEGNALE AD ATTIVATORE per creazione atomo*/

	while(semctl(sem_saveatom, 0, GETVAL) == 0); // aspetto che attivatore mi salvi 

	printf("\nDOPO SEGNALE IL PROCESSO ATOMO CONTINUA\n");	
	
	sem_release(sem_pid_atomi, 0);/* ANCHE GLI ATOMI CREATI DURANTE LA SIMULAZIONE LO AUMENTANO, MODO PER RISOLVERE? :( */
	printf("IN ATOMO 230 dopo pid atomi - SEM pid atomi = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
	printf("ATOMO CONTINUA \n");
	while(!end_flag); /*Esce dal loop per chiamare signal handler quando riceve un segnale da attivatore*/
	/*GUARDA BENE SE FUNZIONA! TORNA NEL LOOP DOPO SCISSIONE? IN TEORIA SI*/
}