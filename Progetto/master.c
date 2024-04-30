#include <signal.h> //consente l'utilizzo delle funzioni per la gestione dei segnali fra processi
#include <errno.h> /*definisce la variabile intera errno, che e' impostato dalle system call e da alcune funzioni di libreria nel
                   file evento di errore per indicare cosa e' andato storto*/
#include <stdlib.h>
#include <unistd.h> // definisce costanti e tipi simbolici vari e dichiara funzioni varie.
#include "shared-memory.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>

//so_days == sim_duration
int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO, STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION;
int ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD, avaliable_energy; //data dagli atomi, da cui prevelo energy_demand ogni sec

void check_inputs(); //???? lascio?
void secPrint(int *, int *, int *, int *, int); //la stampa fatta ogni secondo (daily)
void finalReport(int *, int *, int *, int *, int *, int *, int *, int);
void endSimulation(pid_t *, pid_t *, pid_t);

struct shared_data {
	int j;
        int id[100];
        pid_t pid[100];
	int num_atomico[100]; 
};

/*DA FINIRE?
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
int shm_pidAtomi_id;
struct shared_data * shmPidAtomi;

int sem_config_id = 0;

int i, n, m_id, s_id;
	struct stat_data * my_stat; // energy e scorie
	struct shared_data * my_data; // dati atomi

char * args_atomi[]  = {"atomo.c", NULL};
char * args_pAtt[] = {"attivatore.c", NULL};
char * args_pAlim[] = {"alimentazione.c", NULL};

/**
  //  int ;
  //  int * ;
   // double * ;

    //sigset_t mask_block;
    
   // struct sigaction sa;
    //int i, j, k, z, x, y, status, errno;
    
   // int sem_config_id = 0, sem_offerte_richieste_id = 0;
   // int * harborIndexNoRepeat, * allHarborIndex, * remeaningHarbor;

   pid_t * pidNavi, * pidPorti, pid_meteo;
   
    int porto_scelto; 
    int flag = 1;
    int flagDecision = 0;
    char argv_buffer[50];

    char * args_navi[]  = {"navi.c", NULL};
    char * args_porti[] = {"porti.c", NULL};
    char * args_meteo[] = {"meteo.c", NULL};
    char * args_merci[] = {"merci.c", NULL};

    struct timespec req, rem;
    

    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_BLOCK, &mask_block, NULL);

    srand(getpid());

    bzero(&sa, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGABRT, &sa, NULL);
*/
    check_inputs();

            //pidAtomi = malloc(sizeof(pidAtomi) * N_ATOMI_INIT);

 
    //array che coniene tutti i pid degli atomi, parent pid + 1
   // shm_pidAtomi_id = shmget(getpid() + 1, sizeof(pid_t) * N_ATOMI_INIT, 0600 | IPC_CREAT);
     //shmPidAtomi = shmat(shm_pidAtomi_id, NULL, 0); 

    /**SHARED MEMORY
     * parent pid + 1: Dati per la stampa ogni secondo
     * parent pid + 2: Pid e numero atomico degli atomi
    */

    s_id=shmget(getpid() + 1, sizeof(*my_stat), 0600| IPC_CREAT); //creazione area di memoria condivisa
	my_stat=shmat(s_id, NULL, 0); // collegare l'area a un puntatore
	
	m_id=shmget(getpid() + 2, sizeof(*my_data), 0600| IPC_CREAT); // sizeof(int)*n_atom_init*x ???
	my_data=shmat(m_id, NULL, 0);
    

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

    sem_config_id = semget(getpid(), 1, 0600 | IPC_CREAT);
    sem_set_val(sem_config_id, 0, (SO_NAVI + SO_PORTI + 1));//////////////?? nella lib

    //sem_offerte_richieste_id = semget(getpid() + 1, 1, 0600 | IPC_CREAT);
    //sem_set_val(sem_offerte_richieste_id, 0, 1);

   
    while(wait(NULL) != - 1);////


   // inizio a salvare in sm i dati del master	
	my_data->pid[0] = getpid();  // parent process - master
	my_data->id[0] = 0; 
    my_data->num_atomico[0]=150;
	
	my_data->j=0; // inizializzo il puntatore della sh my_data

 //Creazione dell'atomo circa

    for(i = 0; i < N_ATOMI_INIT; i++){
        switch (pidAtomi[i] = fork()){
            case -1:
                printf("C'è stato un errore nella scissione (fork) di un atomo: %s", strerror(errno));
                kill -KILL pidAtomi[i]; // vedi se funz
               // exit(-1);
            case 0:
                my_data->pid[my_data->j+=1] = getpid();
                my_data->id[my_data->j] = my_data->j;
			    my_data->num_atomico[my_data->j]=rand()%118+1;

                sprintf(argv_buffer, "%d", (i+1));
                args_atomo[1] = argv_buffer;
                execve("../bin/atomo", args_atomo, NULL); //???????? path args = 'atomo.c'
                exit(0); 
                break;
            default:
            break;
        }
    }   //  args_atomo[1] = " ";  ///??
    printf("[SISTEMA]\t -> \t TUTTI GLI ATOMI SONO PRONTI\n");
    
    
//Generazionie PROCESSO ATTIVATORE
    
   switch (pidPAttivatore = fork()){
        case -1:
            printf("C'è stato un errore nel fork per il processo attivatore: %s", strerror(errno));
             kill -KILL pidAtomi[i]; // vedi se funz
           // exit(-1);
            break;
         case 0:      
            execve("../bin/attivatore", args_attivatore , NULL); //?????????? path args = 'attivatore.c'
            exit(- 1);
            break;
            default:
            break;
    }
    printf("[SISTEMA]\t -> \t IL PROCESSO ATTIVATORE E' PRONTO\n");

    
//Generazione processo ALIMENTAZIONE
    
    switch(pid_pAliment = fork()){
        case -1:
            printf("C'è stato un errore nel fork per il processo alimentazione: %s", strerror(errno));
             kill -KILL pidAtomi[i]; // vedi se funz
            //exit(-1);
            break;
        case 0:
            execve("../bin/alimentazione", args_alimentazione , NULL); // args = 'alimentazione.c'
            exit(0);
        default:
        break;
    }
    printf("[SISTEMA]\t -> \t IL PROCESSO ALIMENTAZIONE E' PRONTO\n");



    int i = 0; //indice dei secondi che passano
    int k = 0;
    printf("[SISTEMA]\t -> \t INIZIO SIMULAZIONE\n");

    while(semctl(sem_config_id, 0, GETVAL) != 0);
    * porti_selezionati = SO_PORTI;
    for(i = 0; i < SO_PORTI; i++){
        kill(pidPorti[i], SIGUSR1);
    }
    i = 0;
    while(i != SIM_DURATION /*&& flagEndMaterials && !flagEndMaelstrom*/){ //cambio so_days in sim_duration
       // checkEndOffers = 0;
       // checkEndRequests = 0;
       // * porti_selezionati = (rand() % (SO_PORTI - 3)) + 4;
        //porto_scelto = rand() % SO_PORTI;
        if(i == 0){
            allAtomsIndex = malloc(sizeof(int) * N_ATOM_INIT);
            /*
            allHarborIndex = malloc(sizeof(int) * SO_PORTI);
            harborIndexNoRepeat = malloc(sizeof(int) * (* porti_selezionati));
            if(SO_PORTI > 4 &&  * porti_selezionati != SO_PORTI){
                remeaningHarbor = malloc(sizeof(int) * (SO_PORTI - * porti_selezionati));
            }
        }
        else{
            if(SO_PORTI > 4 && * porti_selezionati != SO_PORTI){
                remeaningHarbor = malloc(sizeof(int) * (SO_PORTI - * porti_selezionati));
            }
            harborIndexNoRepeat = malloc(sizeof(int) * (* porti_selezionati)); */
        } 
        j = 0;
        while(j != SO_PORTI){
            allHarborIndex[j] = j;
            if(k < * porti_selezionati){
                harborIndexNoRepeat[k] = -1;
                k++;
            }
            j++;
        }
        j = 0;
        k = 0;
        for(k = 0; k < * porti_selezionati; k++){
            flag = 1;
            while(flag){
                if(allHarborIndex[porto_scelto] != -1 && harborIndexNoRepeat[k] == -1){
                    harborIndexNoRepeat[k] = allHarborIndex[porto_scelto];
                    allHarborIndex[porto_scelto] = -1;
                    flag = 0;
                }
                else{
                    porto_scelto = rand() % SO_PORTI;
                }
            }
        }

        k = 0;
        z = 0;

        /*Comparo k con harborIndexNoRepeat se non trovo congruenza ho trovato l'indice da inserire in remeinign harbor*/
        if(SO_PORTI > 4){
            for(k = 0; k < SO_PORTI; k++){
                flagDecision = 0;
                for(j = 0; j < * porti_selezionati; j++){
                    if(k == harborIndexNoRepeat[j]){
                        flagDecision = 1;
                        break;
                    }
                }
                if(!flagDecision){
                    remeaningHarbor[z] = k;
                    z++;
                }
            }
        }
        
        z = 0;
        j = 0;
        k = 0;

        i++;

        if(SO_PORTI != 4){
            for(j = 0; j < (SO_PORTI - * porti_selezionati); j++){
                kill(pidPorti[remeaningHarbor[j]], SIGTERM);
            }
        }
        j = 0;
        for(j = 0; j < * porti_selezionati; j++){
            kill(pidPorti[harborIndexNoRepeat[j]], SIGUSR1);
        }
        kill(pid_meteo, SIGUSR1);

        j = 0;
        k = 0;

        req.tv_sec = 1;
        req.tv_nsec = 0;
        nanosleep(&req, &rem);

        //secPrint
        dailyPrint(statusNavi, statusMerci, statusPorti, portiSwell, i);
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
    
    /**
     * Fine sumulazione, invio un SIGABRT a tutti i processi indicandogli di terminare, subito dopo dealloco tutte le varibili allocate con MALLOC e SHARED MEMORY
    */
    endSimulation(shmPidPorti, shmPidNavi, pid_meteo);

    while(wait(NULL) != -1);
    
    finalReport(statusNavi, statusMerci, maxOfferte, maxRichieste, merci_consegnate, statusPorti, portiSwell, i);
    
    shmdt(tipi_merci);
    shmdt(arr_richieste);
    shmdt(arr_offerte);
    shmdt(arr_pos_porti);
    shmdt(porti_selezionati);
    shmdt(arr_richieste_global);
    shmdt(arr_offerte_global);
    shmdt(merci_consegnate);
    shmdt(statusNavi);
    shmdt(statusMerci);
    shmdt(maxOfferte);
    shmdt(maxRichieste);
    shmdt(shmPidNavi);
    shmdt(shmPidPorti);
    shmdt(statusPorti);
    shmdt(portiSwell);


    //nostri 2
    shmctl(s_id, IPC_RMID, NULL); 
    shmctl(m_id, IPC_RMID, NULL);

    shmctl(shm_merci_id, IPC_RMID, NULL);
    shmctl(shm_richieste_id, IPC_RMID, NULL);
    shmctl(shm_offerte_id, IPC_RMID, NULL);
    shmctl(shm_pos_porti_id, IPC_RMID, NULL);
    shmctl(shm_porti_selezionati_id, IPC_RMID, NULL);
    shmctl(shm_richieste_global_id, IPC_RMID, NULL);
    shmctl(shm_offerte_global_id, IPC_RMID, NULL);
    shmctl(shm_merci_cosegnate_id, IPC_RMID, NULL);
    shmctl(shm_statusNavi_id, IPC_RMID, NULL);
    shmctl(shm_statusMerci_id, IPC_RMID, NULL);
    shmctl(shm_maxOfferte_id, IPC_RMID, NULL);
    shmctl(shm_maxRichieste_id, IPC_RMID, NULL);
    shmctl(shm_pidNavi_id, IPC_RMID, NULL);
    shmctl(shm_pidPorti_id, IPC_RMID, NULL);
    shmctl(shm_statusPorti_id, IPC_RMID, NULL);
    shmctl(shm_portiSwell_id, IPC_RMID, NULL);

    semop(sem_config_id, NULL, IPC_RMID);
    semop(sem_offerte_richieste_id, NULL, IPC_RMID);
    
    free(pidNavi);
    free(pidPorti);
    free(allHarborIndex);

    printf("[SISTEMA] -> SIMULAZIONE TERMINATA\n");

    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_UNBLOCK, &mask_block, NULL);
}

/**
 * Generazione matrice offerte, tutte le offerte vengono generate in una matrice di 0 e 1 la quale e' la copia della matrice delle richieste ma invertita
 * Infine si effettua un controllo sull'ultimo porto, in modo che tutte le Merce siano offerte almeno una volta
*/

void gen_offerta(int * matr_richieste, int * matr_offerte, int * pidPorti, int num_merci, int print){
    int * arr_control;
    int i, k, j, flag = 0, sum = 0, col_merce_offerte;
    int rand_pid, counter = SO_PORTI;

    arr_control = malloc(sizeof(int) * SO_MERCI);

    for(i = 0; i < SO_PORTI; i++){
        matr_offerte[(i * (SO_MERCI + 1))] = pidPorti[i];
        for(j = 1; j < SO_MERCI + 1; j++){
            matr_offerte[(i * (SO_MERCI + 1)) + j] = !matr_richieste[(i * (SO_MERCI + 1)) + j];
        }
        j = 1;
        counter = 0;
    }

    free(arr_control);
}
    
/**
 * Creazione matrice richieste, una matrice riempita di 0 e 1, la prima colonna viene riempita dai PID dei porti che richiederanno la merce. 
 * 0 e 1 servono per indicare se la corrispettiva merce indicata nella colonna e' richiesta o meno
 * Ogni porto richiede un numero casuale di Merce totali tra 1 e SO_MERCI/2
 * L'ultimo porto richiederà tutte le Merce che non sono state richieste dai restanti SO_PORTI - 1 porti, assicurandoci che tutte le Merce siano richieste
 * da almeno un porto
*/

void gen_richiesta_offerta(int * pidPorti, int * arr_richieste, int * arr_offerte, int print){
    int col_merce_richiesta = 0, counter = 0;
    int * arr_control;
    int * matr_richieste, * matr_offerte;
    int i, j, k, z;
    int sum = 0, flag = 0, num_richieste, num_merci;

    if(SO_MERCI == 1){
        num_merci = 1;
    }
    else{
        num_merci = SO_MERCI/2;
    }
    
    arr_control = malloc(sizeof(int) * SO_MERCI);
    matr_richieste = malloc(sizeof(int) * SO_PORTI * (SO_MERCI + 1));
    matr_offerte = malloc(sizeof(int) * SO_PORTI * (SO_MERCI + 1));
    
     
    /**
     * Generazione delle Merce richeste dai primi SO_PORTI-1 porti, i controlli effettuati sulla generazione sono: 
     * 1) che il numero di richieste totali per singola merce non può eccedere SO_MERCI/2 (num_merci)
     * 2) che la merce non sia già stata richiesta dal porto corrente
     * In questo caso andiamo a gestire anche il caso in cui il numero di Merce totali e' 1, evitanto possibili crush o loop
    */

    if(SO_MERCI == 1){
        for(i = 0; i < SO_PORTI; i++){
            matr_richieste[i * (SO_MERCI + 1)] = pidPorti[i];
            if(i < SO_PORTI/2){
                matr_richieste[(i * (SO_MERCI + 1)) + 1] = 1;
            }
            
        }
    }
    else{        
        for(j = 0; j < (SO_PORTI - 1); j++){
            matr_richieste[j * (SO_MERCI + 1)] = pidPorti[j];
            num_richieste = (rand() % (num_merci)) + 1;
            for(k = 0; k < num_richieste; k++){
                col_merce_richiesta = (rand() % SO_MERCI) + 1;
                for(i = 0; i < SO_PORTI - 1; i++){
                    sum += matr_richieste[(i * (SO_MERCI + 1)) + col_merce_richiesta];
                }
                while(matr_richieste[(j * (SO_MERCI + 1)) + col_merce_richiesta] == 1 || sum >= SO_PORTI/2){
                    sum = 0;
                    col_merce_richiesta = (rand() % SO_MERCI) + 1;
                    for(i = 0; i < SO_PORTI - 1; i++){
                        sum += matr_richieste[(i * (SO_MERCI + 1)) + col_merce_richiesta];
                    }
                }
                
                matr_richieste[(j * (SO_MERCI + 1)) + col_merce_richiesta] = 1;
                
                
            }
        }
          

        
        /**
         * Controllo sull'intera matrice, arr_control[i] = 0 se merce i non è mai stata richiesta dai primi SO_PORTI-1 porti
         * Tramite il contenuto di quest'ultimo array andremo ad assegnare all'ultimo porto tutte le risorse che non sono state richieste dagli altri
        */

        for(j = 0; j < SO_PORTI - 1; j++){
            for(k = 1; k < SO_MERCI + 1; k++){
                arr_control[k - 1] |= matr_richieste[(j * (SO_MERCI + 1)) + k];
            }
        }
        matr_richieste[(SO_PORTI - 1) * (SO_MERCI + 1)] = pidPorti[SO_PORTI - 1];
        
        for(k = 1; k < SO_MERCI + 1; k++){
            if(arr_control[k - 1] == 0){
                matr_richieste[((SO_PORTI - 1) *  (SO_MERCI + 1)) + k] = 1;
                counter += 1;
            }
        }
        
        /**
         * Assegnazione delle rimanenti richieste per l'ultimo porto, escludendo quelle con troppe richieste (> SO_PORTI/2)
         * oppure già richieste dallo stesso porto (matr_richieste[SO_PORTI-1][col_merce_richiesta] == 1)
         * Quindi ogni merce in totale puà esser richiesta al masismo SO_PORTI/2 volte gestito tramite la variabile sum
        */

        flag = counter;
        for(k = 1; k <= SO_MERCI && flag < num_merci; k++){
            sum = 0;
            col_merce_richiesta = k;
            for(i = 0; i < SO_PORTI - 1; i++){
                sum += matr_richieste[(i * (SO_MERCI + 1)) + col_merce_richiesta];
            }
            if(matr_richieste[((SO_PORTI - 1) * (SO_MERCI + 1)) + col_merce_richiesta] == 1 || sum >= SO_PORTI/2){
                continue;
            }
            matr_richieste[((SO_PORTI - 1) * (SO_MERCI + 1)) + col_merce_richiesta] = 1;
            flag++;
        }
    }  

    /**
     * Una volta generata la tabella della richieste vado a generare la tabella delle offerte
    */
    
    gen_offerta(matr_richieste, matr_offerte,pidPorti, num_merci, print);

    /**
     * Generate le due matrici 
    */

    i = 0;
    for(j = 0; j < SO_PORTI; j++){
        for(k = 0; k < SO_MERCI + 1; k++){
            arr_richieste[i] = matr_richieste[(j * (SO_MERCI + 1)) + k];
            arr_offerte[i] = matr_offerte[(j * (SO_MERCI + 1)) + k];
            i++;
        }
    }

    /**
     * Semplice stampa per testing
    */

    if(print){
        printf("\tRICHIESTE\n\n");
        for(j = 0; j < SO_PORTI; j++){
            printf("Pid: %d\n", matr_richieste[j * (SO_MERCI + 1)]);
            for(k = 1; k < SO_MERCI + 1; k++){
                printf("merce: %d presa: %d ", k, matr_richieste[(j * (SO_MERCI + 1)) + k]);
            }
            printf("\n");
        }
        printf("\n");

        printf("\tOFFERTE\n\n");
        for(j = 0; j < SO_PORTI; j++){
            printf("Pid: %d\n", matr_offerte[j * (SO_MERCI + 1)]);
            for(k = 1; k < SO_MERCI + 1; k++){
                printf("merce: %d presa: %d ", k, matr_offerte[(j * (SO_MERCI + 1)) + k]);
            }
            printf("\n");
        }
    }
    printf("[SISTEMA]\t -> \t MODULI DOMANDA/OFFERTA GENERATI CORRETTAMENTE\n");

    free(arr_control);
    free(matr_richieste);
    free(matr_offerte);
}


/**
 * Questa funzione serve per effettuare un semplice controllo iniziale sulla maggior parte dei parametri inseriti all'interno del file env_var.h
 * Se un controllo non passa, il programma non parte
*/

void check_inputs(){
    const char * file_path = "../src/env_var.txt"; ////PATH!
    char buffer[1024];
    char buffer_cpy[1024];
    ssize_t bytes_read;
    char * token;
    FILE * file_descriptor = fopen(file_path, "r");

    if (!file_descriptor) {
        perror("Failed to open the file");
    }

    while (fgets(buffer, 1024, file_descriptor)){ // CAMBIA switch case!
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

//secPrint dailyPrint
void secPrint(int * statusNavi, int * statusMerci, int * statusPorti, int * portiSwell, int secondo){
    /**
     * numero di attivazioni occorse ad opera del processo attivatore (totali e relative all’ultimo secondo),
    • numero di scissioni (totali e relative all’ultimo secondo);
    • quantit`a di energia prodotta (totale e relativa all’ultimo secondo);
    • quantit`a di energia consumata (totale e relativa all’ultimo secondo);
    • quantit`a di scorie prodotte (totale e relativa all’ultimo secondo);
    */


    int carico = 0;
    int noCarico = 0;
    int operandoPorto = 0;
    int naviAbbattute = 0;
    int naviBloccate = 0;
    int i;

    for(i = 0; i < SO_NAVI; i++){
        noCarico += statusNavi[(i * 6) + 1];
        carico += statusNavi[(i * 6) + 2];
        operandoPorto += statusNavi[(i * 6) + 3];
        naviAbbattute += statusNavi[(i * 6) + 4];
        naviBloccate += statusNavi[(i * 6) + 5];
    }
    printf("\033[0;33m");
    printf("----------------------------------------------------------------------------------------------\n");
    printf("\033[0m");
    printf("\033[1;31m");
    printf("SECONDO\t->\t[%d]\n", secondo);
    printf("\033[0m");
    printf("\n");
    printf("NAVI IN VIAGGIO SENZA CARICO\t->\t[\033[1;32m%d\033[0m]\n", noCarico);
    printf("NAVI IN VIAGGIO CON CARICO\t->\t[\033[1;32m%d\033[0m]\n", carico);
    printf("NAVI OPERANDO PRESSO PORTI\t->\t[\033[1;32m%d\033[0m]\n", operandoPorto);
    printf("NAVI CHE SONO STATE AFFONDATE\t->\t[\033[2;31m%d\033[0m]\n", naviAbbattute);
    printf("NAVI BLOCCATE DA STORM\t\t->\t[\033[0;31m%d\033[0m]\n", naviBloccate);
    printf("\n");
    printf("PORTI BLOCCATI DA SWELL\n");
    for(i = 0; i < secondo; i++){
        printf("[PORTO\t\033[0;31m%d\033[0m]\n", portiSwell[i]);
    }
    printf("\n");
    for(i = 0; i < SO_MERCI; i++){
        if(i%2 == 0){
            printf("\033[0;36m");
        }
        else{
            printf("\033[0;34m");
        }
        printf("Merce->%d presente in porto->%d\tpresente in nave->%d\tconsegnata in porto->%d\tscaduta in porto->%d\tscaduta in nave->%d\n", (i + 1), statusMerci[(i*5)], statusMerci[(i*5)+1], statusMerci[(i*5)+2], statusMerci[(i*5)+3], statusMerci[(i*5)+4]);
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
    printf("\033[0;33m");
    printf("----------------------------------------------------------------------------------------------\n");
    printf("\033[0m");
}

void finalReport(int * statusNavi, int * statusMerci, int * maxOfferte, int * maxRichieste, int * merci_consegnate, int * statusPorti, int * portiSwell, int giorni){
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
    printf("[SISTEMA] -> LA SIMULAZIONE E' DURATA %d GIORNI\n\n", giorni);
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

void endSimulation (pid_t * pidPorti, pid_t * pidNavi, pid_t pidMeteo){
    int i;
    if(pidNavi != NULL){
        for (i = 0; i < SO_NAVI; i++){
            if(pidNavi[i] != -1){
                kill(pidNavi[i], SIGABRT);
            }
        }
    }
    for (i = 0; i < SO_PORTI; i++){
        kill(pidPorti[i], SIGABRT);
    }
    kill(pidMeteo, SIGABRT);
