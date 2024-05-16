#include "utils.h"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

void check_in();
void secPrint(int *, int);
void endSimulation();

//Mi serve dichiararle globali, perchè verranno usate sia dal main sia dall'handler e da endSimulation
int stat_id, atom_pid_id, atom_num_id; 
int * shm_stat, * shm_atom_pid, * shm_atom_num;
int sem_config_id;

pid_t pidAttivatore, pidAliment; //process identificator dei process id

void handle_signal(int signal){
    switch (signal){
        case SIGABRT:
            endSimulation();
        break;
        default:
        break;
    }
}

int main(int argc, char * argv[]){
    sem_config_id = 0;

    //pidAtomi = malloc(sizeof(pidAtomi) * N_ATOMI_INIT); Non lo uso più, uso shm
   // pidAttivatore = malloc(sizeof(pidAttivatore));
  //  pidAliment = malloc(sizeof(pidAliment) *1);
    
    char * args_atomi[]  = {"atomo.c", NULL}; // array di stringhe terminante con NULL da passare in execve()
    char * args_attivatore[] = {"attivatore.c", NULL};
    char * args_aliment[] = {"alimentazione.c", NULL}; 
    char argv_buffer[50];

    struct timespec req; //struct timespec req, rem;
    struct sigaction sa;
    /**  sigset_t mask_block;
    int i, j, k, status, errno;
    int * harborIndexNoRepeat, * allHarborIndex, * remeaningHarbor;
    int flag = 1;
    int flagDecision = 0;
    char argv_buffer[50];
    char * args_navi[]  = {"navi.c", NULL};
    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_BLOCK, &mask_block, NULL);
    s(getpid())*/

    bzero(&sa, sizeof(sa)); //set all bytes to 0
    sa.sa_handler = handle_signal; //puntatore alla funzione handler dei segnali
    sigaction(SIGABRT, &sa, NULL); //set del signal handler

    check_in(); // da chiamare prima di creare sh e semafori!!

    /**SHARED MEMORY
     * parent pid + 1: array dei dati necessari per la stampa shm_stat[i]
     * parent pid + 2: array dei pid 
     * parent pid + 3: array dei numeri atomici
     * 
     * shm_stat[0] = int energy; totale, non tiene conto dei prelievi del master
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
     * shm_stat[10] = energia generata dagli atomi, disponibile quando il master preleva l'Energy Demand;
     * shm_stat[11] = int num_atom_tot, counter di tutti gli atomi effettivamente generati
     * shm_stat[12] = int num_atom_max, il massimo numero di atomi che si potrebbero generare, per sizeof
    */

    stat_id = shmget(getpid() + 1, sizeof(int) *13, 0600 | IPC_CREAT); //creazione area di memoria condivisa
	shm_stat = shmat(stat_id, NULL, 0); //collegare l'area a un puntatore
    shm_stat[12] = N_ATOMI_INIT + 2 + (N_NUOVI_ATOMI*(SIM_DURATION/STEP_ALIMENTAZIONE));
    //Qui inizialmente shm_stat[11] assume il valore massimo degli atomi che posso avere, 
    //poi negli altri processi lo aggiorno ogni volta con il numero effettivo di atomi presenti

    atom_pid_id = shmget(getpid() + 2, sizeof(int) * shm_stat[12], 0600 | IPC_CREAT); ///size????
	shm_atom_pid = shmat(atom_pid_id, NULL, 0);

    atom_num_id = shmget(getpid() + 3, sizeof(int) * shm_stat[12], 0600 | IPC_CREAT); //size?
	shm_atom_num = shmat(atom_num_id, NULL, 0);

    /**
     * SEMAFORI
     * parent pid + 0: fine configuarazione iniziale, la simulazione parte solo dopo che tutto e' stato inizializzato
     * // CANC parent pid + 1: fine creazione matrice richieste/offerte dei porti
    */

    sem_config_id = semget(getpid(), 1, 0600 | IPC_CREAT);
   // sem_set_val(sem_config_id, 0, (N_ATOMI_INIT + 2));
    //Valore = il num di p atomo che creo inizialmente + 1 per il p attivatore + 1 per il p alimentazione
    //Quando il valore sarà 0, ovvero tutti i p saranno stati creati, inizierà la simulazione

    while(wait(NULL) != - 1);////??

   //Non essendo il master un atomo, questo probably non serve: Inizializzazione shm con pid del master
	shm_atom_pid[0] = getpid(); 
    shm_atom_num[0] = 0;
			

    //Creazione degli ATOMI INIZIALI
    int i;
    for(i = 0; i < N_ATOMI_INIT; i++){
        switch (shm_atom_pid[i] = fork()){
            case -1:
                printf("MELTDOWN: ERRORE NEL FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
                kill(getpid(), SIGABRT);
                // printf("MELTDOWN: %s", strerror(errno));
                // exit(-1);
            case 0:
                shm_atom_pid[i] = getpid(); 
                shm_atom_num[i] = rand()%N_ATOM_MAX+1;
                
                sprintf(argv_buffer, "%d", (i+1)); //prima era i+1 provo a cambiare
                args_atomi[1] = argv_buffer;
                execve("../SO/atomo", args_atomi, NULL); 
                exit(0); 
                break;
            default:
            break;
        }
    }
    shm_stat[11] = i; //aggiorno il num di atomi totali creati  
    //  args_atomo[1] = " ";  ///??
    printf("TUTTI GLI ATOMI SONO PRONTI\n");
    
    //Per ora non credo mi interessi salvare il pid di arrivatore e alimentazione, vediamo poi con i SEGNALI

    //Generazione processo ATTIVATORE
    switch (pidAttivatore = fork()){
        case -1:
            printf("MELTDOWN: ERRORE NEL FORK PER LA CREAZIONE DEL PROCESSO ATTIVATORE\n");
            kill(getpid(), SIGABRT);
            break;
         case 0:      
            execve("../SO/attivatore", args_attivatore, NULL); 
            exit(0);
            break;
            default:
            break;
    }
    printf("IL PROCESSO ATTIVATORE E' PRONTO\n");

    
    //Generazione processo ALIMENTAZIONE 
    switch(pidAliment = fork()){
        case -1:
            printf("MELTDOWN: ERRORE NEL FORK PER LA CREAZIONE DEL PROCESSO ALIMENTAZIONE\n");
            kill(getpid(), SIGABRT);
            break;
        case 0:
            execve("../SO/alimentazione", args_aliment, NULL);
            exit(0);
        default:
        break;
    }
    printf("IL PROCESSO ALIMENTAZIONE E' PRONTO\n");


    printf("INIZIO DELLA SIMULAZIONE\n");

    //Rimango qui fin quando il semaforo non è a 0, ovvero fin quando non sono stati creati tutti i processi
    while(semctl(sem_config_id, 0, GETVAL) != 0); 
                                                
    int t = 0; //indice dei secondi che passano
    while(t != SIM_DURATION){ //SE SERVE metto delle flags qui per interrompere la sim

       //prima attendo con un semaforo che tutti gli atomi si siano scissi FAI!
       
    //PRELIEVO DELL'ENERGY DEMAND
        shm_stat[10] = shm_stat[10] - ENERGY_DEMAND;
        if(shm_stat[10] > ENERGY_EXPLODE_THRESHOLD){
            printf("EXPLODE: L \'ENERGIA LIBERATA E\' MAGGIORE DEL THRESHOLD\n");
            kill(getpid(), SIGTERM);
        }
       else if(shm_stat[10] < 0){//è corretto leggendo la consegna???
            printf("BLACKOUT: L \'ENERGIA CHE SI VUOLE PRELEVARE NON E\' DISPONIBILE\n");
            kill(getpid(), SIGTERM);
        }
            
        req.tv_sec = 1;
        req.tv_nsec = 0;
        nanosleep(&req, NULL); //nanosleep(&req, &rem);

        secPrint(shm_stat, t);
        t ++;
    }
    printf("IL TEMPO DI SIMULATIONE IMPOSTATO E' TERMINATO\n");

    //Fine sumulazione, invio un SIGABRT a tutti i processi indicandogli di terminare
    endSimulation();

    while(wait(NULL) != -1); //???????????
    
    printf("SIMULAZIONE TERMINATA CON SUCCESSO\n");

    /*???????????
    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_UNBLOCK, &mask_block, NULL);*/
}

//Controllo iniziale sui parametri inseriti nel file input
void check_in(){
    const char * file_path = "../SO/input.txt";
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
        }else if(strcmp(token, "N_ATOM_MAX") == 0){
            token = strtok(NULL, " ");
            N_ATOM_MAX = atoi(token);
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

    //qui posso usare exit(-1) perchè non ho ancora allocato memorie, sh, semafori etc quando la chiamo
    if(ENERGY_DEMAND < 0){
        printf("ERRORE: IL NUMERO DI ENERGY_DEMAND DEVE ESSERE MAGGIORE DI 0\n");
        exit(-1);
    } else if(N_ATOMI_INIT < 0){
        printf("ERRORE: IL NUMERO DI ATOMI INIZIALE DEVE ESSERE MAGGIORE DI 0\n");
        exit(-1);
    } else if(N_ATOM_MAX <1 || N_ATOM_MAX > 118){
        printf("ERRORE: IL NUMERO ATOMICO MASSIMO DEVE ESSERE 118\n");
        exit(-1);
    } else if(MIN_N_ATOMICO < 1 || MIN_N_ATOMICO > 118){
        printf("ERRORE: IL NUMERO ATOMICO DEVE ESSERE COMPRESO TRA 1 E 118\n");
        exit(-1);
    } else if(STEP_ATTIVATORE < 0){
        printf("ERRORE: LO STEP ATTIVATORE NON PUO' ASSUMERE QUESTO VALORE\n");
        exit(-1);
    } else if(STEP_ALIMENTAZIONE < 0){
        printf("ERRORE: LO STEP ALIMENTAZIONE NON PUO' ASSUMERE QUESTO VALOREn");
        exit(-1);
    } else if(N_NUOVI_ATOMI < 0){
        printf("ERRORE: IL NUMERI DI NUOVI ATOMI DA CREARE NON PUO' ESSERE 0\n");
        exit(-1);
    } else if(SIM_DURATION < 0){
        printf("ERRORE: LA SIMULAZIONE NON PUO' DURARE 0 SECONDI\n");
        exit(-1);
    } else if(ENERGY_EXPLODE_THRESHOLD < 0){
        printf("ERRORE: IL THRESHOLD NON PUO' ESSERE 0\n");
        exit(-1);
    }
}

//Eseguo la print ogni secondo
void secPrint(int * shm_stat, int sec){
    printf("\033[0;33m");
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
    //Potrei aggiungere la stampa dell'enegia disponibile, al netto dei vari prelievi di energy demand
    printf("L\'ENERGIA CONSUMATA NELL\'ULTIMO SECONDO E\' STATA\t->\t[\033[2;32m%d\033[0m]\n", shm_stat[3]);
    printf("L\'ENERGIA CONSUMATA TOTALE E\' \t->\t[\033[2;32m%d\033[0m]\n", shm_stat[2]);
    printf("\n");
    printf("LE SCORIE PRODOTTE NELL\'ULTIMO SECONDO SONO STATE\t->\t[\033[0;32m%d\033[0m]\n", shm_stat[5]);
    printf("LE SCORIE PRODOTTE TOTALI SONO STATE\t->\t[\033[0;32m%d\033[0m]\n", shm_stat[4]);
    printf("\n");
    printf("\033[0;33m");
    printf("--------------------------------------------------------------------------------\n");
    printf("\033[0m");

    //Poi azzero le statistiche relative all'ultimo secondo _sec
    shm_stat[1]= 0; 
	shm_stat[3]= 0;
	shm_stat[5]= 0; 
	shm_stat[7] = 0; 
	shm_stat[9] = 0;
}

//Invio un SIGABRT a tutti i processi indicandogli di terminare
void endSimulation (){ //devo killare anche alimentazione e attivatore giusto?
    int i = shm_stat[11];
    while(shm_atom_pid[i] != -1){
        kill(shm_atom_pid[i], SIGTERM); //da un'eccezione a run time, RISOLVI!!!
        i--;
    }   
    /*int i;
    if(shm_atom_pid[i]!= NULL){ 
        for (i = 0; i < shm_stat[11]; i++){  
            if(shm_atom_pid[i] != -1){
                kill(shm_atom_pid[i], SIGABRT);
            }
        }
    }*/

    kill(pidAttivatore, SIGTERM);//sigterm? sigabort? boooo
    kill(pidAliment, SIGTERM);

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
    /*
    if(pidAttivatore != -1){ //metto l'i perchè potrebbe aver fallito la fork e quindi 
                             //potrebbero non essere ancora stati creati
        free(pidAttivatore);
    }
    if(pidAliment != -1){
        free(pidAliment);
    }*/
}
