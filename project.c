#include "project.h"

int main( int argc, char** argv )
{
    int num_threads = read_int( argc, argv, "-n", THREADS);
    num_threads = min(num_threads, THREADS);
    
    shared int *data;
    shared strict int *valid;
    
    data  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    valid = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );

    srand48( 2012 + MYTHREAD );
    
    upc_forall(int i = 0; i < THREADS; i++; &valid[i]) valid[i] = 0;
    
    if(MYTHREAD == 0) printf("\n");
    
    if(MYTHREAD == 0)
    {
        upc_tick_t time1 = upc_ticks_now();
        upc_tick_t time2 = upc_ticks_now();
        int overhead = upc_ticks_to_ns(time2 - time1);
        
        printf("Overhead: %d\n", overhead);
    }
    
    upc_barrier;
    if(MYTHREAD == 0) printf("\nSpinlock Experiment with %d threads\n", num_threads);
    test_spinlock(data, valid, num_threads);
    
    upc_barrier;
    if(MYTHREAD == 0) printf("\nLimited Directory Experiment with %d threads\n", num_threads);
    setup_limited_directory(4);
    test_limited_directory(data, valid, num_threads, 4);
    cleanup_limited_directory();
    
    upc_barrier;
    if(MYTHREAD == 0) printf("\nWrite List Experiment with %d threads\n", num_threads);
    setup_write_list();
    test_write_list(data, valid, num_threads);
    cleanup_write_list();
    
    upc_barrier;
    
    if(MYTHREAD ==0)
    {
        upc_free((shared int *) valid);
        upc_free(data);
    }
    
    return 0;
}
