#include <signal.h> //utilizzo delle funzioni per la gestione dei segnali fra processi
#include <errno.h> //errno, variabile per indicare cosa e' andato storto
#include <stdlib.h>
#include <unistd.h> 
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h> 

#define SIZE 3
int * pid_atomo;
int * num_atomico;


int main() {

	/*
	int pid_atomo[SIZE];
	int num_atomico[SIZE];*/

	srand(time(NULL));
	pid_atomo = malloc(sizeof(int) * SIZE);
	num_atomico = malloc(sizeof(int) * SIZE);

	int max_index = SIZE-1; // massimo numero di elementi che si può mettere negli array
	int index = 0; // a quale posizione sono arrivata per scrivere nel array
	int b; 

	for(int i = 0; i < 3; i++){
       // int count = 0;
        switch (b = fork()){
            case -1:
                printf("C'è stato un errore");
                exit(-1); 
                //kill (pid, SIGABORT) 
            case 0:
				sleep(2);
				
                // sprintf(argv_buffer, "%d", (i+1));
                // args_atomi[1] = argv_buffer;
                // execve("../bin/atomo", args_atomi, NULL); //???????? args atomi = pid del master
                
				exit(0); 
                break;
            default:
				pid_atomo[i]= b;// getpid(); 
                num_atomico[i]=rand()%118+1;
				// Ho messo che è il padre che salva i dati in memoria perchè è lui il process principale
				// su cui lavoriamo, se mettevo che li salvano i figli dopo non riuscivo più a recuperarli 
				// perchè le memorie locali dei vari figli e del genitore sono diverse. 
				/*printf("ID: %d \n", i);
				printf("PID: %d \n", pid_atomo[i]);
				printf("NUM_ATOMICO: %d \n", num_atomico[i]);
				printf("\n"); */
            break;
        }
		index++;
		// Se index == max_index allora sono arrivata alla fine degli array, 
		// quindi realloc per aumentarne la dimensione

		if(index == max_index) {
			printf("AUMENTO SPAZIO \n");
			pid_atomo = realloc(pid_atomo, (SIZE*2)*sizeof(int));
			num_atomico = realloc(num_atomico, (SIZE*2)*sizeof(int));

			if (pid_atomo == NULL || num_atomico == NULL) {
				perror("Insufficient memory");
				break;
			}
			max_index += SIZE;
		}

		
        
    }

	for (int i = 0; i<index; i++) {
		printf("ID: %d \n", i);
		printf("PID: %d \n", pid_atomo[i]);
		printf("NUM_ATOMICO: %d \n", num_atomico[i]);
		printf("INDEX: %d \n", index);
		printf("MAX_INDEX: %d \n", max_index);
		printf("\n");
	}
	
	free(pid_atomo);
	free(num_atomico);
	
}

