#include "project.h"

shared int lock_holder;
shared int time_offset;

shared strict volatile int *sentinel;
shared strict volatile int *next_alert_l;
shared strict volatile int *next_alert_r;
shared volatile int *reader_count;
shared slockpt *dir_locks;

static int reads, writes;
static int acc_reads = 0, acc_writes = 0, acc_time = 0, acc_count = 0;
static int max_reads = 0, max_writes = 0, max_time = 0;

shared volatile int *s_time;
shared volatile int *s_read;
shared volatile int *s_write;

void setup_read_tree()
{
    s_time  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    s_read  = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    s_write = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    
    sentinel = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    
    next_alert_l = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    next_alert_r = (shared strict int *) upc_all_alloc( THREADS, sizeof(int) );
    reader_count = (shared int *) upc_all_alloc( THREADS, sizeof(int) );
    
    dir_locks = (shared slockpt *) upc_all_alloc(THREADS, sizeof(slockpt));
    dir_locks[MYTHREAD] = upc_global_lock_alloc();
    
    upc_barrier;
}

void cleanup_read_tree()
{
    upc_lock_free(dir_locks[MYTHREAD]);
    
    upc_barrier;
    
    if(MYTHREAD == 0) upc_free(dir_locks);
    if(MYTHREAD == 0) upc_free((shared int *) reader_count);
    if(MYTHREAD == 0) upc_free((shared int *) next_alert_l);
    if(MYTHREAD == 0) upc_free((shared int *) next_alert_r);
    if(MYTHREAD == 0) upc_free((shared int *) sentinel);
    
    if(MYTHREAD == 0) upc_free((shared int *) s_write);
    if(MYTHREAD == 0) upc_free((shared int *) s_read);
    if(MYTHREAD == 0) upc_free((shared int *) s_time);
}

void reset_read_tree(shared strict volatile int *valid)
{
    next_alert_l[MYTHREAD] = NO_ALERT;
    next_alert_r[MYTHREAD] = NO_ALERT;
    reader_count[MYTHREAD] = 0;
    valid[MYTHREAD] = 0;
    sentinel[MYTHREAD] = 0;
    
    reads = 0;
    writes = 0;
}

void stall_read_tree(shared strict volatile int *valid)
{
    reads++;
    if(valid[lock_holder] == 0)
    {
        upc_lock(dir_locks[lock_holder]); writes++; reads++;
        
        int reader_id = reader_count[lock_holder]; reads++;
        reader_count[lock_holder] = reader_id + 1; writes++;
        
        int location = reader_id + 2;
        
        int tail_loc = lock_holder;
        while(1)
        {
            if((location & 1) == 0)
            {
                int lookup = next_alert_l[tail_loc]; reads++;
                if(lookup == NO_ALERT)
                {
                    next_alert_l[tail_loc] = MYTHREAD; writes++;
                    break;
                }
                else
                {
                    location = location >> 1;
                    tail_loc = lookup;
                }
            }
            else
            {
                int lookup = next_alert_r[tail_loc]; reads++;
                if(lookup == NO_ALERT)
                {
                    next_alert_r[tail_loc] = MYTHREAD; writes++;
                    break;
                }
                else
                {
                    location = location >> 1;
                    tail_loc = lookup;
                }
            }
        }
        
        upc_unlock(dir_locks[lock_holder]); writes++;
        
        reads++;
        if(valid[lock_holder] == 0)
        {
            while(sentinel[MYTHREAD] == 0) {}
        }
        else printf("%d: TIMEOUT ISSUE\n", MYTHREAD);
        
        int tail_alert;
        tail_alert = next_alert_l[MYTHREAD];
        if(tail_alert != NO_ALERT)
        {
            sentinel[tail_alert] = 1; writes++;
            next_alert_l[MYTHREAD] = NO_ALERT;
        }
        tail_alert = next_alert_r[MYTHREAD];
        if(tail_alert != NO_ALERT)
        {
            sentinel[tail_alert] = 1; writes++;
            next_alert_r[MYTHREAD] = NO_ALERT;
        }
    }
}

void unlock_read_tree(shared strict volatile int *valid)
{   
    valid[MYTHREAD] = 1;
    
    int tail_alert_l = next_alert_l[MYTHREAD];
    if(tail_alert_l != NO_ALERT)
    {
        sentinel[tail_alert_l] = 1; writes++;
        next_alert_l[MYTHREAD] = NO_ALERT;
    }
    int tail_alert_r = next_alert_r[MYTHREAD];
    if(tail_alert_r != NO_ALERT)
    {
        sentinel[tail_alert_r] = 1; writes++;
        next_alert_r[MYTHREAD] = NO_ALERT;
    }
    
}

void test_read_tree(shared int * data, shared strict volatile int *valid, int num_threads)
{
    if(MYTHREAD == 0) lock_holder = rand() % num_threads;
    
    upc_barrier;
    upc_tick_t start_time = upc_ticks_now();
    upc_tick_t used_time;
    
    reset_read_tree(valid);
    
    upc_barrier;
    
    if(MYTHREAD < num_threads)
    {
        if(MYTHREAD == lock_holder)
        {
            sleep(1);
            
            upc_tick_t write_time = upc_ticks_now();
            data[MYTHREAD] = 1;
            
            // Declare valid
            unlock_read_tree(valid);

            // Declaration complete
            used_time = upc_ticks_now() - start_time;
            time_offset = upc_ticks_to_ns(write_time-start_time);
        }
        else
        {
            // Wait for valid
            stall_read_tree(valid);
            
            upc_tick_t read_time = upc_ticks_now();
            used_time = read_time - start_time;
        }
        
    }
    int interval = upc_ticks_to_ns(used_time);
    
    s_time[MYTHREAD]  = interval;
    s_read[MYTHREAD]  = reads;
    s_write[MYTHREAD] = writes;
    
    upc_barrier;
    
    int time_diff = interval - time_offset;
    if((time_diff > 0) && (lock_holder != MYTHREAD))
    {
        acc_reads += reads;
        acc_writes += writes;
        acc_time += time_diff;
        acc_count += 1;
        
        max_reads = max(reads, max_reads);
        max_writes = max(writes, max_writes);
        max_time = max(time_diff, max_time);
    }
    
    if(MYTHREAD == 0)
    {
        printf("\nRead Tree Experiment with %d threads (%d owns)\n", num_threads, lock_holder);
        for(int i = 0; i < num_threads; i++)
        {
            printf("%d: time %d ns, reads %d, writes %d \n", i, s_time[i]-time_offset, s_read[i], s_write[i]);
        }
    }
    
    upc_barrier;
}

void stats_read_tree(int num_threads)
{
    acc_count = max(acc_count, 1);

    s_time[MYTHREAD]  = acc_time/acc_count;
    s_read[MYTHREAD]  = acc_reads/acc_count;
    s_write[MYTHREAD] = acc_writes/acc_count;
    
    upc_barrier;
    
    if(MYTHREAD == 0)
    {
        printf("\nSUMMARY: Read Tree Experiment with %d threads\n", num_threads);
        int t_time=0, t_reads=0, t_writes=0;
        for(int i = 0; i < num_threads; i++)
        {
            t_time += s_time[i]; t_reads += s_read[i]; t_writes += s_write[i];
        }
        printf("avg: %d ns, %d reads, %d writes\n", t_time/num_threads, t_reads/num_threads, t_writes/num_threads);
    }
    
    upc_barrier;

    s_time[MYTHREAD]  = max_time;
    s_read[MYTHREAD]  = max_reads;
    s_write[MYTHREAD] = max_writes;
    
    upc_barrier;
    
    if(MYTHREAD == 0)
    {
        int t_time=0, t_reads=0, t_writes=0;
        for(int i = 0; i < num_threads; i++)
        {
            t_time = max(t_time, s_time[i]);
            t_reads = max(t_reads, s_read[i]);
            t_writes = max(t_writes, s_write[i]);
        }
        printf("max: %d ns, %d reads, %d writes\n", t_time, t_reads, t_writes);
    }
    
    upc_barrier;
}
