#include <stdio.h>
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
	struct stat_data * shm_stat;
	struct shared_data * shm_data;
	
	s_id=shmget(getpid() + 1, sizeof(*shm_stat), 0600 | IPC_CREAT); //creazione area di memoria condivisa
	shm_stat=shmat(s_id, NULL, 0); // collegare l'area a un puntatore
	
	m_id=shmget(getpid() + 2, sizeof(*shm_data), 0600 | IPC_CREAT); //creazione area di memoria condivisa
	shm_data=shmat(m_id, NULL, 0); // collegare l'area a un puntatore
	
	// inizio a salvare in sm i dati del master	
	shm_data->pid[0] = getpid();  // parent process - master
	shm_data->id[0] = 0; 
    shm_data->num_atomico[0]=150;
	
	shm_data->j=0; // inizializzo il puntatore della sh shm_data
		
		printf("*ID: %d \n",shm_data->id[0]);
        printf("*PID: %d \n",shm_data->pid[0]);
        printf("*NUM_ATOMICO %d \n", shm_data->num_atomico[0]);
        printf("\n");
	
	// inizializzo i valori della sm shm_stat
		shm_stat->energy = 100;  
        shm_stat->scorie = 0;
        
        printf("STAT PROVVISORIE\n");
        printf("ENERGY: %d \n", shm_stat->energy);
        printf("SCORIE: %d \n", shm_stat->scorie);
       	printf("\n");
       	
       	// crea tot processi e salva i loro dati in sm
		// si potrebbe fare (tot_processi_da_creare /2) visto che dovrebbero bastare la metà delle fork
       	for(i=0; i<2; i++) { 
	


		//aggiungere il controllo se fork fallisce -> termina simulazione
		if(p = fork() == 0){ // children processes - atomo 
			
			sleep(1);
			// sops.sem_op = -1;
			// semop(s_id, &sops, 1);
			
			shm_data->pid[shm_data->j+=1] = getpid();
            shm_data->id[shm_data->j] = shm_data->j;
			shm_data->num_atomico[shm_data->j]=rand()%118+1; 
			// da riguardare da random: mettere N_ATOM_MAX
			
			
			printf("ID: %d \n",shm_data->id[shm_data->j]);
			printf("PID: %d \n",shm_data->pid[shm_data->j]);
			printf("NUM_ATOMICO: %d \n",shm_data->num_atomico[shm_data->j]);
			printf("\n");
			exit(0); // per ora, termino i processi - problemi di sync
		} else if(p == -1) {
			printf("Terminazione per meltdown.");
		}



		sleep(2);
		
	}
       	
       	int z = shm_data->j;
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
        		if(shm_data->id[b]==n){
        			break;
        		}
        	}
        	       
        }while(b!=shm_data->j); 
        
        sleep(1);
        // sciossione
		int num = 20;
        // int num = shm_data->num_atomico[n]; // num atomico del padre
        printf("NUM: %d \n", num);
        
        if(num <= MIN_N_ATOMICO) {
        	// trovato scoria 
        	shm_data->id[n] = -1; // togliamo l'id della scoria dalla shared memory     
			shm_stat->scorie += 1;
        	// non posso diminuire j	
        	// exit();
        	        	
        } else {
        	printf("Effettua scissione\n");
        	printf("\n");

        	shm_data->num_atomico[n] = num;// cambiamo il num_atomico del padre
        	printf("NUM POST: %d \n", num);

        	// fork
        	if(!fork()){
        		sleep(1);
				//gestione del numero dispari 
				if (num % 2 == 0) {
					num /= 2;
        			shm_data->num_atomico[n] = num;// non  cambiamo il num_atomico del padre
				} else {
					num /= 2;
        			shm_data->num_atomico[n] = num+1;// cambiamo il num_atomico del padre
				}

        		shm_data->pid[shm_data->j+=1] = getpid();
            	shm_data->id[shm_data->j] = shm_data->j;
				shm_data->num_atomico[shm_data->j]=num; 
			// da riguardare da random: 
			// ci possono essere atomi con lo stesso numero atomico?
			printf("DATI NUOVO ATOMO: \n");
			printf("ID: %d \n",shm_data->id[shm_data->j]);
			printf("PID: %d \n",shm_data->pid[shm_data->j]);
			printf("NUM_ATOMICO: %d \n",shm_data->num_atomico[shm_data->j]);
			printf("\n");
			
			shm_stat->num_scissioni += 1;	
			shm_stat->energy = (num*num)-num;
			
			exit(0); // per ora, termino i processi - problemi di sync
        	}
        	shm_stat->energy = (num*num)-num;
        }
       	
         
        
        
        // printf("*NUM_ATOMICO = %d \n", shm_data->num_atomico[n]);
                
        //incrementiamo l'energia 
        
        
        
        printf("STAT POST SCISSIONE\n");
        printf("ENERGY: %d \n", shm_stat->energy);
        printf("SCORIE: %d \n", shm_stat->scorie);
        
        
        // chiudo le sm
    shmdt(shm_stat);
	//shmctl(s_id, IPC_RMID, NULL); // deallocare la shared memory, viene cancellato dopo però
	shmdt(shm_data);
	//shmctl(m_id, IPC_RMID, NULL);
	
	return 0;
}             
