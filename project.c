#include "project.h"

const int num_trials = 16;

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
    setup_spinlock();
    for(int i = 0; i < num_trials; i++) test_spinlock(data, valid, num_threads);
    cleanup_spinlock();
    
//    upc_barrier;
//    setup_limited_directory(4);
//    test_limited_directory(data, valid, num_threads, 4);
//    cleanup_limited_directory();

    upc_barrier;
    setup_write_list();
    for(int i = 0; i < num_trials; i++) test_write_list(data, valid, num_threads);
    cleanup_write_list();
    
    upc_barrier;
    setup_read_list();
    for(int i = 0; i < num_trials; i++) test_read_list(data, valid, num_threads);
    cleanup_read_list();
    
    upc_barrier;
    setup_read_tree();
    for(int i = 0; i < num_trials; i++) test_read_tree(data, valid, num_threads);
    cleanup_read_tree();
    
    upc_barrier;
    stats_spinlock(num_threads);
    stats_write_list(num_threads);
    stats_read_list(num_threads);
    stats_read_tree(num_threads);
    
    if(MYTHREAD ==0)
    {
        upc_free((shared int *) valid);
        upc_free(data);
    }
    
    return 0;
}
