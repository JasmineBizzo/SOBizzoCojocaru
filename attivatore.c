#define _GNU_SOURCE // per signal handler

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define SIZE 3

int * pid_atomo;
int * num_atomico;
int max_index = SIZE-1; // massimo numero di elementi che si può mettere negli array
int ind = 0; // indice degli array 
int STEP_ATTIVATORE = 10; 
int stat_id;
int *shm_stat;

int msqid; // id coda di messaggi

void handle_signal(); // handler
void add_atom(); // aggiunge atomi quando riceve SIGUSR1
int check_memory_size(); // controlla se ho spazio per salvare l'atomo
void print(); // stampa per prova
void choose_atom(); // sceglie atomo da scindere
void found_waste(); // trovato scoria

void handle_signal(int signal){
	switch (signal){
		case SIGUSR1:
			add_atom(); //salva atomo nella tabella 
			
			break;
		case SIGUSR2: // atomo diventa scoria, bisogna toglierlo dalla tabella 
			// found_waste();
			break;
	}

}

struct msgbuff {
	int mtype; //message type
	int mnum_atomico; //message data
	int mpid;
} my_msg;

int main() {
	// shm_stat[11] = numero di atomi totali  
	// shm_stat[13] = pid del attivatore
    // shm_stat[14] = pid di atomo
	
	stat_id = shmget(getppid() + 1, sizeof(int) *15, 0600 | IPC_CREAT); //creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0);
	int tempo = 0; // secondi che passano 
	
	struct sigaction sa;
	bzero(&sa, sizeof(sa));		   // set all bytes to 0
	sa.sa_handler = handle_signal; // puntatore alla funzione handler dei segnali
	sigaction(SIGUSR1, &sa, NULL);

	srand(time(NULL));
	pid_atomo = malloc(sizeof(int) * SIZE);
	num_atomico = malloc(sizeof(int) * SIZE);
	
	// creazione / individuazione coda di messaggi
	msqid = msgget((getppid()+4), IPC_CREAT | IPC_EXCL | 0600);
	
	struct timespec req;
	req.tv_sec = 1;
	req.tv_nsec = 0;
	nanosleep(&req, NULL);

	while (tempo != STEP_ATTIVATORE){
		choose_atom();
	} 

/*
	add_atom();
	choose_atom();
	print();
    printf("COSA RIMANE IN MTEXT: %d \n ", my_msg.mtext); 
*/

	free(pid_atomo);
	free(num_atomico);
	msgctl(msqid, IPC_RMID, NULL);

	return 0;
}

void add_atom(){
	int num_bytes;

	if(!check_memory_size()){
		perror("Insufficient memory \n");
		return;
	}

	num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 1, 0);

	if( num_bytes < 0){
		printf("Messaggio non arrivato, num_bytes = %d \n", num_bytes);
		// return;
	}


	switch (fork()){
            case -1:
                printf("C'è stato un errore \n");
                exit(-1); 
                //kill (pid, SIGABORT) 
            case 0:
				sleep(2);
				
				exit(0); 
                break;
            default:
				pid_atomo[ind] = my_msg.mpid;// getpid(); 

				if (my_msg.mnum_atomico == 0) {
					num_atomico[ind]=rand()%118+1;
				} else {
					num_atomico[ind]= my_msg.mnum_atomico;
				}
				
            break;
    }
	ind++;
	shm_stat[11] += 1; // incremento il numero di atomi presenti
}

int check_memory_size(){
	if(ind == max_index) {
			printf("AUMENTO SPAZIO \n");
			pid_atomo = realloc(pid_atomo, (SIZE*2)*sizeof(int));
			num_atomico = realloc(num_atomico, (SIZE*2)*sizeof(int));

			if (pid_atomo == NULL || num_atomico == NULL) {
				perror("Insufficient memory");
				return 0;
			}
			max_index += SIZE;
	}

	return 1;
}

void choose_atom(){
	
	int n, b;
	n = rand()%ind+0;
	/*do {
       		// printf("IN WHILE \n");
       		// n = 1;
       		n = rand()%ind+0; // n= id del atomo da scindere
       		// printf("ID ATOMO DA SCINDERE: %d \n", n);
       		// controllo che l'id ci sia - magari era una scoria che ho già tolto
        	for (int b=1; b<=ind; b++){
        		if(shm_data->id[b]==n){
        			break;
        		}
        	}
        	       
        }while(b!=ind);*/
	// N = atomo da scindere
	my_msg.mnum_atomico=num_atomico[n]; 
	my_msg.mpid = pid_atomo[n];

	num_atomico[n] /= 2; //lo divido già per due  
	msgsnd(msqid, &my_msg, sizeof(my_msg), 0); // NUM_ATOMICO DEL ATOMO DA SCINDERE
	
	kill(shm_stat[14], SIGUSR1); // SEGNALE AD ATOMO PER AVVIARE LA SCISSIONE
	
}

void print(){
	for (int i = 0; i<ind; i++) {
		printf("ID: %d \n", i);
		printf("PID: %d \n", pid_atomo[i]);
		printf("NUM_ATOMICO: %d \n", num_atomico[i]);
		printf("INDEX: %d \n", ind);
		printf("MAX_INDEX: %d \n", max_index);
		printf("\n");
	}
}

void found_waste() {
	int num_bytes = msgrcv(msqid, &my_msg, sizeof(my_msg), 1, 0);

	if( num_bytes < 0){
		printf("Messaggio non arrivato, num_bytes = %d \n", num_bytes);
		// return;
	} else {
		for(int i=0; i<ind; i++) {
			// cerco l'atomo da cancellare dalla tabella usando il pid
			if(pid_atomo[i]==my_msg.mpid) {
				pid_atomo[i] = -1;
				num_atomico[i] = -1;
				shm_stat[11] = shm_stat[11]-1 ; // decremento il numero di atomi presenti

			}
		}
	}
}