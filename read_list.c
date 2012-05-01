#include "project.h"

shared int lock_holder;
shared int time_offset;

shared strict volatile int *sentinel;
shared strict volatile int *next_alert;
shared slockpt *dir_locks;

int reads, writes;

shared int *s_time;
shared int *s_read;
shared int *s_write;

void setup_read_list()
{
    s_time  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    s_read  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    s_write = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    
    sentinel = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    
    next_alert = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    
    dir_locks = (shared slockpt *) upc_all_alloc(THREADS, sizeof(slockpt));
    dir_locks[MYTHREAD] = upc_global_lock_alloc();
    
    upc_barrier;
}

void cleanup_read_list()
{
    upc_lock_free(dir_locks[MYTHREAD]);
    
    upc_barrier;
    
    if(MYTHREAD == 0) upc_free(dir_locks);
    if(MYTHREAD == 0) upc_free((shared int *) next_alert);
    if(MYTHREAD == 0) upc_free((shared int *) sentinel);
    
    if(MYTHREAD == 0) upc_free(s_write);
    if(MYTHREAD == 0) upc_free(s_read);
    if(MYTHREAD == 0) upc_free(s_time);
}

void reset_read_list(shared strict volatile int *valid)
{
    next_alert[MYTHREAD] = NO_ALERT;
    valid[MYTHREAD] = 0;
    sentinel[MYTHREAD] = 0;
    
    reads = 0;
    writes = 0;
}

void stall_read_list(shared strict volatile int *valid)
{
    reads++;
    if(valid[lock_holder] == 0)
    {
        upc_lock(dir_locks[lock_holder]); writes++; reads++;
        
        int tail_loc = lock_holder;
        int tail_alert = next_alert[lock_holder]; reads++;
        while(tail_alert != MYTHREAD)
        {
            if(tail_alert == NO_ALERT)
            {
                next_alert[tail_loc] = MYTHREAD; writes++;
                tail_alert = MYTHREAD;
                break;
            }
            else
            {
                tail_loc = tail_alert;
                tail_alert = next_alert[tail_loc]; reads++;
            }
        }
        
        upc_unlock(dir_locks[lock_holder]); writes++;
        
        reads++;
        if(valid[lock_holder] == 0)
        {
            while(sentinel[MYTHREAD] == 0) {}
        }
        
        tail_alert = next_alert[MYTHREAD];
        if(tail_alert != NO_ALERT)
        {
            sentinel[tail_alert] = 1; writes++;
            next_alert[MYTHREAD] = NO_ALERT;
        }
    }
}

void unlock_read_list(shared strict volatile int *valid)
{   
    valid[MYTHREAD] = 1;
    
    int tail_alert = next_alert[MYTHREAD];
    if(tail_alert != NO_ALERT)
    {
        sentinel[tail_alert] = 1; writes++;
        next_alert[MYTHREAD] = NO_ALERT;
    }
    
}

void test_read_list(shared int * data, shared strict volatile int *valid, int num_threads)
{
    if(MYTHREAD == 0) lock_holder = rand() % num_threads;
    
    upc_barrier;
    upc_tick_t start_time = upc_ticks_now();
    upc_tick_t used_time;
    
    reset_read_list(valid);
    
    upc_barrier;
    
    if(MYTHREAD < num_threads)
    {
        if(MYTHREAD == lock_holder)
        {
            sleep(1);
            
            upc_tick_t write_time = upc_ticks_now();
            data[MYTHREAD] = 1;
            
            // Declare valid
            unlock_read_list(valid);

            // Declaration complete
            used_time = upc_ticks_now() - start_time;
            time_offset = upc_ticks_to_ns(write_time-start_time);
        }
        else
        {
            // Wait for valid
            stall_read_list(valid);
            
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
        printf("\nRead List Experiment with %d threads (%d owns)\n", num_threads, lock_holder);
        for(int i = 0; i < num_threads; i++)
        {
            printf("%d: time %d ns, reads %d, writes %d \n", i, s_time[i]-time_offset, s_read[i], s_write[i]);
        }
    }
    
    upc_barrier;
}
