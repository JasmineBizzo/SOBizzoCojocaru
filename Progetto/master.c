#include <signal.h> //consente l'utilizzo delle funzioni per la gestione dei segnali fra processi
#include <errno.h> //definisce errno, variabile impostata da system call e funzioni di libreria nel per indicare cosa e' andato storto
#include <stdlib.h>
#include <unistd.h> //definisce costanti e tipi simbolici vari e dichiara funzioni varie.
#include "shared-memory.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h> //senza questo non mi fa usare pid_t??
#include <sys/shm.h> //per shm

//so_days == sim_duration
int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO, STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION;
int ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD, avaliable_energy; //data dagli atomi, da cui prevelo energy_demand ogni sec

void check_in(); 
void secPrint(struct stat_data, int); //la stampa fatta ogni secondo (daily)
void finalReport(struct stat_data *, int);
void endSimulation(pid_t *); //uccido anche attivatore e alimentazione?

/*DA FINIRE
void handler(int signal){ // DEVO riportare la causa della terminazione
    switch (signal){
        case SIGABRT:
         printf("[SISTEMA] -> LA SIMULAZIONE E' TERMINATA\n");/////////
         //   flag =1;
        default:
        break;
    }
} */

int main(int argc, char * argv[]){
    int sem_config_id = 0;

    int i, l, n, m_id, s_id;
    struct stat_data * shm_stat; // tutte le statistiche
    struct shared_data * shm_data; // dati atomi

    pid_t * pidAtomi, pidAttivatore, pidAliment;
    pidAtomi = malloc(sizeof(pidAtomi) * N_ATOMI_INIT);
    pidAttivatore = malloc(sizeof(pidAttivatore));
    pidAliment= malloc(sizeof(pidAliment));

    char * args_atomi[]  = {"atomo.c", NULL};
    char * args_attivatore[] = {"attivatore.c", NULL};
    char * args_aliment[] = {"alimentazione.c", NULL};
    char argv_buffer[50];

/**
    sigset_t mask_block;
   struct sigaction sa;
    //int i, j, k, status, errno;
   // int sem_config_id = 0, sem_offerte_richieste_id = 0;
   // int * harborIndexNoRepeat, * allHarborIndex, * remeaningHarbor;
    pid_t * pidNavi, * pidPorti, pid_meteo;
   
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
    sigaction(SIGABRT, &sa, NULL);
*/

    check_in();

    /**SHARED MEMORY
     * parent pid + 1: Dati per la stampa ogni secondo
     * parent pid + 2: Pid e numero atomico degli atomi
    */

    s_id=shmget(getpid() + 1, sizeof(*shm_stat), 0600 | IPC_CREAT); //creazione area di memoria condivisa
	shm_stat=shmat(s_id, NULL, 0); // collegare l'area a un puntatore
	
	m_id=shmget(getpid() + 2, sizeof(*shm_data), 0600| IPC_CREAT); // sizeof(int)*n_atom_init*x ???
	shm_data=shmat(m_id, NULL, 0);
    
/** parent pid + 16: array dei porti che sono stati colpiti dalla mareggiata
    shm_merci_id = shmget(getpid() + 1, sizeof(Merce) * SO_MERCI, 0600 | IPC_CREAT);
    tipi_merci = shmat(shm_merci_id, NULL, 0);
    shm_richieste_id = shmget(getpid() + 2, sizeof(int) * ((SO_MERCI + 1)*SO_PORTI), 0600 | IPC_CREAT);
    arr_richieste = shmat(shm_richieste_id, NULL, 0);
    */

    /**
     * SEMAFORI
     * parent pid + 0: fine configuarazione iniziale, la simulazione parte solo dopo che tutto e' stato inizializzato
     * // CANC parent pid + 1: fine creazione matrice richieste/offerte dei porti
    */

// sem_config_id = semget(getpid(), 1, 0600 | IPC_CREAT);
   // sem_set_val(sem_config_id, 0, (SO_NAVI + SO_PORTI + 1));//////////////?? nella lib

//sem_offerte_richieste_id = semget(getpid() + 1, 1, 0600 | IPC_CREAT);
    //sem_set_val(sem_offerte_richieste_id, 0, 1);

   
    while(wait(NULL) != - 1);////??

   //inizio a salvare in shm i dati del master	
	shm_data->pid[0] = getpid();  // parent process - master
	shm_data->id[0] = 0; 
    shm_data->num_atomico[shm_data->j]=rand()%118+1; 
			// da riguardare da random: mettere N_ATOM_MAX
	shm_data->j=0; // inizializzo il puntatore della sh shm_data

    //Non so se questo serve
    shm_stat->energy = 0; 
	shm_stat->energy_used = 0;
	shm_stat->scorie = 0; 
	shm_stat->num_scissioni = 0; 
	shm_stat->num_attivazioni = 0;
	
	shm_stat->energy_sec = 0; 
	shm_stat->energy_used_sec = 0;
	shm_stat->scorie_sec = 0; 
	shm_stat->num_scissioni_sec = 0; 
	shm_stat->num_attivazioni_sec = 0;



    //Creazione degli n ATOMI iniziali

    for(i = 0; i < N_ATOMI_INIT; i++){
        switch (pidAtomi[i] = fork()){
            case -1:
                printf("C'è stato un errore nella scissione (fork) di un atomo: %s", strerror(errno));
                exit(-1); 
               //kill -KILL pidAtomi[i]; // vedi se funz
            case 0:
                shm_data->pid[shm_data->j+=1] = getpid();
                shm_data->id[shm_data->j] = shm_data->j;
			    shm_data->num_atomico[shm_data->j]=rand()%118+1;

                sprintf(argv_buffer, "%d", (i+1));
                args_atomi[1] = argv_buffer;
                execve("../bin/atomo", args_atomi, NULL); //???????? path args = 'atomo.c'
                exit(0); 
                break;
            default:
            break;
        }
    }   //  args_atomo[1] = " ";  ///??
    printf("[SISTEMA]\t -> \t TUTTI GLI ATOMI SONO PRONTI\n");
    
    //Generazione processo ATTIVATORE
    
   switch (pidAttivatore = fork()){
        case -1:
            printf("C'è stato un errore nel fork per il processo attivatore: %s", strerror(errno));
              exit(-1); 
               //kill -KILL pidAtomi[i]; // vedi se funz
            break;
         case 0:      
            execve("../bin/attivatore", args_attivatore, NULL); //?????????? path args = 'attivatore.c'
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
               //kill -KILL pidAtomi[i]; // vedi se funz
            break;
        case 0:
            execve("../bin/alimentazione", args_aliment, NULL); // args = 'alimentazione.c'
            exit(0);
        default:
        break;
    }
    printf("[SISTEMA]\t -> \t IL PROCESSO ALIMENTAZIONE E' PRONTO\n");
    
    printf("[SISTEMA]\t -> \t INIZIO DELLA SIMULAZIONE\n");

    int i = 0; //indice dei secondi che passano
    int k = 0;
   
    while(semctl(sem_config_id, 0, GETVAL) != 0); //semaforo a noi serve??
    * porti_selezionati = SO_PORTI;
    for(i = 0; i < SO_PORTI; i++){
        kill(pidPorti[i], SIGUSR1);
    }
    i = 0;


    while(i != SIM_DURATION){ //SE SERVE metto delle flags qui per interrompere la sim
        //cambio so_days in sim_duration
       
    /** FAI ENERGY DEMAND
     * Prima ricevo l'energia liberata dagli atomi, poi prelevo
     * 
     * 
    */

    /* o passo questi uno per uno? xo non sto usando la sm
    int energy; // energia creata
	int energy_used; // energia consumata
	int scorie; 
	int num_scissioni; 
	int num_attivazioni;
	
	
	int energy_sec; // energia creata nell'ultimo secondo
	int energy_used_sec; // energia consumata nell'ultimo sec
	int scorie_sec; 
	int num_scissioni_sec; 
	int num_attivazioni_sec;*/


        secPrint(shm_stat, i); // PARAMETRI DA CAPIRE


        for(x = 1; x <= SO_MERCI; x++){
            for(y = 0; y < SO_PORTI; y++){
                checkEndOffers += arr_offerte_global[y * (SO_MERCI + 1) + x];
                checkEndRequests += arr_richieste_global[y * (SO_MERCI + 1) + x];
            }
        }
        if(checkEndOffers == 0 || checkEndRequests == 0){
            flagEndMaterials = 0;
        }
        
        free(harborIndexNoRepeat);
        if(SO_PORTI > 4 && * porti_selezionati != SO_PORTI){
            free(remeaningHarbor);
        }
        printf("[SISTEMA]\t -> \t IL TEMPO DI SIMULATIONE E' TERMINATO\n");
    }
    
    //Fine sumulazione, invio un SIGABRT a tutti i processi indicandogli di terminare
    endSimulation(shmPidPorti, shmPidNavi, pid_meteo);

    while(wait(NULL) != -1);
    
    finalReport(shm_stat, i);
    
    //Faccio detach e dealloco tutte le variabili delle shared memories
    shmdt(shm_stat);
	shmdt(shm_data);
  
    shmctl(s_id, IPC_RMID, NULL); 
    shmctl(m_id, IPC_RMID, NULL);

    //semafori
    semop(sem_config_id, NULL, IPC_RMID);
    semop(sem_offerte_richieste_id, NULL, IPC_RMID);
    
    //malloc
    free(pidNavi);
    free(pidPorti);
    free(allHarborIndex);

    printf("[SISTEMA] -> SIMULAZIONE TERMINATA\n");

    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_UNBLOCK, &mask_block, NULL);
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
void secPrint(struct stat_data shm_stat, int sec){
    int i, n, m_id, s_id, p;
	struct stat_data * shm_stat;
    s_id=shmget(getpid() + 1, sizeof(*shm_stat), 0600 | IPC_CREAT); //creazione area di memoria condivisa
	shm_stat=shmat(s_id, NULL, 0); // collegare l'area a un puntatore
	
	
 //prime 4 righe non so che sono ma per ora lascio e non so cosa sono i numeri dopo la quadra
    printf("\033[0;33m");
    printf("----------------------------------------------------------------------------------------------\n");
    printf("\033[0m");
    printf("\033[1;31m");
    printf("SECONDO\t->\t[%d]\n", sec);
    printf("\033[0m");
    printf("\n");
    printf("LE ATTIVAZIONI NELL\'ULTIMO SECONDO SONO STATE\t->\t[\033[1;32m%d\033[0m]\n", shm_stat.num_attivazioni_sec);
    printf("LE ATTIVAZIONI TOTALI SONO STATE\t->\t[\033[1;32m%d\033[0m]\n", shm_stat.num_attivazioni);
    printf("\n");
    printf("LE SCISSIONI NELL\'ULTIMO SECONDO SONO STATE\t->\t[\033[1;32m%d\033[0m]\n", shm_stat.num_scissioni_sec);
    printf("LE SCISSIONI TOTALI SONO STATE\t->\t[\033[1;32m%d\033[0m]\n", shm_stat.num_scissioni);
    printf("\n");
    printf("L\'ENERGIA PRODOTTA NELL\'ULTIMO SECONDO E\' STATA\t->\t[\033[1;32m%d\033[0m]\n", shm_stat.energy_sec);
    printf("L\'ENERGIA PRODOTTA TOTALE E\' \t->\t[\033[1;32m%d\033[0m]\n", shm_stat.energy);
    printf("\n");
    printf("L\'ENERGIA CONSUMATA NELL\'ULTIMO SECONDO E\' STATA\t->\t[\033[2;32m%d\033[0m]\n", shm_stat.energy_used_sec);
    printf("L\'ENERGIA CONSUMATA TOTALE E\' \t->\t[\033[2;32m%d\033[0m]\n", shm_stat.energy_used);
    printf("\n");
    printf("LE SCORIE PRODOTTE NELL\'ULTIMO SECONDO SONO STATE\t->\t[\033[0;32m%d\033[0m]\n", shm_stat.scorie_sec);
    printf("LE SCORIE PRODOTTE TOTALI SONO STATE\t->\t[\033[0;32m%d\033[0m]\n", shm_stat.scorie);
    printf("\n");
    printf("\033[0;33m");
    printf("----------------------------------------------------------------------------------------------\n");
    printf("\033[0m");

    //Poi azzero le statistiche _sec
    shm_stat.energy_sec = 0; 
	shm_stat.energy_used_sec = 0;
	shm_stat.scorie_sec = 0; 
	shm_stat.num_scissioni_sec = 0; 
	shm_stat.num_attivazioni_sec = 0;
}

void finalReport(struct stat_data shm_stat, int sec){
    int carico = 0;
    int noCarico = 0;
    int operandoPorto = 0;
    int naviAbbattute = 0;
    int naviBloccate = 0;
    int merciTot = 0;
    int i;

    printf("\033[0;36m");
    printf("\n\n\n\n\n\n----------------------------------------------------------------------------------------------\n\n[SISTEMA]\t->\tREPORT FINALE\n\n");
    printf("\033[0m");
    printf("[SISTEMA] -> LA SIMULAZIONE E' DURATA %d GIORNI\n\n", sec);
    if(!flagEndMaterials){
        printf("[SISTEMA] -> TUTTE LE RICHIESTE SODDISFATTE OPPURE L'OFFERTA E' PARI A 0\n");
    }
    if(flagEndMaelstrom){
        printf("[SISTEMA] -> TUTTE LE NAVI SONO STATE COLPITE DAL MAELSTROM\n");
    }
    for(i = 0; i < SO_NAVI; i++){
        noCarico += statusNavi[(i * 6) + 1];
        carico += statusNavi[(i * 6) + 2];
        operandoPorto += statusNavi[(i * 6) + 3];
        naviAbbattute += statusNavi[(i * 6) + 4];
        naviBloccate += statusNavi[(i * 6) + 5];
    }
    printf("NAVI IN VIAGGIO SENZA CARICO\t->\t[\033[1;32m%d\033[0m]\n", noCarico);
    printf("NAVI IN VIAGGIO CON CARICO\t->\t[\033[1;32m%d\033[0m]\n", carico);
    printf("NAVI OPERANDO PRESSO PORTI\t->\t[\033[1;32m%d\033[0m]\n", operandoPorto);
    printf("NAVI CHE SONO STATE AFFONDATE\t->\t[\033[2;31m%d\033[0m]\n", naviAbbattute);
    printf("NAVI BLOCCATE DA STORM\t\t->\t[\033[0;31m%d\033[0m]\n", naviBloccate);
    printf("\n");
    printf("PORTI BLOCCATI DA SWELL\n");
    for(i = 0; i < SO_DAYS; i++){
        printf("[PORTO\t\033[0;31m%d\033[0m]\n", portiSwell[i]);
    }
    printf("\n");
    for(i = 0; i < SO_MERCI; i++){
        merciTot = statusMerci[(i * 5)] + statusMerci[(i * 5) + 1] + statusMerci[(i * 5) + 2] +  statusMerci[(i * 5) + 3] + statusMerci[(i * 5) + 4];
        printf("Merce -> %d: totale generata %d, di cui: presente in porto->%d\tpresente in nave->%d\tconsegnata in porto->%d\tscaduta in porto->%d scaduta in nave->%d\n", (i + 1),merciTot, statusMerci[(i*5)], statusMerci[(i*5)+1], merci_consegnate[i], statusMerci[(i*5)+3], statusMerci[(i*5)+4]);
        printf("\033[2;37m");
        printf("Porto che ne ha offerta di più -> %d, Porto che ne ha richiesta di più -> %d\n\n", maxOfferte[(i*2)], maxRichieste[(i*2)]);
        printf("\033[0m");
    }
    printf("\n");
    for(i = 0; i < SO_PORTI; i++){
         if(i%2 == 0){
            printf("\033[1;32m");
        }
        else{
            printf("\033[1;35m");
        }
        printf("[PORTO -> %d] Merci:\t presenti->%d\tricevute->%d\tspedite->%d\tbanchine->\tlibere %d su %d totali\n", statusPorti[(i * 6)], statusPorti[(i * 6) + 1], statusPorti[(i * 6) + 2], statusPorti[(i * 6) + 3], statusPorti[(i * 6) + 4], statusPorti[(i * 6) + 5]);
        printf("\033[0m");
    }
    printf("\033[0;36m");
    printf("----------------------------------------------------------------------------------------------\n");
    printf("\033[0m");
}

void endSimulation (pid_t * pidAtomi){ //qui devo uccidere tutti i processi, forse dobbiamo tenere conte del numero TOTALE atomi
                        //devo killare anche alimentazione e attivatore?
    int i;
    if(pidAtomi!= NULL){
        for (i = 0; i < N_ATOM_INIT; i++){
            if(pidAtomi[i] != -1){
                kill(pidAtomi[i], SIGABRT);
            }
        }
    }
}
