// Assignment completed by Shamanthi Rajagopal

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <sys/types.h>

// Defines Constants

#define CITIES_LENGTH 7
#define NUM_CITIES (CITIES_LENGTH - 1)

// Defines variables

static const char* cities[] = { "Central City", "Starling City", "Gotham City", "Metropolis", "Coast City", "National City" };

const int distances[CITIES_LENGTH - 1][ CITIES_LENGTH - 1] = {
    {0, 793, 802, 254, 616, 918},
    {793, 0, 197, 313, 802, 500},
    {802, 197, 0, 496, 227, 198},
    {254, 313, 496, 0, 121, 110},
    {616, 802, 227, 121, 0, 127},
    {918, 500, 198, 110, 127, 0}
};

int initial_vector[CITIES_LENGTH] = { 0, 1, 2, 3, 4, 5, 0 };

// Data Structure for a Route

typedef struct {
    int cities[CITIES_LENGTH];
    int total_dist; 
} route;

// Output function (must remain the same)

void print_route ( route* r ) {
    printf ("Route: ");
    for ( int i = 0; i < CITIES_LENGTH; i++ ) {
        if ( i == CITIES_LENGTH - 1 ) {
            printf( "%s\n", cities[r->cities[i]] );
        } else {
            printf( "%s - ", cities[r->cities[i]] );
        }
    }
}

// Calculates the distance (must be called in child)

void calculate_distance( route* r ) {
    if ( r->cities[0] != 0 ) { // error checks
        printf( "Route must start with %s (but was %s)!\n", cities[0], cities[r->cities[0]]);
        exit( -1 );
    } 
    if ( r->cities[6] != 0 ) { // error checks
        printf( "Route must end with %s (but was %s)!\n", cities[0], cities[r->cities[6]]);
        exit ( -2 );
    }
    int distance = 0;
    for ( int i = 1; i < CITIES_LENGTH; i++ ) {
        int to_add = distances[r->cities[i-1]][r->cities[i]];
        if ( to_add == 0 ) {
            printf( "Route cannot have a zero distance segment.\n");
            exit ( -3 );
        }
        distance += to_add;
    }
    r->total_dist = distance;
}

void swap(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// Helper function so the child will compute the distance and not the parent in permute function
// This function follows the logic that the child will store the calculated distance in a temporary file (easy IPC way) and the parent will read it
static int compute_distance_child(route* r){

    char temp_file[] = "/tmp/flashXXXXXX";
    int file_des = mkstemp(temp_file);
    
    if (file_des == -1) { //invalid file directory error check
        return -1;
    }

    close(file_des);

    pid_t pid = fork(); // fork needs to be used

    if (pid < 0) { // error check
        unlink(temp_file);
        return -1;
    }

    if (pid == 0) {

        //printf("child compute pid=%d\n", getpid()); // test print to debug

        route test = *r;
        calculate_distance(&test); // child calculates the distance
        FILE* file = fopen(temp_file, "w");
        
        if (!file) { // error check
            _exit(1);
        }

        fprintf(file, "%d\n", test.total_dist); // add child's distance to file
        fclose(file);

        _exit(0);
    
    } else {
        int check = 0;
        
        if (waitpid(pid, &check, 0) < 0) {
            unlink(temp_file);
            return -1;
        }

        FILE* file = fopen(temp_file, "r");

        if (!file) { // if file does no open return -1 for error
            unlink(temp_file);
            return -1;
        }

        int distance = 0;
        int error_check = fscanf(file, "%d", &distance); // check if there is issue with the file

        if (error_check != 1){
            fclose(file);
            unlink(temp_file);
            return -1;
            
        }

        fclose(file);
        unlink(temp_file);

        r->total_dist = distance;

        return 0;

    }

}

void permute(route* r, int left, int right, route* best) {
    if (left == right) {
        
        // Parent no longer calculates the distance, the child (helper func) does
        
        //printf("parent pid=%d\n", getpid()); // test print to debug
        
        if (compute_distance_child(r) == 0) {
            
            if (r->total_dist < best->total_dist) {
                memcpy(best, r, sizeof(route));
            } 
        
        } else {
                fprintf(stderr, "Child failed to calculate the distance\n"); // debugging help
            }

            return;
    }

    for (int i = left; i <= right; i++) {
        swap(&r->cities[left], &r->cities[i]);
        permute(r, left + 1, right, best);
        swap(&r->cities[left], &r->cities[i]);
    }
}   

void assign_best(route** best, route* candidate) {
    if (*best == NULL) {
        *best = candidate;
        return;
    }

    int a = candidate->total_dist;
    int b = (*best)->total_dist;

    if (a < b) {
        free(*best);
        *best = candidate;
    } else {
        free(candidate);
    }
}

route* find_best_route( ) {
    route* candidate = malloc( sizeof(route) );
    memcpy (candidate->cities, initial_vector, CITIES_LENGTH * sizeof( int ));
    candidate->total_dist = 0;

    route* best = malloc( sizeof(route) );
    memset( best, 0, sizeof(route) );
    best->total_dist = 999999;

    permute( candidate, 1, 5, best );

    free( candidate );
    return best;
}

int main( int argc, char** argv ) {
    route * best = find_best_route( );
    print_route( best );
    printf( "Distance: %d\n", best->total_dist ); 
    free( best );
    return 0;
}

