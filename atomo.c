##include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shared-memory.h"

//#define MAX(x,y) (((x) > (y)) > (x) : (y))
// tanto padre e figlio hanno lo stesso num atomico lol

#define MIN_N_ATOMICO 20 

int main() {
	int i, n, m_id, s_id, p;
	struct stat_data * my_stat;
	struct shared_data * my_data;
	
	s_id=shmget(getpid() + 1, sizeof(*my_stat), 0600 | IPC_CREAT); //creazione area di memoria condivisa
	my_stat=shmat(s_id, NULL, 0); // collegare l'area a un puntatore
	
	m_id=shmget(getpid() + 2, sizeof(*my_data), 0600 | IPC_CREAT); //creazione area di memoria condivisa
	my_data=shmat(m_id, NULL, 0); // collegare l'area a un puntatore
	
	// inizio a salvare in sm i dati del master	
	my_data->pid[0] = getpid();  // parent process - master
	my_data->id[0] = 0; 
    my_data->num_atomico[0]=150;
	
	my_data->j=0; // inizializzo il puntatore della sh my_data
		
		printf("*ID: %d \n",my_data->id[0]);
        printf("*PID: %d \n",my_data->pid[0]);
        printf("*NUM_ATOMICO %d \n", my_data->num_atomico[0]);
        printf("\n");
	
	// inizializzo i valori della sm my_stat
		my_stat->energy = 100;  
        my_stat->scorie = 0;
        
        printf("STAT PROVVISORIE\n");
        printf("ENERGY: %d \n", my_stat->energy);
        printf("SCORIE: %d \n", my_stat->scorie);
       	printf("\n");
       	
       	// crea tot processi e salva i loro dati in sm
       	for(i=0; i<2; i++) { 
		
		if(p = fork() == 0){ // children processes - atomo 
			
			sleep(1);
			// sops.sem_op = -1;
			// semop(s_id, &sops, 1);
			
			my_data->pid[my_data->j+=1] = getpid();
            my_data->id[my_data->j] = my_data->j;
			my_data->num_atomico[my_data->j]=rand()%118+1; 
			// da riguardare da random: 
			// ci possono essere atomi con lo stesso numero atomico?
			
			printf("ID: %d \n",my_data->id[my_data->j]);
			printf("PID: %d \n",my_data->pid[my_data->j]);
			printf("NUM_ATOMICO: %d \n",my_data->num_atomico[my_data->j]);
			printf("\n");
			exit(0); // per ora, termino i processi - problemi di sync
		} else if(p == -1) {
			printf("Terminazione per meltdown.");
		}



		sleep(2);
		

	}
       	
       	int z = my_data->j;
       	printf("QUANTO VALE J: %d \n", z);
       	// scelta random del processo da scindere 
       	int b;
       	do {
       		// printf("IN WHILE \n");
       		// n = 1;
       		n = rand()%z+1; // n= id del atomo da scindere
       		// printf("ID ATOMO DA SCINDERE: %d \n", n);
       		// controllo che l'id ci sia - magari era una scoria che ho già tolto
        	for (b=1; b<=z; b++){
        		if(my_data->id[b]==n){
        			break;
        		}
        	}
        	       
        }while(b!=my_data->j); 
        
        sleep(1);
        // sciossione
		int num = 20;
        // int num = my_data->num_atomico[n]; // num atomico del padre
        printf("NUM: %d \n", num);
        
        if(num <= MIN_N_ATOMICO) {
        	// trovato scoria 
        	my_data->id[n] = -1; // togliamo l'id della scoria dalla shared memory     
			my_stat->scorie += 1;
        	// non posso diminuire j	
        	// exit();
        	        	
        } else {
        	printf("Effettua scissione\n");
        	printf("\n");

        	my_data->num_atomico[n] = num;// cambiamo il num_atomico del padre
        	printf("NUM POST: %d \n", num);

        	// fork
        	if(!fork()){
        		sleep(1);
				//gestione del numero dispari 
				if (num % 2 == 0) {
					num /= 2;
        			my_data->num_atomico[n] = num;// cambiamo il num_atomico del padre
				} else {
					num /= 2;
        			my_data->num_atomico[n] = num+1;// cambiamo il num_atomico del padre
				}

        		my_data->pid[my_data->j+=1] = getpid();
            	my_data->id[my_data->j] = my_data->j;
				my_data->num_atomico[my_data->j]=num; 
			// da riguardare da random: 
			// ci possono essere atomi con lo stesso numero atomico?
			printf("DATI NUOVO ATOMO: \n");
			printf("ID: %d \n",my_data->id[my_data->j]);
			printf("PID: %d \n",my_data->pid[my_data->j]);
			printf("NUM_ATOMICO: %d \n",my_data->num_atomico[my_data->j]);
			printf("\n");
			
			my_stat->num_scissioni += 1;	
			my_stat->energy = (num*num)-num;
			
			exit(0); // per ora, termino i processi - problemi di sync
        	}
        	my_stat->energy = (num*num)-num;
        }
       	
         
        
        
        // printf("*NUM_ATOMICO = %d \n", my_data->num_atomico[n]);
                
        //incrementiamo l'energia 
        
        
        
        printf("STAT POST SCISSIONE\n");
        printf("ENERGY: %d \n", my_stat->energy);
        printf("SCORIE: %d \n", my_stat->scorie);
        
        
        // chiudo le sm
    shmdt(my_stat);
	//shmctl(s_id, IPC_RMID, NULL); // deallocare la shared memory, viene cancellato dopo però
	
	shmdt(my_data);
	//shmctl(m_id, IPC_RMID, NULL);
	
	return 0;
}                
