#include "project.h"

int main( int argc, char** argv )
{
    int num_threads = read_int( argc, argv, "-n", THREADS);
    num_threads = min(num_threads, THREADS);
    
    shared int *data;
    shared strict int *valid;
    shared strict int *sentinel;
    
    data  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    valid = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    sentinel = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );

    srand48( 2012 + MYTHREAD );
    
    upc_barrier;
    read_timer(); // Make sure all timers initialized at same time
    
    upc_forall(int i = 0; i < THREADS; i++; &valid[i]) valid[i] = 0;
    
    if(MYTHREAD == 0) printf("\nLimited Directory Experiment with %d threads\n", num_threads);
    
    if(MYTHREAD == 0)
    {
        double overhead = read_timer();
        overhead = read_timer() - overhead;
        
        printf("Overhead: %g\n", overhead);
    }
    
    upc_barrier;
    
    test_limiteddirectory(data, valid, sentinel, num_threads, 4);
    
    upc_barrier;
    
    if(MYTHREAD ==0)
    {
        upc_free((shared int *) sentinel);
        upc_free((shared int *) valid);
        upc_free(data);
    }
    
    return 0;
}
