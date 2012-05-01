#include "project.h"

const int DIR_EMPTY = -1;

shared int lock_holder;
shared int time_offset;

shared strict volatile int *sentinel;
shared sintpt *s_directory;
shared slockpt *dir_locks;

int reads, writes;

shared int *s_time;
shared int *s_read;
shared int *s_write;

void setup_limited_directory(int max_dir_size)
{
    s_time  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    s_read  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    s_write = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    
    sentinel = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    
    s_directory  = (shared sintpt *) upc_all_alloc( THREADS, sizeof(sintpt) );
    s_directory[MYTHREAD] = (sintpt) upc_alloc(max_dir_size * sizeof(int));
    
    dir_locks = (shared slockpt *) upc_all_alloc(THREADS, sizeof(slockpt));
    dir_locks[MYTHREAD] = upc_global_lock_alloc();
    
    upc_barrier;
}

void cleanup_limited_directory()
{
    upc_lock_free(dir_locks[MYTHREAD]);
    upc_free(s_directory[MYTHREAD]);
    
    upc_barrier;
    
    if(MYTHREAD == 0) upc_free(dir_locks);
    if(MYTHREAD == 0) upc_free(s_directory);
    if(MYTHREAD == 0) upc_free((shared int *) sentinel);
    
    if(MYTHREAD == 0) upc_free(s_write);
    if(MYTHREAD == 0) upc_free(s_read);
    if(MYTHREAD == 0) upc_free(s_time);
}

void reset_limited_directory(shared strict volatile int *valid, int dir_size)
{
    volatile int * l_directory = (int *) s_directory[MYTHREAD];
    for(int i = 0; i < dir_size; i++)
    {
        l_directory[i] = DIR_EMPTY;
    }
    
    valid[MYTHREAD] = 0;
    sentinel[MYTHREAD] = 0;
    
    reads = 0;
    writes = 0;
}

void stall_limited_directory(shared strict volatile int *valid, int dir_size)
{
    if(valid[lock_holder] == 0)
    {
        upc_lock(dir_locks[lock_holder]); writes++; reads++;
        
        sintpt rem_dir = s_directory[lock_holder]; reads++;
        
        int entry;
        for(entry = 0; entry < dir_size; entry++)
        {
            reads++;
            if(rem_dir[entry] == DIR_EMPTY)
            {
                rem_dir[entry] = MYTHREAD; writes++;
                break;
            }
        }
        
        upc_unlock(dir_locks[lock_holder]); writes++;
        
        if(entry < dir_size)
        {
            while(sentinel[MYTHREAD] == 0) {}
        }
        else
        {
            while(valid[lock_holder] == 0) {reads++;}
        }
    }
}

void unlock_limited_directory(shared strict volatile int *valid, int dir_size)
{
    volatile int * l_directory = (int *) s_directory[MYTHREAD];
    
    valid[MYTHREAD] = 1;
    for(int i = 0; i < dir_size; i++)
    {
        int lookup = l_directory[i];
        if(lookup != DIR_EMPTY)
        {
            l_directory[lookup] = DIR_EMPTY;
            sentinel[lookup] = 1;
            writes++;
        }
    }
}

void test_limited_directory(shared int * data, shared strict volatile int *valid, int num_threads, int dir_size)
{
    if(MYTHREAD == 0) lock_holder = rand() % num_threads;
    
    upc_barrier;
    upc_tick_t start_time = upc_ticks_now();
    upc_tick_t used_time;
    
    reset_limited_directory(valid, dir_size);
    
    upc_barrier;
    
    if(MYTHREAD < num_threads)
    {
        if(MYTHREAD == lock_holder)
        {
            sleep(1);
            
            upc_tick_t write_time = upc_ticks_now();
            data[MYTHREAD] = 1;
            
            // Declare valid
            unlock_limited_directory(valid, dir_size);

            // Declaration complete
            used_time = upc_ticks_now() - start_time;
            time_offset = upc_ticks_to_ns(write_time-start_time);
        }
        else
        {
            // Wait for valid
            stall_limited_directory(valid, dir_size);
            
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
        printf("\nLimited Directory Experiment with %d threads (%d owns)\n", num_threads, lock_holder);
        for(int i = 0; i < num_threads; i++)
        {
            printf("%d: time %d ns, reads %d, writes %d \n", i, s_time[i]-time_offset, s_read[i], s_write[i]);
        }
    }
}
