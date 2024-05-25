#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int valuePreSim, stat_id, msqid; //, atom_pid;
int *shm_stat; //, *shm_atom;
int sem_pid_atomi, sem_writeShm, sem_att, sem_saveatom;
//int ind = 0;
// pid_t pidAttivatore, pidAliment; /* process identificator dei process id */
struct msgbuff my_msg; //, my_msg_createAtom;

void check_in();
void secPrint(int *, int);
void endSimulation();

/* Handler */
void handle_signal(int signal) {
    switch (signal) {
    case SIGABRT:
        endSimulation();
        break;
    default:
        break;
    }
}

/* Prendo i parametri inseriti nel file input e ne controllo i parametri */
void check_in() {
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

    /*qui posso usare exit(-1) perchè non ho ancora allocato memorie, sh, semafori etc quando la chiamo */
    if (ENERGY_DEMAND < 0){
        printf("ERRORE: IL NUMERO DI ENERGY_DEMAND DEVE ESSERE MAGGIORE DI 0\n");
        exit(-1);
    }
    else if (N_ATOMI_INIT < 0){
        printf("ERRORE: IL NUMERO DI ATOMI INIZIALE DEVE ESSERE MAGGIORE DI 0\n");
        exit(-1);
    }
    else if (N_ATOM_MAX < 1 || N_ATOM_MAX > 118){
        printf("ERRORE: IL NUMERO ATOMICO MASSIMO DEVE ESSERE 118\n");
        exit(-1);
    }
    else if (MIN_N_ATOMICO < 1 || MIN_N_ATOMICO > 118){
        printf("ERRORE: IL NUMERO ATOMICO DEVE ESSERE COMPRESO TRA 1 E 118\n");
        exit(-1);
    }
    else if (STEP_ATTIVATORE < 0){
        printf("ERRORE: LO STEP ATTIVATORE NON PUO' ASSUMERE QUESTO VALORE\n");
        exit(-1);
    }
    else if (STEP_ALIMENTAZIONE < 0){
        printf("ERRORE: LO STEP ALIMENTAZIONE NON PUO' ASSUMERE QUESTO VALOREn");
        exit(-1);
    }
    else if (N_NUOVI_ATOMI < 0){
        printf("ERRORE: IL NUMERI DI NUOVI ATOMI DA CREARE NON PUO' ESSERE 0\n");
        exit(-1);
    }
    else if (SIM_DURATION < 0){
        printf("ERRORE: LA SIMULAZIONE NON PUO' DURARE 0 SECONDI\n");
        exit(-1);
    }
    else if (ENERGY_EXPLODE_THRESHOLD < 0){
        printf("ERRORE: IL THRESHOLD NON PUO' ESSERE 0\n");
        exit(-1);
    }
}

/* Eseguo la print ogni secondo */
void secPrint(int *shm_stat, int sec) {
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

    printf("L\'ENERGIA CONSUMATA NELL\'ULTIMO SECONDO E\' STATA\t->\t[\033[2;32m%d\033[0m]\n", shm_stat[3]);
    printf("L\'ENERGIA CONSUMATA TOTALE E\' \t->\t[\033[2;32m%d\033[0m]\n", shm_stat[2]);
    printf("\n");
    printf("LE SCORIE PRODOTTE NELL\'ULTIMO SECONDO SONO STATE\t->\t[\033[0;32m%d\033[0m]\n", shm_stat[5]);
    printf("LE SCORIE PRODOTTE TOTALI SONO STATE\t->\t[\033[0;32m%d\033[0m]\n", shm_stat[4]);
    printf("\n");
    printf("\033[0;33m");
    printf("--------------------------------------------------------------------------------\n");
    printf("\033[0m");
    /*Potrei aggiungere la stampa dell'enegia disponibile, al netto dei vari prelievi di energy demand */
    //printf("EXTRA %d \n", shm_stat[10]);
}

/* Invio un SIGABRT a tutti i processi e termino il Master stesso */
void endSimulation() {
    /*
    int i = pos_AtomPid;
    if(pid_atomo[i]!= -1) {
        for (i; i >= 0; i--) {
            if(pid_atomo[i] != -1){
                kill(pid_atomo[i], SIGABRT);
            }
        }
    } */

    if (shm_stat[13] != -1)
        kill(shm_stat[13], SIGABRT);

    if (shm_stat[14] != -1)
        kill(shm_stat[14], SIGABRT);

    /*shared memories*/
    shmdt(shm_stat);

    shmctl(stat_id, IPC_RMID, NULL);

    /*semafori*/
    semctl(sem_pid_atomi, 0, IPC_RMID);
    semctl(sem_writeShm, 0, IPC_RMID);
    semctl(sem_att, 0, IPC_RMID);
    semctl(sem_saveatom, 0, IPC_RMID);

    // free(pid_atomo);
    msgctl(msqid, IPC_RMID, NULL);
   // msgctl(msqid_createAtom, IPC_RMID, NULL);
}

int main(int argc, char *argv[]) {
    int t = 0; /* indice dei secondi che passano */
    int pid_master = getpid(); /*POI TOGLI*/
    printf("PID MASTER: %d \n", pid_master);
    
    srand(time(NULL));
    struct timespec req;
    struct sigaction sa;
    bzero(&sa, sizeof(sa));        /*set all bytes to 0*/
    sa.sa_handler = handle_signal; /*puntatore alla funzione handler dei segnali*/
    sigaction(SIGABRT, &sa, NULL); /*set del signal handler*/
      // sigaction(SIGUSR1, &sa, NULL);

 /*sigset_t mask_block;
    int status, errno;
    int flag = 1;
    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_BLOCK, &mask_block, NULL); */

    check_in(); /* da chiamare prima di creare shm e semafori!! */

    /**
     * SHARED MEMORY
     * Pid master + 1: array dei dati necessari per la stampa shm_stat[i]
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
     * shm_stat[12] = pid del master
     * shm_stat[13] = pid dell'attivatore
     * shm_stat[14] = pid dell'alimentazione
     * 
     */

    stat_id = shmget(getpid() + 1, sizeof(int) * 15, 0600 | IPC_CREAT); /*creazione area di memoria condivisa */
    shm_stat = shmat(stat_id, NULL, 0);                                 /*collega l'area a un puntatore*/
   
   // atom_pid = shmget(getpid() + 2, sizeof(int) * N_ATOMI_INIT, 0600 | IPC_CREAT); /*creazione area di memoria condivisa */
    //shm_atom = shmat(atom_pid, NULL, 0);                                   /*collega l'area a un puntatore*/

    shm_stat[12] = getpid();

    /**
     * SEMAFORI
     * pid master + 1: Gestisce la creazione iniziale di tutti i processi
     * pid master + 2:
     * pid master + 3:
     * pid master + 4:
     */

    sem_pid_atomi = semget(getpid() + 1, 1, 0600 | IPC_CREAT);
    sem_setval(sem_pid_atomi, 0, (N_ATOMI_INIT + 2));
    printf("VALORE SEMAFORO SEM_PID_ATOMI: %i \n", semctl(sem_pid_atomi, 0, GETVAL));
    int valuePreSim = semctl(sem_pid_atomi, 0, GETVAL);
    printf("VALORE valuePreSim: %d \n", valuePreSim);

    sem_writeShm = semget(getpid() + 2, 1, 0600 | IPC_CREAT);
    sem_setval(sem_writeShm, 0, 1);
    
    sem_att = semget(getpid() + 3, 1, 0600 | IPC_CREAT);
    sem_setval(sem_att, 0, 1);
    printf("VALORE SEMAFORO SEM_ATT: %i \n ", semctl(sem_att, 0, GETVAL));

    sem_saveatom = semget(getpid() + 4, 1, 0600 | IPC_CREAT); // mi serve per aspettare che attivatore salvi gli atomi
    sem_setval(sem_saveatom, 0, 0); //metto a 1
    
    /* MESSAGE QUEUE */ //SE NON SERVE IN MASTER TOGLI
	//msqid = msgget(getpid() + 1, IPC_CREAT | 0600); // CODA TRA ATTIVATORE E ATOMO PER SCISSIONE
	// IPC_EXCL |
   // msqid_createAtom = msgget(getpid() + 2, IPC_CREAT | 0600); // CODA TRA ATTIVATORE E ALIMENTAZIONE

   
    /* array di stringhe, terminanti con NULL, da passare in execve() */
    char *args_atomi[] = {"atomo.c", NULL};
    char *args_attivatore[] = {"attivatore.c", NULL};
    char *args_aliment[] = {"alimentazione.c", NULL};
    char argv_buffer[50]; /*!!!!!!!!*/
    char att[] = {"../src/attivatore"};
    char aliment[] = {"../src/alimentazione"};
    char ato[] = {"../src/atomo"};


    printf("\nATTIVATORE \n");
    /* Generazione processo ATTIVATORE */
    switch (fork()) {
    case -1:
        printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEL PROCESSO ATTIVATORE\n");
        kill(getpid(), SIGABRT);
        break;
    case 0:
        shm_stat[13] = getpid();
        execve(att, args_attivatore, NULL);
        exit(0);
        break;
    default:
        break;
    }
    //printf("valuePreSim = %d \n", valuePreSim);
    while (semctl(sem_pid_atomi, 0, GETVAL) != (valuePreSim - 1));
    valuePreSim -= 1;
    printf("MASTER valuePreSim DOPO ATTIVATORE 2 = %d \n", valuePreSim);
    printf("SEMAFORO SEM_PID_ATOMI 2 = %i \n", semctl(sem_pid_atomi, 0, GETVAL));

    printf("\nIL PROCESSO ATTIVATORE E' PRONTO\n");


    printf("\nATOMI: \n");
    /* Generazione degli ATOMI INIZIALI */
    for (int i = 0; i < N_ATOMI_INIT; i++) {
        printf("prima m316 sem pid atomi %i \n", semctl(sem_pid_atomi, 0, GETVAL));

        switch (fork()) {
        case -1:
            printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
            kill(getpid(), SIGABRT);
            break;
            /* printf("MELTDOWN: %s", strerror(errno));
             exit(-1);*/
        case 0: 
            // shm_atom[i] = getpid();
            sprintf(argv_buffer, "%d", (getppid()));
            args_atomi[1] = argv_buffer;
            // printf("VEDIAMO IL BUFFER: %s \n", argv_buffer);
            // sprintf(argv_buffer + j + 1, "%d", rand()%(N_ATOM_MAX+1)+1);
		    //args_atomi[2] = argv_buffer;
            printf("VEDIAMO ARGS_ATOMI 1: %d \n", atoi(args_atomi[1]));
            // printf("VEDIAMO ARGS_ATOMI 2: %d \n", atoi(args_atomi[2]));

            execve(ato, args_atomi, NULL);
            exit(0);
            break;
        default:
            break;
        }

        while (semctl(sem_pid_atomi, 0, GETVAL) != (valuePreSim - 1));
        valuePreSim -= 1;
        printf("dopo atomo SEMAFOTO PID ATOMI: %i \n", semctl(sem_pid_atomi, 0, GETVAL));
        printf("dopo atomo valuePreSim %i \n", valuePreSim);
    }
    printf("TUTTI GLI ATOMI SONO PRONTI\n");

    printf("\n");

    

    //printf("ALIMENTAZIONE \n");
    /* Generazione processo ALIMENTAZIONE */
    /*
    switch (fork()) {
    case -1:
        printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEL PROCESSO ALIMENTAZIONE\n");
        kill(getpid(), SIGABRT);
        break;
    case 0:
        shm_stat[14] = getpid();
        execve(aliment, args_aliment, NULL);
        exit(0);
        break;
    default:
        break;
    }
    printf("IL PROCESSO ALIMENTAZIONE E' PRONTO\n");
    */
    while (semctl(sem_pid_atomi, 0, GETVAL) != (valuePreSim - 1));
    valuePreSim -= 1;
   
    printf("SEMAFORO sem pid in master 364 0 = %i \n", semctl(sem_pid_atomi, 0, GETVAL));
    printf("\n");

    /* Aspetto fin quando il semaforo non è a 0, ovvero non sono stati creati tutti gli atomi */
    while (semctl(sem_pid_atomi, 0, GETVAL) != 0);

    printf("\n");

    printf("INIZIO DELLA SIMULAZIONE\n");

   // shm_stat[10] = 20;
    while (t < SIM_DURATION){ /*SE SERVE metto delle flags qui per interrompere la sim*/
      
      

        printf("sem att inizio ciclo master: %i \n ", semctl(sem_att, 0, GETVAL));
    printf("\n");

        /* Attendo che gli altri processi facciano le loro cose */
        printf("sem att Mast242: %i\n ", semctl(sem_att, 0, GETVAL));
        while (semctl(sem_att, 0, GETVAL) == 1);
            printf("\n");
        printf("sem att Mast242: %i\n ", semctl(sem_att, 0, GETVAL)); //// PROBLEMA QUIIIII XK RIMANE 000000
  
   req.tv_sec = 1;
        req.tv_nsec = 0;
        nanosleep(&req, NULL);

        printf("sem write shm Mast254: %i\n ", semctl(sem_writeShm, 0, GETVAL));
        
        while(semctl(sem_writeShm, 0, GETVAL) == 0);
        sem_reserve(sem_writeShm, 0);

        printf("sem write shm  dopo reserve Mast258: %i\n ", semctl(sem_writeShm, 0, GETVAL));

        /* PRELIEVO DELL'ENERGY DEMAND */
        shm_stat[10] = shm_stat[10] - ENERGY_DEMAND;
        shm_stat[2] = shm_stat[2] + ENERGY_DEMAND; /*Aumento in shm il contatore di energia consumata*/
        shm_stat[3] = shm_stat[3] + ENERGY_DEMAND;
        printf("shm stat 10 %d \n", shm_stat[10]);
        printf("threshold %d \n", ENERGY_EXPLODE_THRESHOLD);

        if (shm_stat[10] > ENERGY_EXPLODE_THRESHOLD)
        {
            printf("EXPLODE: L\'ENERGIA LIBERATA E\' MAGGIORE DEL THRESHOLD\n");
            kill(getpid(), SIGTERM);
        }
        else if (shm_stat[10] < 0)
        {
            printf("BLACKOUT: L\'ENERGIA CHE SI VUOLE PRELEVARE NON E\' DISPONIBILE\n");
            kill(getpid(), SIGTERM);
        }

        t++;
        secPrint(shm_stat, t);
        /* Poi azzero le statistiche relative all'ultimo secondo _sec */
        shm_stat[1] = 0;
        shm_stat[3] = 0;
        shm_stat[5] = 0;
        shm_stat[7] = 0;
        shm_stat[9] = 0;
        
        sem_release(sem_writeShm, 0);

        sem_release(sem_att, 0); /*attivatore può ricominciare a girare*/
    }

    printf("IL TEMPO DI SIMULATIONE IMPOSTATO E' TERMINATO\n");

    /* Fine sumulazione, invio un SIGABRT a tutti i processi indicandogli di terminare*/
    endSimulation();

    while (wait(NULL) != -1); /*???????????*/

    printf("SIMULAZIONE TERMINATA CON SUCCESSO\n");

    /*???????????
    sigemptyset(&mask_block);
    sigaddset(&mask_block, SIGINT);
    sigprocmask(SIG_UNBLOCK, &mask_block, NULL);*/
}