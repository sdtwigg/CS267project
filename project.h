#ifndef __PROJECT_H
#define __PROJECT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <upc.h>
#include <upc_cray.h>
#include <string.h>

typedef shared [] int * sintpt; // pointer to shared integer (if array, all on same thread)
typedef shared upc_lock_t * slockpt; // pointer to shared lock


double read_timer();

void test_spinlock(shared int * data, shared strict volatile int *valid, int num_threads);
void test_limiteddirectory(shared int * data, shared strict volatile int *valid, shared strict volatile int *sentinel, int num_threads, int dir_size);

inline int max( int a, int b ) { return a > b ? a : b; }
inline int min( int a, int b ) { return a < b ? a : b; }

int find_option( int argc, char **argv, const char *option );
int read_int( int argc, char **argv, const char *option, int default_value );

#endif
