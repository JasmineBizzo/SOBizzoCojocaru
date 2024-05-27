#include "utils.h"
#include "../lib/ipc.c"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int stat_id, sem_init, sem_writeShm;
int *shm_stat;

void check_in();
void secPrint(int); //(int *, int);
void endSimulation();

    /* Handler */
void handle_signal(int signal) {
    switch (signal) {
    case SIGABRT:
        endSimulation();
        break;
    case SIGUSR1: /*DA TOGLIERE!!!!*/
        printf("RICEVUTO SIGUSR1\n");
        break;
    default:
        break;
    }
}

    /* Prende i parametri inseriti nel file input e ne fa il controllo */
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
        printf("ERRORE: LO STEP ALIMENTAZIONE NON PUO' ASSUMERE QUESTO VALORE\n");
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

    /* Esegue la print ogni secondo */
void secPrint(int sec) { ///(int *shm_stat, int sec) {
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
    printf("L\'ENERGIA DISPONIBILE PER IL PROSSIMO PRELIEVO E\' \t->\t[\033[1;32m%d\033[0m]\n", shm_stat[10]);
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
}

/* Invio un SIGABRT a tutti i processi indicandogli di terminare */
void endSimulation(){
    if (shm_stat[12] != -1) //prima invio segnale ad alimentazione
        kill(shm_stat[12], SIGABRT); 

    if (shm_stat[11] != -1)
        kill(shm_stat[11], SIGABRT);
        
    

    /* Memoria condivisa */
    shmdt(shm_stat);
    shmctl(stat_id, IPC_RMID, NULL);

    /* Semafori */
    semctl(sem_init, 0, IPC_RMID);
    semctl(sem_writeShm, 0, IPC_RMID);
   // semctl(sem_att, 0, IPC_RMID);

    // exit(0);
}

int main(int argc, char *argv[]) {
    int t = 0; /* indice dei secondi che passano */

    struct timespec req;
    srand(time(NULL));

    struct sigaction sa;
    bzero(&sa, sizeof(sa));        /*set all bytes to 0*/
    sa.sa_handler = handle_signal; /*puntatore alla funzione handler dei segnali*/
    sigaction(SIGABRT, &sa, NULL); /*set del signal handler*/
    sigaction(SIGUSR1, &sa, NULL); /******dA TOGLIERE!!!!*/
    
    check_in(); /* da chiamare prima di creare shm e semafori!! */

    /**
     * SHARED MEMORY
     * Pid master + 1: array dei dati necessari per la stampa + pid di attivatore e alimentazione
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
     * shm_stat[10] = energia disponibile quando il master preleva l'Energy Demand;
     *
     * shm_stat[11] = pid dell'attivatore
     * shm_stat[12] = pid dell'alimentazione
     */
    stat_id = shmget(getpid() + 1, sizeof(int) * 13, 0600 | IPC_CREAT); /*creazione area di memoria condivisa */
    shm_stat = shmat(stat_id, NULL, 0); /*collega l'area a un puntatore*/
   
    /**
     * SEMAFORI
     * pid master + 1: Gestisce la creazione iniziale di tutti i processi
     * pid master + 2: Gestisce la scrittura nella memoria condivisa
     */

    sem_init = semget(getpid() + 1, 1, 0600 | IPC_CREAT);
    sem_setval(sem_init, 0, (N_ATOMI_INIT + 2));
    //int valuePreSim = ; /*Per effettuare controlli su sem_init*/

    sem_writeShm = semget(getpid() + 2, 1, 0600 | IPC_CREAT);
    sem_setval(sem_writeShm, 0, 1);
    /* sem_att = semget(getpid() + 3, 1, 0600 | IPC_CREAT);
    sem_setval(sem_att, 0, 1);
*/
    /* MESSAGE QUEUE */ 
	//msqid = msgget(getpid() + 1, IPC_CREAT | 0600); // CODA TRA ATTIVATORE E ATOMO PER SCISSIONE
   
    /* array di stringhe, terminanti con NULL, da passare in execve() */
    char sprintf_buffer[100];
    char *args_atomi[] = {"atomo.c", NULL};
    char *args_attivatore[] = {"attivatore.c", NULL};
    char *args_aliment[] = {"alimentazione.c", NULL};
    char ato[] = {"../src/atomo"};
    char att[] = {"../src/attivatore"};
    char aliment[] = {"../src/alimentazione"};

    /* Generazione degli ATOMI INIZIALI */
    for (int i = 0; i < N_ATOMI_INIT; i++) {
        int num_atomico = rand()%N_ATOM_MAX+1;
        switch (fork()) {
        case -1:
            printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
            kill(getpid(), SIGABRT);
            break;
        case 0: 
            sprintf(sprintf_buffer, "%d", getppid());
            args_atomi[1] = strdup (sprintf_buffer);
            sprintf(sprintf_buffer, "%d", num_atomico);
		    args_atomi[2] = strdup (sprintf_buffer);

            // printf("VEDIAMO ARGS_ATOMI: %d %d\n", atoi(args_atomi[1]), atoi(args_atomi[2]));
            execve(ato, args_atomi, NULL);
            exit(0);
            break;
        default:
            break;
        }
    }
    while (semctl(sem_init, 0, GETVAL) != 2);
    //printf("%d", semctl(sem_init, 0, GETVAL));
    printf("TUTTI GLI ATOMI INIZIALI SONO PRONTI\n");

 /* Generazione processo ATTIVATORE */
    switch (fork()) {
    case -1:
        printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEL PROCESSO ATTIVATORE\n");
        kill(getpid(), SIGABRT);
        break;
    case 0:
        shm_stat[11] = getpid();
        execve(att, args_attivatore, NULL);
        exit(0);
        break;
    default:
        break;
    }
    while (semctl(sem_init, 0, GETVAL) != 1);
    printf("IL PROCESSO ATTIVATORE E' PRONTO\n");

    
    /* Generazione processo ALIMENTAZIONE */
    switch (fork()) {
    case -1:
        printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEL PROCESSO ALIMENTAZIONE\n");
        kill(getpid(), SIGABRT);
        break;
    case 0:
        shm_stat[12] = getpid();
        execve(aliment, args_aliment, NULL);
        exit(0);
        break;
    default:
        break;
    }
    printf("IL PROCESSO ALIMENTAZIONE E' PRONTO\n");
//printf("%d", semctl(sem_init, 0, GETVAL));

    /* Aspetto fin quando non sono stati creati tutti i processi */
    while (semctl(sem_init, 0, GETVAL) != 0);

    printf("INIZIO DELLA SIMULAZIONE\n");

    while (t < SIM_DURATION){
        req.tv_sec = 1;
        req.tv_nsec = 0;
        nanosleep(&req, NULL);
        //while (semctl(sem_att, 0, GETVAL) == 1);
        //while(semctl(sem_writeShm, 0, GETVAL) == 1);
        
    /* Prelievo dell'ENERGY DEMAND */
        sem_reserve(sem_writeShm, 0);
        shm_stat[10] = shm_stat[10] - ENERGY_DEMAND;
        shm_stat[2] = shm_stat[2] + ENERGY_DEMAND;
        shm_stat[3] = shm_stat[3] + ENERGY_DEMAND;

        if (shm_stat[10] > ENERGY_EXPLODE_THRESHOLD) {
            printf("EXPLODE: L\'ENERGIA LIBERATA E\' MAGGIORE DEL THRESHOLD\n");
            kill(getpid(), SIGABRT);
            
        } else if (shm_stat[10] < 0) {
            printf("BLACKOUT: L\'ENERGIA CHE SI VUOLE PRELEVARE NON E\' DISPONIBILE\n");
            kill(getpid(), SIGABRT);
           
        }

        t++;
        secPrint(t);
        
    /* Poi azzero le statistiche relative all'ultimo secondo _sec */
        shm_stat[1] = 0;
        shm_stat[3] = 0;
        shm_stat[5] = 0;
        shm_stat[7] = 0;
        shm_stat[9] = 0;
        sem_release(sem_writeShm, 0);
       //sem_release(sem_att, 0); 
       
    }
    printf("IL TEMPO DI SIMULATIONE IMPOSTATO E' TERMINATO\n");

    endSimulation();

    while (wait(NULL) != -1); /*SERVE CREDO MA CONTROLLA???????????*/

    printf("SIMULAZIONE TERMINATA CON SUCCESSO\n");
    return 0;
}