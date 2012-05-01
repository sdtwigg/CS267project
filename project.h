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

#define NO_ALERT -1

typedef shared [] int * sintpt; // pointer to shared integer (if array, all on same thread)
typedef shared upc_lock_t * slockpt; // pointer to shared lock

void setup_spinlock();
void test_spinlock(shared int * data, shared strict volatile int *valid, int num_threads);
void cleanup_spinlock();

void setup_limited_directory(int max_dir_size);
void test_limited_directory(shared int * data, shared strict volatile int *valid, int num_threads, int dir_size);
void cleanup_limited_directory();

void setup_write_list();
void test_write_list(shared int * data, shared strict volatile int *valid, int num_threads);
void cleanup_write_list();

void setup_read_list();
void test_read_list(shared int * data, shared strict volatile int *valid, int num_threads);
void cleanup_read_list();

void setup_read_tree();
void test_read_tree(shared int * data, shared strict volatile int *valid, int num_threads);
void cleanup_read_tree();

inline int max( int a, int b ) { return a > b ? a : b; }
inline int min( int a, int b ) { return a < b ? a : b; }

int find_option( int argc, char **argv, const char *option );
int read_int( int argc, char **argv, const char *option, int default_value );

#endif
