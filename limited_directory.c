#include "project.h"

const int DIR_EMPTY = -1;

shared int lock_holder;
shared int time_offset;

shared strict volatile int *sentinel;
shared sintpt *s_directory;
shared slockpt *dir_locks;

void setup_limiteddirectory(int max_dir_size)
{
    sentinel = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    
    s_directory  = (shared sintpt *) upc_all_alloc( THREADS, sizeof(sintpt) );
    s_directory[MYTHREAD] = (sintpt) upc_alloc(max_dir_size * sizeof(int));
    
    dir_locks = (shared slockpt *) upc_all_alloc(THREADS, sizeof(slockpt));
    dir_locks[MYTHREAD] = upc_global_lock_alloc();
    
    upc_barrier;
}

void cleanup_limitteddirectory()
{
    upc_lock_free(dir_locks[MYTHREAD]);
    upc_free(s_directory[MYTHREAD]);
    
    upc_barrier;
    
    if(MYTHREAD == 0) upc_free(dir_locks);
    if(MYTHREAD == 0) upc_free(s_directory);
    if(MYTHREAD == 0) upc_free((shared int *) sentinel);
}

void test_limiteddirectory(shared int * data, shared strict volatile int *valid, int num_threads, int dir_size)
{
    if(MYTHREAD == 0) lock_holder = rand() % num_threads;
    
    upc_barrier;
    upc_tick_t start_time = upc_ticks_now();
    upc_tick_t used_time;
    
    volatile int * l_directory = (int *) s_directory[MYTHREAD];
    for(int i = 0; i < dir_size; i++)
    {
        l_directory[i] = DIR_EMPTY;
    }
    
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
            for(int i = 0; i < dir_size; i++)
            {
                int lookup = l_directory[i];
                if(lookup != DIR_EMPTY)
                {
                    sentinel[lookup] = 1;
                    writes++;
                }
            }
            // Declaration complete
            time_offset = upc_ticks_to_ns(write_time-start_time);
            used_time = upc_ticks_now() - start_time;
        }
        else
        {
            // Wait for valid
            sentinel[MYTHREAD] = 0;
            reads++;
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
            // Data is valid
            
            upc_tick_t read_time = upc_ticks_now();
            used_time = read_time - start_time;
        }
        
    }
    int interval = upc_ticks_to_ns(used_time);
    
    upc_barrier;
    if(MYTHREAD < num_threads) printf("%d: time %d ns, reads %d, writes %d \n", MYTHREAD, interval-time_offset, reads, writes);
}
