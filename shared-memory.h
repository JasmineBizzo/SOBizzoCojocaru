#define NUM_PROC  4
#define NUM_ITER  10000

/*
 * Declaration of the type of the shared data structure
 */
struct shared_data {
	int j;
        int id[100];
        pid_t pid[100];
	int num_atomico[100]; 
};

struct stat_data {
	
	int energy;
	int scorie;
	
};


