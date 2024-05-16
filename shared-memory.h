#define NUM_PROC  4
#define NUM_ITER  10000

/*
 * Declaration of the type of the shared data structure
 */

struct stat_data {
	int energy; // energia creata
	int energy_used; // energia consumata
	int scorie; 
	int num_scissioni; 
	int num_attivazioni;
	
	
	int energy_sec; // energia creata nell'ultimo secondo
	int energy_used_sec; // energia consumata nell'ultimo sec
	int scorie_sec; 
	int num_scissioni_sec; 
	int num_attivazioni_sec;
};

struct shared_data { //array probabilmente
	int j;
    int id[100];  //DA RIVEDERE //posizione in cui si trova il pid
    pid_t pid[100]; // process id 
	int num_atomico[100]; 
};


