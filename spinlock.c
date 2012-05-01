#include "project.h"

shared int lock_holder;
shared int time_offset;

int reads, writes;

shared int *s_time;
shared int *s_read;
shared int *s_write;

void setup_spinlock()
{
    s_time  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    s_read  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    s_write = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
}

void cleanup_spinlock()
{   
    upc_barrier;
    
    if(MYTHREAD == 0) upc_free(s_write);
    if(MYTHREAD == 0) upc_free(s_read);
    if(MYTHREAD == 0) upc_free(s_time);
}

void test_spinlock(shared int * data, shared strict volatile int *valid, int num_threads)
{
    if(MYTHREAD == 0) lock_holder = rand() % num_threads;
    
    upc_barrier;
    upc_tick_t start_time = upc_ticks_now();
    upc_tick_t used_time;
    
    upc_barrier;
    upc_forall(int i = 0; i < THREADS; i++; &valid[i]) valid[i] = 0;
    upc_barrier;
    
    reads = 0;
    writes = 0;
    
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
    
    s_time[MYTHREAD]  = interval;
    s_read[MYTHREAD]  = reads;
    s_write[MYTHREAD] = writes;
    
    upc_barrier;
    
    if(MYTHREAD == 0)
    {
        printf("\nSpinlock Experiment with %d threads (%d owns)\n", num_threads, lock_holder);
        for(int i = 0; i < num_threads; i++)
        {
            printf("%d: time %d ns, reads %d, writes %d \n", i, s_time[i]-time_offset, s_read[i], s_write[i]);
        }
    }
    
    upc_barrier;
}
