#include "utils.h"
#include "../lib/ipc.h"

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO;
int STEP_ATTIVATORE, STEP_ALIMENTAZIONE, N_NUOVI_ATOMI;
int SIM_DURATION, ENERGY_DEMAND, ENERGY_EXPLODE_THRESHOLD;

int stat_id, sem_init, sem_writeShm, pid_attivatore, pid_alimentazione, sem_atoms, atoms_id;
int generata_sec, usata_sec, scorie_sec, scissioni_sec, attivazioni_sec, energy_avaliable;
int generata_tot = 0, usata_tot = 0, scorie_tot = 0, scissioni_tot = 0, attivazioni_tot = 0;
int *shm_stat, * shm_atoms;

void check_in();
void secPrint(int);
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

    /* Esegue la print ogni secondo */
void secPrint(int sec) {
  printf("\033[0;33m");
    printf("\nDURANTE IL SECONDO -> [%d]\n", sec);
    printf("\033[0m");
    printf("\n");
    printf("LE ATTIVAZIONI NELL\'ULTIMO SECONDO SONO STATE -> [\033[1;32m%d\033[0m]\n", attivazioni_sec);
    printf("LE ATTIVAZIONI TOTALI SONO STATE -> [\033[1;32m%d\033[0m]\n", attivazioni_tot);
    printf("\n");
    printf("LE SCISSIONI NELL\'ULTIMO SECONDO SONO STATE -> [\033[1;32m%d\033[0m]\n", scissioni_sec);
    printf("LE SCISSIONI TOTALI SONO STATE -> [\033[1;32m%d\033[0m]\n", scissioni_tot);
    printf("\n");
    printf("L\'ENERGIA PRODOTTA NELL\'ULTIMO SECONDO E\' STATA -> [\033[1;32m%d\033[0m]\n", generata_sec);
    printf("L\'ENERGIA PRODOTTA TOTALE E\' -> [\033[1;32m%d\033[0m]\n", generata_tot);
    printf("\n");
    printf("L\'ENERGIA CONSUMATA NELL\'ULTIMO SECONDO E\' STATA -> [\033[1;32m%d\033[0m]\n", usata_sec);
    printf("L\'ENERGIA CONSUMATA TOTALE E\' t-> [\033[1;32m%d\033[0m]\n", usata_tot);
    printf("\n");
    printf("LE SCORIE PRODOTTE NELL\'ULTIMO SECONDO SONO STATE -> [\033[1;32m%d\033[0m]\n", scorie_sec);
    printf("LE SCORIE PRODOTTE TOTALI SONO STATE -> [\033[1;32m%d\033[0m]\n", scorie_tot);
    printf("\033[0;33m");
    printf("-------------------------------------------------------------------------------\n");
    printf("\033[0m");
}

    /* Invio un SIGABRT a tutti i processi indicandogli di terminare */
void endSimulation() {
    if (pid_alimentazione != -1){
        kill(pid_alimentazione, SIGABRT);
        printf("\nABORT ALIMENTAZIONE %d\n", pid_alimentazione);
    }

    if (pid_attivatore != -1){
        kill(pid_attivatore, SIGABRT);
       // sem_release(sem_writeShm, 0); /***PROVA*/
        printf("\nABORT ATTIVATORE %d\n", pid_attivatore);
    }

   //while(wait(NULL) != -1);
    while (*shm_atoms > 0)
    {
        printf("Atomi Rimanenti %d\n", *shm_atoms);
    }

    /* Memoria condivisa */
    shmdt(shm_stat);
    shmctl(stat_id, IPC_RMID, NULL);
    //shmdt(shm_atoms);
    //shmctl(atoms_id, IPC_RMID, NULL);

    /* Semafori */
    semctl(sem_init, 0, IPC_RMID);
    semctl(sem_writeShm, 0, IPC_RMID);
    semctl(sem_atoms, 0, IPC_RMID);

    exit(0);
}

int main(int argc, char *argv[]) {
    struct timespec req;
    srand(time(NULL));

    struct sigaction sa;
    bzero(&sa, sizeof(sa));        /*set all bytes to 0*/
    sa.sa_handler = handle_signal; /*puntatore alla funzione handler dei segnali*/
    sigaction(SIGABRT, &sa, NULL); /*set del signal handler*/
    
    check_in();

    /**
     * SHARED MEMORY
     * Pid master + 1: array dei dati necessari per la stampa + pid di attivatore
    */
    stat_id = shmget(getpid() + 1, sizeof(int) * 6, 0600 | IPC_CREAT); /*creazione area di memoria condivisa */
    shm_stat = shmat(stat_id, NULL, 0); /*collega l'area a un puntatore*/

   atoms_id = shmget(getpid() + 2, sizeof(int), 0600 | IPC_CREAT); 
    shm_atoms = shmat(atoms_id, NULL, 0);
    *shm_atoms = 0;
   
    /**
     * SEMAFORI
     * pid master + 1: Gestisce la creazione iniziale di tutti i processi
     * pid master + 2: Gestisce la scrittura nella memoria condivisa
     */

    sem_init = semget(getpid() + 1, 1, 0600 | IPC_CREAT);
    sem_setval(sem_init, 0, (N_ATOMI_INIT + 2));

    sem_writeShm = semget(getpid() + 2, 1, 0600 | IPC_CREAT);
    sem_setval(sem_writeShm, 0, 1);

sem_atoms = semget(getpid() + 3, 1, 0600 | IPC_CREAT);
sem_setval(sem_atoms, 0, 1);

    
    /* array di stringhe, terminanti con NULL, da passare in execve() */
    char sprintf_buffer[100];
    char *args_atomi[] = {"atomo.c", NULL};
    char *args_attivatore[] = {"attivatore.c", NULL};
    char *args_aliment[] = {"alimentazione.c", NULL};
    char ato[] = {"../bin/atomo"};
    char att[] = {"../bin/attivatore"};
    char aliment[] = {"../bin/alimentazione"};

    /* Generazione degli ATOMI INIZIALI */
    for (int i = 0; i < N_ATOMI_INIT; i++) {
        int num_atomico = rand()%N_ATOM_MAX+1;
        switch (fork()) {
        case -1:
            printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEGLI ATOMI INIZIALI\n");
            endSimulation();
            break;
        case 0: 
            sprintf(sprintf_buffer, "%d", getppid());
            args_atomi[1] = strdup (sprintf_buffer);
            sprintf(sprintf_buffer, "%d", num_atomico);
            args_atomi[2] = strdup (sprintf_buffer);

            execve(ato, args_atomi, NULL);
            exit(0);
            break;
        default:
            break;
        }
    }
    while (semctl(sem_init, 0, GETVAL) > 2);
    printf("TUTTI GLI ATOMI INIZIALI SONO PRONTI\n");

 /* Generazione processo ATTIVATORE */
    switch (pid_attivatore = fork()) {
    case -1:
        printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEL PROCESSO ATTIVATORE\n");
        endSimulation();
        break;
    case 0:
        execve(att, args_attivatore, NULL);
        exit(0);
        break;
    default:
        break;
    }
    shm_stat[0] = pid_attivatore;
    printf("IL PROCESSO ATTIVATORE E' PRONTO\n");

    /* Generazione processo ALIMENTAZIONE */
    switch (pid_alimentazione = fork()) {
    case -1:
        printf("MELTDOWN: ERRORE NELLA FORK PER LA CREAZIONE DEL PROCESSO ALIMENTAZIONE\n");
        endSimulation();
        break;
    case 0:
        execve(aliment, args_aliment, NULL);
        exit(0);
        break;
    default:
        break;
    }
    printf("IL PROCESSO ALIMENTAZIONE E' PRONTO\n");

    int t = 0; /* indice dei secondi che passano */

    /* Aspetto fin quando non sono stati creati tutti i processi */
    while (semctl(sem_init, 0, GETVAL) != 0);

    printf("\nINIZIO DELLA SIMULAZIONE\n");

    while (t < SIM_DURATION){
        req.tv_sec = 1;
        req.tv_nsec = 0;
        nanosleep(&req, NULL);

        sem_reserve(sem_writeShm, 0);
        attivazioni_sec = shm_stat[1] - attivazioni_tot;
        scissioni_sec = shm_stat[2] - scissioni_tot;
        generata_sec = shm_stat[3] - generata_tot;
        usata_sec = shm_stat[4] - usata_tot;
        scorie_sec = shm_stat[5] - scorie_tot;

        attivazioni_tot = shm_stat[1];  
        scissioni_tot = shm_stat[2];
        generata_tot = shm_stat[3];
        usata_tot = shm_stat[4];
        scorie_tot = shm_stat[5];
        sem_release(sem_writeShm, 0);

        t++;
        secPrint(t);
        
        sem_reserve(sem_writeShm, 0);
        shm_stat[4] = shm_stat[4] + ENERGY_DEMAND; /* aumento l'energia consumata */
        energy_avaliable = shm_stat[3] - shm_stat[4];
        sem_release(sem_writeShm, 0);

        if (energy_avaliable > ENERGY_EXPLODE_THRESHOLD) {
            printf("EXPLODE: L\'ENERGIA LIBERATA E\' MAGGIORE DEL THRESHOLD\n");
            endSimulation();
        } else if (energy_avaliable < 0) {
            printf("BLACKOUT: L\'ENERGIA CHE SI VUOLE PRELEVARE NON E\' DISPONIBILE\n");
            endSimulation();
        }
    }
    printf("IL TEMPO DI SIMULATIONE IMPOSTATO E' TERMINATO\n");

    endSimulation();

    //printf("SIMULAZIONE TERMINATA CON SUCCESSO\n");
   //return 0;
}
