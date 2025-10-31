// Assignment completed by Shamanthi Rajagopal
// Assignment 3 - NQueens

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Helper function to check if (i,j) is safe given config[0..i-1] 
static int safe(const char *config, int i, int j) {
    for (int r = 0; r < i; r++) {
        int s = config[r];              // column of queen in row r
        if (j == s) return 0;           // same column
        if (i - r == j - s) return 0;   // same major diagonal
        if (i - r == s - j) return 0    ;// same minor diagonal
    }
    return 1;
}

//Recursive function for calculations
static long long solve_count(char *config, int n, int i) {
    
    if (i == n) {
        return 1; // placed all queens
    }

    long long sub_total = 0;
    
    for (int j = 0; j < n; j++) {
        // allocate just enough to extend the prefix by one
        char *new_config = (char *)malloc((i + 1) * sizeof(char));
        
        if (!new_config) {
            // On OOM, be conservative: stop this branch.
            return sub_total;
        }
        
        memcpy(new_config, config, i * sizeof(char));

        if (safe(new_config, i, j)) {
            new_config[i] = (char)j;
            sub_total += solve_count(new_config, n, i + 1);
        }
        
        free(new_config);
    }
    
    return sub_total;
}

// Initialize Thread Struct
typedef struct {
    
    int n;
    int first_col;     // the column for row 0 that this thread will explore
    long long count;   // result written by the thread

} task_t;

static void *worker(void *arg) { // thread work function
    task_t *t = (task_t *)arg; // create thread struct object

    // Build a base config with row 0 fixed to first_col
    char *config = (char *)malloc(t->n * sizeof(char));
     
    if (!config) { // error check
        t->count = 0;
        return NULL;
    }
    
    config[0] = (char)t->first_col;

    // Explore at row 1 and forward
    t->count = solve_count(config, t->n, 1);

    //clean up
    free(config);
    return NULL;
}

int main(int argc, char *argv[]) { // user input
    
    if (argc < 2) { // error check to make sure enough queens
        printf("%s: number of queens required\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]); // string to int

    if (n <= 0) { // print statement for negative number of queens

        printf("running queens %d\n", n);  
        printf("# solutions: %d\n", 0);
        
        return 0;
   
    }

    printf("running queens %d\n", n); // print

    // Row 0 has no conflicts, and N threads created
    pthread_t *threads = (pthread_t *)malloc(n * sizeof(pthread_t));
    task_t    *tasks    = (task_t *)malloc(n * sizeof(task_t));
    
    if (!threads || !tasks) { // error check kinda
        
        // Fallback to single-thread if allocation fails 
        long long total = 0;
        char *config_queen = (char *)malloc(n * sizeof(char));
        
        if (!config_queen) {
            printf("# solutions: %d\n", 0);
            free(threads); free(tasks);
            return 0;
        }
        
        total = solve_count(config_queen, n, 0);

        printf("# solutions: %lld\n", total);
        free(config_queen);
        free(threads); free(tasks);
        
        return 0;
    }

    // Launch N threads, each with a different first column in row 0
    for (int j = 0; j < n; j++) {
        
        tasks[j].n = n;
        tasks[j].first_col = j;
        tasks[j].count = 0;
        
        int receive = pthread_create(&threads[j], NULL, worker, &tasks[j]); // create thread
        
        if (receive != 0) { // check condition

            // If a thread fails to start, this part runs
            char *config_queen = (char *)malloc(n * sizeof(char));
            
            if (config_queen) {
                config_queen[0] = (char)j;
                tasks[j].count = solve_count(config_queen, n, 1);
                free(config_queen);
            }
            
            threads[j] = 0; // mark as not created
        }
    }

    // Join and add
    long long total = 0;
    
    for (int j = 0; j < n; j++) { // iterate through threads and join
        
        if (threads[j] != 0) {
            (void)pthread_join(threads[j], NULL);  // join thread
        }
        
        total += tasks[j].count;
    }

    printf("# solutions: %lld\n", total); // print sol

    //clean up
    free(threads);
    free(tasks);
    return 0;
}
