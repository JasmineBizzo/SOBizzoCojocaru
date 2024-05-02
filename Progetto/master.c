#include <signal.h> //utilizzo delle funzioni per la gestione dei segnali fra processi
#include <errno.h> //errno, variabile per indicare cosa e' andato storto
#include <stdlib.h>
#include <unistd.h> //costanti e tipi simbolici vari e dichiara funzioni varie.
//#include "shared-memory.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h> //per pid_t
#include <sys/shm.h> //per shm
#include <sys/sem.h> //per sem

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

void check_in();
void secPrint(int *, int);
void endSimulation(int*, pid_t *, pid_t *, int, int *, int *, int, int, int, int);
// VECCHIO endSimulation(int*, pid_t *, pid_t *, int);

//DA FINIRE e CAPIRE
void handler(int signal){
    switch (signal){
        case SIGABRT:
         printf("\n");/////////
         //   flag =1;
         case SIGTERM:
         endSimulation();
        default:
        break;
    }
} 

int main(int argc, char * argv[]){
    int sem_config_id = 0;

    int i, l, n;

    pid_t pidAttivatore, pidAliment; 
    //pidAtomi = malloc(sizeof(pidAtomi) * N_ATOMI_INIT); Non lo uso più, uso shm
    pidAttivatore = malloc(sizeof(pidAttivatore));
    pidAliment= malloc(sizeof(pidAliment));

    char * args_atomi[]  = {"atomo.c", NULL};
    char * args_attivatore[] = {"attivatore.c", NULL};
    char * args_aliment[] = {"alimentazione.c", NULL};
    char argv_buffer[50];

    /**  sigset_t mask_block;
    struct sigaction sa;
    int i, j, k, status, errno;
    int * harborIndexNoRepeat, * allHarborIndex, * remeaningHarbor;
    int flag = 1;
    int flagDecision = 0;
    char argv_buffer[50];
    char * args_navi[]  = {"navi.c", NULL};
    struct timespec req, rem;
    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_BLOCK, &mask_block, NULL);
    s(getpid());
    bzero(&sa, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGABRT, &sa, NULL);  */

    check_in();

    /**SHARED MEMORY
     * parent pid + 1: array dei dati necessari per la stampa shm_stat[i]
     * parent pid + 2: array dei pid 
     * parent pid + 3: array dei numeri atomici
     * 
     * shm_stat[0] = int energy; 
	 * shm_stat[1] = int energy_sec;
     * shm_stat[2] = int energy_used; 
	 * shm_stat[3] = int energy_used_sec;
     * shm_stat[4] = int scorie; 
     * shm_stat[5] = int scorie_sec; 
     * shm_stat[6] = int num_scissioni; 
     * shm_stat[7] = int num_scissioni_sec; 
     * shm_stat[8] = int num_attivazioni;
	 * shm_stat[9] = int num_attivazioni_sec;
     * 
     * shm_stat[10] = energia generata dagli atomi, da cui il master preleva l'Energy Demand;
    */

    int num_atom_tot = N_ATOMI_INIT + (N_NUOVI_ATOMI*(SIM_DURATION/STEP_ALIMENTAZIONE));

    int stat_id, /*atom_id,*/ * atom_pid_id, atom_num_id;
    int * shm_stat, /* * shm_atom_id,*/ * shm_atom_pid, * shm_atom_num;

    stat_id = shmget(getpid() + 1, sizeof(int) *11, 0600 | IPC_CREAT); //creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0); //collegare l'area a un puntatore
	
	//atom_id = shmget(getpid() + 2, sizeof(int) * num_atom_tot, 0600 | IPC_CREAT); ///size?????
	//shm_atom_id = shmat(atom_id, NULL, 0);

    atom_pid_id = shmget(getpid() + 2, sizeof(int) * num_atom_tot, 0600 | IPC_CREAT); ///size????
	shm_atom_pid = shmat(atom_pid_id, NULL, 0);

    atom_num_id = shmget(getpid() + 3, sizeof(int) * num_atom_tot, 0600 | IPC_CREAT); //size?
	shm_atom_num = shmat(atom_num_id, NULL, 0);

    /**
     * SEMAFORI
     * parent pid + 0: fine configuarazione iniziale, la simulazione parte solo dopo che tutto e' stato inizializzato
     * // CANC parent pid + 1: fine creazione matrice richieste/offerte dei porti
    */

    sem_config_id = semget(getpid(), 1, 0600 | IPC_CREAT);
    sem_set_val(sem_config_id, 0, (N_ATOMI_INIT + 2));
    //Valore = il num di p atomo che creo inizialmente + 1 per il p attivatore + 1 per il p alimentazione
    //Quando il valore sarà 0, ovvero tutti i p saranno stati creati, inizierà la simulazione

    while(wait(NULL) != - 1);////??

   //Non essendo il master un atomo, questo probably non serve: Inizializzazione shm con pid del master
	shm_atom_pid[0] = getpid(); 
    shm_atom_num[0]= 0;
			

    //Creazione degli n ATOMI iniziali
    for(i = 0; i < N_ATOMI_INIT; i++){
       // int count = 0;
        switch (shm_atom_pid[i] = fork()){
            case -1:
                printf("C'è stato un errore nella scissione (fork) di un atomo: %s", strerror(errno));
                exit(-1); 
                //kill (pid, SIGABORT) 
            case 0:
                shm_atom_pid[i] = getpid(); // è giusto??
                shm_atom_num[i]=rand()%118+1;
                        ///Cambia da random a max num atomico!!
                sprintf(argv_buffer, "%d", (i+1));
                args_atomi[1] = argv_buffer;
                execve("../bin/atomo", args_atomi, NULL); //???????? args atomi = pid del master
                exit(0); 
                break;
            default:
            break;
        }
        //count ++;
    }   //  args_atomo[1] = " ";  ///??
    printf("[SISTEMA]\t -> \t TUTTI GLI ATOMI SONO PRONTI\n");
    
    //Generazione processo ATTIVATORE
    switch (pidAttivatore = fork()){
        case -1:
            printf("C'è stato un errore nel fork per il processo attivatore: %s", strerror(errno));
              exit(-1); 
               //fai
            break;
         case 0:      
            execve("../bin/attivatore", args_attivatore, NULL); //?????????? path args = 'attivatore.c' o pid master
            exit(- 1);
            break;
            default:
            break;
    }
    printf("[SISTEMA]\t -> \t IL PROCESSO ATTIVATORE E' PRONTO\n");

    
    //Generazione processo ALIMENTAZIONE 
    switch(pidAliment = fork()){
        case -1:
            printf("C'è stato un errore nel fork per il processo alimentazione: %s", strerror(errno));
            exit(-1); 
               //fai
            break;
        case 0:
            execve("../bin/alimentazione", args_aliment, NULL); // args = 'alimentazione.c' o pid master
            exit(0);
        default:
        break;
    }
    printf("[SISTEMA]\t -> \t IL PROCESSO ALIMENTAZIONE E' PRONTO\n");
    

    printf("[SISTEMA]\t -> \t INIZIO DELLA SIMULAZIONE\n");

    while(semctl(sem_config_id, 0, GETVAL) != 0); //Rimango qui fin quando il semaforo non è a 0,
                                                //ovvero fin quando non sono stati creati tutti i processi
    int i = 0; //indice dei secondi che passano
    while(i != SIM_DURATION){ //SE SERVE metto delle flags qui per interrompere la sim
       //prima attendo con un semaforo che tutti gli atomi si siano scissi
       
    //PRELIEVO DELL'ENERGY DEMAND
        shm_stat[10] = shm_stat[10] - ENERGY_DEMAND;
        if(shm_stat[10] > ENERGY_EXPLODE_THRESHOLD){
            printf("[SISTEMA]\t -> \t EXPLODE: L \'ENERGIA LIBERATA E\' MAGGIORE DEL THRESHOLD\n");
            //kill
        }
        else if(shm_stat[10] < 0){//è corretto leggendo la consegna???
            printf("[SISTEMA]\t -> \t BLACKOUT: L \'ENERGIA CHE SI VUOLE PRELEVARE NON E\' DISPONIBILE\n");
            //kill
        }
            
        secPrint(shm_stat, i);
        i ++;
    }
    printf("[SISTEMA]\t -> \t IL TEMPO DI SIMULATIONE IMPOSTATO E' TERMINATO\n");

    //Fine sumulazione, invio un SIGABRT a tutti i processi indicandogli di terminare
    endSimulation(shm_atom_pid, pidAttivatore, pidAliment, num_atom_tot,    
                    shm_stat, shm_atom_num, stat_id, atom_pid_id, atom_num_id,
                    sem_config_id);
    // VECCHIO endSimulation(shm_atom_pid, pidAttivatore, pidAliment, num_atom_tot);

    while(wait(NULL) != -1);
    
    //SPOSTO per facilitare in caso di Handler - Terminazione, da vedere se è giusto!
    /*
    //Faccio detach e dealloco tutte le variabili delle shared memories
    shmdt(shm_stat);
	shmdt(shm_atom_pid);
    shmdt(shm_atom_num);
    
    shmctl(stat_id, IPC_RMID, NULL); 
    shmctl(atom_pid_id, IPC_RMID, NULL);
    shmctl(atom_num_id, IPC_RMID, NULL);

    //semafori
    semop(sem_config_id, NULL, IPC_RMID);
    //semop(sem_offerte_richieste_id, NULL, IPC_RMID);
    
    //malloc
    free(pidAttivatore);
    free(pidAliment);
    */
    
    printf("[SISTEMA] -> SIMULAZIONE TERMINATA CON SUCCESSO\n");

    /*??????????????????
    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_UNBLOCK, &mask_block, NULL);*/
}

//Controllo iniziale sui parametri inseriti nel file input
void check_in(){
    const char * file_path = "../src/env_var.txt"; ////PATH!
    char buffer[1024]; //size
    //char buffer_cpy[1024];
    //ssize_t bytes_read;
    char * token;
    FILE * file_descriptor = fopen(file_path, "r");

    if (!file_descriptor) {
        perror("Failed to open the file");
    }
    while (fgets(buffer, 1024, file_descriptor)){
        token = strtok(buffer, " ");
    
        if(strcmp(token, "ENERGY_DEMAND") == 0){
            token = strtok(NULL, " ");
            ENERGY_DEMAND = atoi(token);
        }else if(strcmp(token, "N_ATOMI_INIT") == 0){
            token = strtok(NULL, " ");
            N_ATOMI_INIT = atoi(token);
        }else if(strcmp(token, "MIN_N_ATOMICO") == 0){
            token = strtok(NULL, " ");
            MIN_N_ATOMICO = atoi(token);
        }else if(strcmp(token, "STEP_ATTIVATORE") == 0){
            token = strtok(NULL, " ");
            STEP_ATTIVATORE = atoi(token);
        }else if(strcmp(token, "STEP_ALIMENTAZIONE") == 0){
            token = strtok(NULL, " ");
            STEP_ALIMENTAZIONE = atoi(token);
        }else if(strcmp(token, "N_NUOVI_ATOMI") == 0){
            token = strtok(NULL, " ");
            N_NUOVI_ATOMI = atoi(token);
        }else if(strcmp(token, "SIM_DURATION") == 0){
            token = strtok(NULL, " ");
            SIM_DURATION = atoi(token);
        }else if(strcmp(token, "ENERGY_EXPLODE_THRESHOLD") == 0){
            token = strtok(NULL, " ");
            ENERGY_EXPLODE_THRESHOLD = atoi(token);
        }
    }
    fclose(file_descriptor);

    /*  switch(buffer[1024]){
            case (strcmp(token, "ENERGY_DEMAND") == 0):
            token = strtok(NULL, " ");
            ENERGY_DEMAND = atoi(token);
            break;

            case (strcmp(token, "N_ATOMI_INIT") == 0):
            token = strtok(NULL, " ");
            N_ATOMI_INIT = atoi(token);
            break;

            case strcmp(token, "MIN_N_ATOMICO") == 0):
            token = strtok(NULL, " ");
            MIN_N_ATOMICO = atoi(token);
            break;

            case strcmp(token, "STEP_ATTIVATORE") == 0):
            token = strtok(NULL, " ");
            STEP_ATTIVATORE = atoi(token);
            break;

            case strcmp(token, "STEP_ALIMENTAZIONE") == 0):
            token = strtok(NULL, " ");
            STEP_ALIMENTAZIONE = atoi(token);
            break;

            case strcmp(token, "N_NUOVI_ATOMI") == 0):
            token = strtok(NULL, " ");
            N_NUOVI_ATOMI = atoi(token);
            break;

            case strcmp(token, "SIM_DURATION") == 0):
            token = strtok(NULL, " ");
            SIM_DURATION = atoi(token);
            break;

            case strcmp(token, "ENERGY_EXPLODE_THRESHOLD") == 0):
            token = strtok(NULL, " ");
            ENERGY_EXPLODE_THRESHOLD = atoi(token);
            break;
        }*/

    if(ENERGY_DEMAND < 0){
        printf("[SISTEMA]\t -> \t ERRORE: IL NUMERO DI ENERGY_DEMAND DEVE ESSERE MAGGIORE DI 0\n");
        exit(- 1);
    }
    else if(N_ATOMI_INIT < 0){
        printf("[SISTEMA]\t -> \t ERRORE: IL NUMERO DI ATOMI INIZIALE DEVE ESSERE MAGGIORE DI 0\n");
        exit(- 1);
    }
    else if(MIN_N_ATOMICO < 1 || MIN_N_ATOMICO > 118){
        printf("[SISTEMA]\t -> \t ERRORE: IL NUMERO ATOMICO DEVE ESSERE COMPRESO TRA 1 E 118\n");
        exit(- 1);
    }
    else if(STEP_ATTIVATORE > 0){
        printf("[SISTEMA]\t -> \t ERRORE: LO STEP ATTIVATORE NON PUO' NON ESSERE PRESENTE\n");
        exit(- 1);
    }
    else if(STEP_ALIMENTAZIONE > 0){
        printf("[SISTEMA]\t -> \t ERRORE: LO STEP ALIMENTAZIONE NON PUO' NON ESSERE PRESENTE\n");
        exit(- 1);
    }
    else if(N_NUOVI_ATOMI < 0){
        printf("[SISTEMA]\t -> \t ERRORE: IL NUMERI DI NUOVI ATOMI DA CREARE NON PUO' ESSERE 0\n");
        exit(- 1);
    }
    else if(SIM_DURATION < 0){
        printf("[SISTEMA]\t -> \t ERRORE: LA SIMULAZIONE NON PUO' DURARE 0 SECONDI\n");
        exit(- 1);
    }
    else if(ENERGY_EXPLODE_THRESHOLD < 0){
        printf("[SISTEMA]\t -> \t ERRORE: IL THRESHOLD NON PUO' ESSERE 0\n");
        exit(- 1);
    }
}

//Eseguo la print ogni secondo
void secPrint(int * shm_stat, int sec){
    printf("\033[0;33m");
    printf("------------------------------------------------------------------------------------\n");
    //printf("\033[0m");
    //printf("\033[1;31m");
    printf("AL SECONDO\t->\t[%d]\n", sec);
    printf("\033[0m");
    printf("\n");
    printf("LE ATTIVAZIONI NELL\'ULTIMO SECONDO SONO STATE\t->\t[\033[1;32m%d\033[0m]\n", shm_stat[9]);
    printf("LE ATTIVAZIONI TOTALI SONO STATE\t->\t[\033[1;32m%d\033[0m]\n", shm_stat[8]);
    printf("\n");
    printf("LE SCISSIONI NELL\'ULTIMO SECONDO SONO STATE\t->\t[\033[1;32m%d\033[0m]\n", shm_stat[7]);
    printf("LE SCISSIONI TOTALI SONO STATE\t->\t[\033[1;32m%d\033[0m]\n", shm_stat[6]);
    printf("\n");
    printf("L\'ENERGIA PRODOTTA NELL\'ULTIMO SECONDO E\' STATA\t->\t[\033[1;32m%d\033[0m]\n", shm_stat[1]);
    printf("L\'ENERGIA PRODOTTA TOTALE E\' \t->\t[\033[1;32m%d\033[0m]\n", shm_stat[0]);
    printf("\n");
    printf("L\'ENERGIA CONSUMATA NELL\'ULTIMO SECONDO E\' STATA\t->\t[\033[2;32m%d\033[0m]\n", shm_stat[3]);
    printf("L\'ENERGIA CONSUMATA TOTALE E\' \t->\t[\033[2;32m%d\033[0m]\n", shm_stat[2]);
    printf("\n");
    printf("LE SCORIE PRODOTTE NELL\'ULTIMO SECONDO SONO STATE\t->\t[\033[0;32m%d\033[0m]\n", shm_stat[5]);
    printf("LE SCORIE PRODOTTE TOTALI SONO STATE\t->\t[\033[0;32m%d\033[0m]\n", shm_stat[4]);
    printf("\n");
    printf("\033[0;33m");
    printf("------------------------------------------------------------------------------------\n");
    printf("\033[0m");

    //Poi azzero le statistiche relative all'ultimo secondo _sec
    shm_stat[1]= 0; 
	shm_stat[3]= 0;
	shm_stat[5]= 0; 
	shm_stat[7] = 0; 
	shm_stat[9] = 0;
}

//Invio un SIGABRT a tutti i processi indicandogli di terminare
void endSimulation (int * shm_atom_pid, pid_t * pidAttivatore, pid_t * pidAliment, int num_atom_tot,
                    int * shm_stat, int * shm_atom_num, int stat_id, int atom_pid_id, int atom_num_id,
                    int sem_config_id){ 
                                 //devo killare anche alimentazione e attivatore giusto?
    int i;
    if(shm_atom_pid[i]!= NULL){ //Riga qui sott: lascio così o salviamo in shm il num Totale di atomi creato?
        for (i = 0; i < num_atom_tot; i++){  
            if(shm_atom_pid[i] != -1){
                kill(shm_atom_pid[i], SIGABRT);
            }
        }
    }

    kill(pidAttivatore, SIGABRT);

    kill(pidAliment, SIGABRT);

    //Faccio detach e dealloco tutte le variabili delle shared memories
    shmdt(shm_stat);
	shmdt(shm_atom_pid);
    shmdt(shm_atom_num);
    
    shmctl(stat_id, IPC_RMID, NULL); 
    shmctl(atom_pid_id, IPC_RMID, NULL);
    shmctl(atom_num_id, IPC_RMID, NULL);

    //semafori
    semop(sem_config_id, NULL, IPC_RMID);
    //semop(sem_offerte_richieste_id, NULL, IPC_RMID);
    
    //malloc
    free(pidAttivatore);
    free(pidAliment);
    
}
