/*
    Georgios Korkolis - 101052327
    Assignment 3 Draft - Lab 10, Lab 11
    Code Blocks were taken from the Beginnning Linux Programming Book on Chapter 12 - Threads
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>

// Total number of threads
#define NUM_THREADS 4
#define MAX_NUM_PROCESSES 100
#define NUM_PROCESSES_PER_THREAD 5

// Input/Output string file path  
// I will be outputing the results onto an output text
#define OUTPUT_FILE "output_text.txt"
#define INPUT_FILE "input_text.txt"

// Used in the Functions Header
#define MAX_LINES 200       // Maximum of number of lines in a text file
#define MAX_LEN 1000        // Maximum expected length for any particular line in the file

// Process Information structure
typedef struct process_info{
    int PID;
    int static_prio;
    int dynamic_prio;
    int time_slice, accu_time_slice, remain_time;
    int execution_time;
    pthread_t thread_id;
    enum sched_policy {RR, FIFO, NORMAL} sched_enum;
    char sched_policy[100];
    struct sched_param sched;
}process_info;

typedef struct thread_queue{
    struct process_info rq0_job_queue[100];
    int total_processes_count;
    // Thread ID
    pthread_t thread_id;
    // Total Amounts of each scheduling
    int rr_count, fifo_count, normal_count;
}thread_queue;

typedef struct iteration_info{
    int num_processes;
    int modulo_res;
    int iterations;
    int total_fifo;
    int total_rr;
    int total_normal;
}iteration_info;
