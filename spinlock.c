#include "project.h"

shared int lock_holder;

void test_spinlock(shared int * data, shared strict volatile int *valid, int num_threads)
{
    if(MYTHREAD == 0) lock_holder = rand() % num_threads;
    double start_time = read_timer();
    double used_time;
    
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
            
            double write_time = read_timer();
            data[MYTHREAD] = write_time;
            
            // Declare valid
            valid[MYTHREAD] = 1;
            // Declaration complete
            
            used_time = read_timer() - write_time;
        }
        else
        {
            // Wait for valid
            reads++;
            while(valid[lock_holder] == 0) {reads++;}
            // Data is valid
            
            double posted_time = data[lock_holder];
            used_time = read_timer() - posted_time;
        }
        
        printf("%d: time %g, reads %d, writes %d \n", MYTHREAD, used_time, reads, writes);
    }
    
    upc_barrier;
}
