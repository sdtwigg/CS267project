#include "project.h"

const int NO_ALERT = -1;

shared int lock_holder;
shared int time_offset;

shared strict volatile int *sentinel;
shared strict int *next_alert;
shared slockpt *dir_locks;

int reads, writes;

void setup_write_list()
{
    sentinel = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    
    next_alert = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    
    dir_locks = (shared slockpt *) upc_all_alloc(THREADS, sizeof(slockpt));
    dir_locks[MYTHREAD] = upc_global_lock_alloc();
    
    upc_barrier;
}

void cleanup_write_list()
{
    upc_lock_free(dir_locks[MYTHREAD]);
    
    upc_barrier;
    
    if(MYTHREAD == 0) upc_free(dir_locks);
    if(MYTHREAD == 0) upc_free((shared int *) next_alert);
    if(MYTHREAD == 0) upc_free((shared int *) sentinel);
}

void reset_write_list(shared strict volatile int *valid)
{
    next_alert[MYTHREAD] = NO_ALERT;
    valid[MYTHREAD] = 0;
    sentinel[MYTHREAD] = 0;
}

void stall_write_list(shared strict volatile int *valid)
{
    if(valid[lock_holder] == 0)
    {
        upc_lock(dir_locks[lock_holder]); writes++; reads++;
        
        int tail_loc = lock_holder;
        int tail_alert = next_alert[lock_holder]; reads++;
        while(tail_alert != MYTHREAD)
        {
            if(tail_alert != NO_ALERT)
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
        
        while(sentinel[MYTHREAD] == 0) {}
    }
}

void unlock_write_list(shared strict volatile int *valid)
{   
    valid[MYTHREAD] = 1;
    
    int tail_loc = MYTHREAD;
    int tail_alert = next_alert[MYTHREAD];
    while(tail_alert != NO_ALERT)
    {
        sentinel[tail_alert] = 1; writes++;
        
        int old_loc = tail_loc;
        tail_loc = tail_alert;
        tail_alert = next_alert[tail_loc]; reads++;
        
        next_alert[old_loc] = NO_ALERT; writes++;
    }
    
}

void test_write_list(shared int * data, shared strict volatile int *valid, int num_threads)
{
    if(MYTHREAD == 0) lock_holder = rand() % num_threads;
    
    upc_barrier;
    upc_tick_t start_time = upc_ticks_now();
    upc_tick_t used_time;
    
    reset_write_list(valid);
    
    upc_barrier;
    
    if(MYTHREAD < num_threads)
    {
        if(MYTHREAD == lock_holder)
        {
            sleep(1);
            
            upc_tick_t write_time = upc_ticks_now();
            data[MYTHREAD] = 1;
            
            // Declare valid
            unlock_write_list(valid);

            // Declaration complete
            used_time = upc_ticks_now() - start_time;
            time_offset = upc_ticks_to_ns(write_time-start_time);
        }
        else
        {
            // Wait for valid
            stall_write_list(valid);
            
            upc_tick_t read_time = upc_ticks_now();
            used_time = read_time - start_time;
        }
        
    }
    int interval = upc_ticks_to_ns(used_time);
    
    upc_barrier;
    if(MYTHREAD < num_threads) printf("%d: time %d ns, reads %d, writes %d \n", MYTHREAD, interval-time_offset, reads, writes);
}
