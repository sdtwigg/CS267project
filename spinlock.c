#include "project.h"

shared int lock_holder;
shared int time_offset;

void test_spinlock(shared int * data, shared strict volatile int *valid, int num_threads)
{
    if(MYTHREAD == 0) lock_holder = rand() % num_threads;
    
    upc_barrier;
    upc_tick_t start_time = upc_ticks_now();
    upc_tick_t used_time;
    
    upc_barrier;
    upc_forall(int i = 0; i < THREADS; i++; &valid[i]) valid[i] = 0;
    upc_barrier;
    
    int reads = 0;
    int writes = 0;
    
    if(MYTHREAD < num_threads)
    {
        if(MYTHREAD == lock_holder)
        {
            sleep(1);
            
            upc_tick_t write_time = upc_ticks_now();
            data[MYTHREAD] = 1;
            
            // Declare valid
            valid[MYTHREAD] = 1;
            // Declaration complete
            
            used_time = upc_ticks_now() - start_time;
            time_offset = upc_ticks_to_ns(write_time-start_time);
        }
        else
        {
            // Wait for valid
            reads++;
            while(valid[lock_holder] == 0) {reads++;}
            // Data is valid
            
            upc_tick_t read_time = upc_ticks_now();
            used_time = read_time - start_time;
        }
    }
    int interval = upc_ticks_to_ns(used_time);
    
    upc_barrier;
    if(MYTHREAD < num_threads) printf("%d: time %d ns, reads %d, writes %d \n", MYTHREAD, interval-time_offset, reads, writes);
}
