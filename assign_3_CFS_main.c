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
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>

#include "cfs_identifiers.h"

#define MAX(X, Y) (X > Y ? X:Y)
#define MIN(X, Y) (X > Y ? Y:X)

// Functions to be used in this script, can be found after the end of main banner
void *producer(void *arg);
void *consumer(void *arg);
void openFileCreateDataProcessQueue();
void sortingProcessQueue(struct process_info data[25], int s);
bool check_if_all_jobs_finished(struct thread_queue job_queue);
int assignEnumSched(char sched_policy_temp[10]);
void setThreadFlag_finished(int thread_id);
bool threads_are_finished();
void print_Processeslist();
void printProcess_thread(thread_queue this_process, int i);

// Global variables, all functions and int main have access to this
struct process_info data_processes[100];
struct thread_queue thread_job_queue[NUM_THREADS];
struct iteration_info iteration;
char data[MAX_LINES][MAX_LEN];
int PID_STARTING_VALUE = 1000;
char str_filter_2[150][10];
int num_processes;
int producer_finished = 0;
bool thread_1_finished = false, thread_2_finished = false, thread_3_finished = false, thread_4_finished = false;

//******************************************     MAIN    **********************************************************************************************
int main(void){
    // Initialize Important Variables
    int res;
    pthread_t consumer_t[NUM_THREADS];  // 4 Consumer Threads
    pthread_t producer_t;               // 1 Producer Thread
    pthread_attr_t attr;
    struct sched_param sched;

    // Initializing each thread's queue structure information
    // Initialized that all threads start with 0 processes, and starting at 0 for each policy
    for (int i = 0; i < NUM_THREADS; i++){
        thread_job_queue[i].fifo_count = 0, thread_job_queue[i].rr_count = 0, thread_job_queue[i].normal_count = 0;
        thread_job_queue[i].total_processes_count = 0;
    } 
    printf("Hereeee\n");
    // This function will open input file, then read it and extract the information from within it
    // And it will write the data found inside the data_processes queue
    openFileCreateDataProcessQueue(); // ----------------------------------- OPEN, READ AND WRITE DATA FROM INPUT FILE
    // All my processes data are sorted by priority
    print_Processeslist();

    // Creating the threads, adn their attributes *************************************************** THREAD CREATION
    res = pthread_attr_init(&attr);
    if(res != 0){
        printf("Failed to create attribute\n");
    }
    // seeting the thread's attribute as detached so that each thread can exit without join
    res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(res != 0){
        printf("Failed to set detached\n");
    }
    res = pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    if(res != 0){
        printf("Failed to make sched policy\n");
    }
    int max_priority = sched_get_priority_max(SCHED_OTHER);
    int min_priority = sched_get_priority_min(SCHED_OTHER);
    sched.sched_priority = min_priority;
    res = pthread_attr_setschedparam(&attr, &sched);
    if(res != 0){
        printf("Failed to make scheduling params\n");
    }
    // Creating the producer thread --------------------------------------------------------- PRODUCER THREAD CREATION
    res = pthread_create(&(producer_t), &attr, producer, NULL);
        //error checking for thread
    if (res != 0) {
        perror("producer thread creation failed");
        exit(EXIT_FAILURE);
    }   
    sched.sched_priority = max_priority;
    res = pthread_attr_setschedparam(&attr, &sched);
    if(res != 0){
        printf("Failed to make scheduling params\n");
    }

    // Check to ensure producer is finished
    while(!producer_finished){
        usleep(5000);
    }

    //Create the threads and assign each thread their thread id ----------------------------- CONSUMER THREAD CREATION
    for(int i = 0; i < NUM_THREADS; i++) {
        res = pthread_create(&(consumer_t[i]), &attr, consumer, (void *)&thread_job_queue[i]);
        // res = pthread_create(&(consumer_t[i]), &attr, consumer, (void *)i);
        // Standard error, checking for thread failed
        if (res != 0) {
            perror("Consumer thread creation failed");
            exit(EXIT_FAILURE);
        }else{
            // Assigning thread
            for(int j = 0; j<num_processes;j++){
                thread_job_queue[i].thread_id = consumer_t[i];
            }
        }
        sleep(1);
    }
    // The while loop and the function as a parameter will keep looping if threads are not finished
    // otherwise it will return false and get out of the while loop, thus indicating that all threads are done 
    //while (threads_are_finished()) {};
    sleep(2);
    printf("\nAll Done.\n");
    exit(EXIT_SUCCESS);
}

//******************************************     END MAIN    ****************************************************************************************
//***************************************************************************************************************************************************


//***************************************    PRODUCER FUNCTION    ***********************************************************************************
void *producer(void*arg){
    /* Requirements:
            1. Every thread needs 1 fifo, 1 rr, and 3 normal
            2. Assign jobs by priority
    */
    int index, next_thread_fifo = 0, next_thread_rr = 0, next_thread_normal = 0;
    for (int i = 0; i < iteration.num_processes; i++){
        // If the processes is rr then assign it to the thread, it will alternate between all threads
        // in order to ensure the requirements talked about later. NOTE that the data_processes struct is sorted 
        // by highest priority first
        if (data_processes[i].sched_enum == 0){
            index = thread_job_queue[next_thread_rr].total_processes_count;
            thread_job_queue[next_thread_rr].rq0_job_queue[index] = data_processes[i];
            thread_job_queue[next_thread_rr].rq0_job_queue[index].accu_time_slice = 0;
            thread_job_queue[next_thread_rr].rq0_job_queue[index].time_slice = 1000;
            thread_job_queue[next_thread_rr].rq0_job_queue[index].remain_time = thread_job_queue[next_thread_rr].rq0_job_queue[index].execution_time;             
            thread_job_queue[next_thread_rr].rq0_job_queue[index].dynamic_prio = data_processes[i].static_prio;
            thread_job_queue[next_thread_rr].rq0_job_queue[index].thread_id = next_thread_rr;
            // printf("Thread ID %d, PID %d, Policy %s, Static Priority %d\n", next_thread_rr, thread_job_queue[next_thread_rr].rq0_job_queue[index].PID,
            // thread_job_queue[next_thread_rr].rq0_job_queue[index].sched_policy, thread_job_queue[next_thread_rr].rq0_job_queue[index].static_prio);
            thread_job_queue[next_thread_rr].total_processes_count = index + 1;
            thread_job_queue[next_thread_rr].rr_count++;
            next_thread_rr = (next_thread_rr + 1) % NUM_THREADS;
        }
        if (data_processes[i].sched_enum == 1){
            index = thread_job_queue[next_thread_fifo].total_processes_count;
            thread_job_queue[next_thread_fifo].rq0_job_queue[index] = data_processes[i];
            thread_job_queue[next_thread_fifo].rq0_job_queue[index].accu_time_slice = 0;
            thread_job_queue[next_thread_fifo].rq0_job_queue[index].time_slice = 1000;
            thread_job_queue[next_thread_fifo].rq0_job_queue[index].remain_time = thread_job_queue[next_thread_fifo].rq0_job_queue[index].execution_time;
            thread_job_queue[next_thread_fifo].rq0_job_queue[index].thread_id = next_thread_fifo;
            // printf("Thread ID %d, PID %d, Policy %s, Static Priority %d\n", next_thread_fifo, thread_job_queue[next_thread_fifo].rq0_job_queue[index].PID,
            // thread_job_queue[next_thread_fifo].rq0_job_queue[index].sched_policy, thread_job_queue[next_thread_fifo].rq0_job_queue[index].static_prio);
            thread_job_queue[next_thread_fifo].total_processes_count = index + 1;
            thread_job_queue[next_thread_fifo].fifo_count++;
            next_thread_fifo = (next_thread_fifo + 1) % NUM_THREADS;
        }
        if (data_processes[i].sched_enum == 2){
            index = thread_job_queue[next_thread_normal].total_processes_count;
            thread_job_queue[next_thread_normal].rq0_job_queue[index] = data_processes[i];
            thread_job_queue[next_thread_normal].rq0_job_queue[index].accu_time_slice = 0;
            thread_job_queue[next_thread_normal].rq0_job_queue[index].remain_time = thread_job_queue[next_thread_normal].rq0_job_queue[index].execution_time;
            thread_job_queue[next_thread_normal].rq0_job_queue[index].dynamic_prio = data_processes[i].static_prio;
            thread_job_queue[next_thread_normal].rq0_job_queue[index].thread_id = next_thread_normal;
            // printf("Thread ID %d, PID %d, Policy %s, Static Priority %d\n", next_thread_normal, thread_job_queue[next_thread_normal].rq0_job_queue[index].PID,
            // thread_job_queue[next_thread_normal].rq0_job_queue[index].sched_policy, thread_job_queue[next_thread_normal].rq0_job_queue[index].static_prio);
            thread_job_queue[next_thread_normal].total_processes_count = index + 1;
            thread_job_queue[next_thread_normal].normal_count++;
            next_thread_normal = (next_thread_normal + 1) % NUM_THREADS;
        }
        printf("\n");
    }
    printf("Producer successfull distributed all jobs equally.\n\n");
    producer_finished = 1;
    pthread_exit(NULL);
}
//***************************************************************************************************************************************************


//***************************************    CONSUMER FUNCTION    ***********************************************************************************
void *consumer(void*arg){
    struct thread_queue thread_job_queue = *(thread_queue *)arg;
    struct thread_queue this_process;
    int thread_id = thread_job_queue.thread_id;
    int num_processes = thread_job_queue.total_processes_count;
    printf("Thread ID#: %d started working on processes .....\n", thread_id);
    int counter = 0;

    // The function will keep returning the value of false if there are processes to be are executed, else it will keep going and returning true
    while (check_if_all_jobs_finished(thread_job_queue)){ 
        int bonus;
        printf("-----------------------------------------------------------\n");
        printf("Thread ID# %d -- Iteration %d started..\n", thread_id, counter);
        for(int i = 0; i < num_processes; i++){
            // Grabbing the process and printing the value before execution
            this_process.rq0_job_queue[i] = thread_job_queue.rq0_job_queue[i];
            printProcess_thread(this_process, i);
            int sp_value = this_process.rq0_job_queue[i].static_prio;
            //Checking if to see if the process has any execution time left 
            srand(time(NULL));   
            bonus = rand() % 10;
            if (this_process.rq0_job_queue[i].execution_time != 0){
                // If the process is high priority and there is still fifo and rr processes then proceed and do that one
                if ((sp_value < 100) && (this_process.fifo_count != 0) && (this_process.rr_count != 0)){
                    // Checking to see if it Round Robin
                    if (this_process.rq0_job_queue[i].sched_enum == 0){
                        int static_prio = this_process.rq0_job_queue[i].static_prio;
                        if (static_prio < 120){ this_process.rq0_job_queue[i].time_slice = ((140 - static_prio) * 20000);}
                        if (static_prio >= 120){ this_process.rq0_job_queue[i].time_slice = ((140 - static_prio) * 5000);}
                        this_process.rq0_job_queue[i].remain_time = this_process.rq0_job_queue[i].remain_time - this_process.rq0_job_queue[i].time_slice;
                        this_process.rq0_job_queue[i].accu_time_slice += this_process.rq0_job_queue[i].time_slice;
                        usleep(this_process.rq0_job_queue[i].time_slice);
                        // After changes printf
                        printProcess_thread(this_process, i);
                        if (this_process.rq0_job_queue[i].remain_time <= 0){ 
                            // Make printf statement to say it is done
                            this_process.rq0_job_queue[i].execution_time = 0;
                            this_process.rr_count--;
                            printProcess_thread(this_process, i);
                        }
                    }
                    // Check to see if it is FIFO
                    if (this_process.rq0_job_queue[i].sched_enum == 1){
                        this_process.rq0_job_queue[i].accu_time_slice = this_process.rq0_job_queue[i].execution_time;
                        this_process.rq0_job_queue[i].time_slice = this_process.rq0_job_queue[i].execution_time;
                        usleep(this_process.rq0_job_queue[i].execution_time);
                        this_process.fifo_count--;
                        this_process.rq0_job_queue[i].remain_time = 0;
                        this_process.rq0_job_queue[i].execution_time = 0;
                        printProcess_thread(this_process, i);
                    }
                }else{
                // If the threads total_fifo_count is 0 this means no more fifo jobs left 
                // and total_rr_count is 0 this means no more rr jobs left
                // Threfore we are doing the normal jobs 
                    // This is NORMAL - time slice is different and dynamic priorirt changes based on equation
                    this_process.rq0_job_queue[i].time_slice = (rand() % 10)*10;
                    this_process.rq0_job_queue[i].dynamic_prio = MAX(100, MIN(this_process.rq0_job_queue[i].static_prio - bonus + 5, 139));
                    this_process.rq0_job_queue[i].remain_time = this_process.rq0_job_queue[i].remain_time - this_process.rq0_job_queue[i].time_slice;
                    this_process.rq0_job_queue[i].accu_time_slice += this_process.rq0_job_queue[i].time_slice;
                    usleep(this_process.rq0_job_queue[i].time_slice);
                    if (this_process.rq0_job_queue[i].remain_time <= 0){ 
                        // Make Printf statement saying this job is done
                        this_process.rq0_job_queue[i].execution_time = 0;
                        this_process.normal_count--;
                        printProcess_thread(this_process, i);
                    }
                    printProcess_thread(this_process, i);
                } // End of the if statement that checks if the fifos and rr jobs are finished, if yes then it does all the normal jobs
            }else{
                // If execution time is zero, continue onto the next process that its execution time is not equal to 0
                continue;
            } // End of if statement check
        } // End of for loop, looping around processes
        printf("Thread ID#: %d -- Iteration %d ended.\n", thread_id, counter);
        counter++;
    } // End of while loop

    printf("Thread ID#: %d finished all jobs, took %d iterations to complete all jobs.\n",thread_id, counter);
    printf("Thread ID# %d is exiting.\n", thread_id);
    setThreadFlag_finished(thread_id);   // Set the flag that this thread is finished
    pthread_exit(NULL);// Exit thread
}
//***************************************************************************************************************************************************


//******************************************    FUNCTIONS    ****************************************************************************************
void setThreadFlag_finished(int thread_id){ //--------------------------------------------------------- FUNCTION 1
    int thread_id_1 = 0, thread_id_2 = 1, thread_id_3 = 0, thread_id_4 = 0;
    if (thread_id_1 == thread_id){
        thread_1_finished = true;
    }else if (thread_id_2 == thread_id){
        thread_2_finished = true;
    }else if (thread_id_3 == thread_id){
        thread_3_finished = true;
    }else if (thread_id_4 == thread_id){
        thread_4_finished = true;
    }
}

bool threads_are_finished(){ //------------------------------------------------------------------------ FUNCTION 2
    if (thread_1_finished && thread_2_finished && thread_3_finished && thread_4_finished){
        return false;
    }
    return true;
}

void sortingProcessQueue(struct process_info data[MAX_NUM_PROCESSES], int s){ //----------------------- FUNCTION 3
    int i, j;
    struct process_info temp;
    
    for (i = 0; i < s - 1; i++){
        for (j = 0; j < (s - 1-i); j++){
            if (data[j].static_prio > data[j + 1].static_prio){
                temp = data[j];
                data[j] = data[j + 1];
                data[j + 1] = temp;
            } 
        }
    }
}

// Assign Enumeration --------------------------------------------------------------------------------- FUNCTION 4
int assignEnumSched(char sched_policy_temp[10]){
    char rr[] = "RR", fifo[] = "FIFO", normal[] = "NORMAL";
    
    if (strcmp(sched_policy_temp,rr) == 0 ){
        // This means that it is the round robin schedule
        return 0;
    }else if(strcmp(sched_policy_temp,fifo) == 0 ){
        // This means that it is the fifo policy schedule
        return 1;
    }else if(strcmp(sched_policy_temp,normal) == 0 ){
        // This means that it is the normal policy schedule
        return 2;
    }else{
        return 99;
    }
}

// ----------------------------------------------------------------------------------------------------- FUNCTION 5
void print_Processeslist(){
    for (int i = 0; i<num_processes; i++){
        printf("PID: %d\tScheduling Policy: %s\tSP: %d\tExecution Time: %d\n",
        data_processes[i].PID, data_processes[i].sched_policy, 
        data_processes[i].static_prio, data_processes[i].execution_time);
    }
    printf("\nTotal Number of FIFO's %d\nTotal Number of Round Robin's %d\nTotal Number of Normal's: %d\n",
    iteration.total_fifo, iteration.total_rr, iteration.total_normal);
}

// ----------------------------------------------------------------------------------------------------- FUNCTION 6
bool check_if_all_jobs_finished(struct thread_queue job_queue){
    int num_processes = job_queue.total_processes_count;
    for (int i = 0; i<num_processes; i++){
        if(job_queue.rq0_job_queue[i].execution_time > 0){
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------------------------------------------- FUNCTION 7
void printProcess_thread(thread_queue this_process, int i){
    printf("Thread ID#: %d, PID: %d, Scheduling Policy: %s, Static Priority: %d, Dynamic Priority: %d\n \
    Remaining Execution Time: %dms, Time slice: %dus, Accumulated time slice: %dms\n\n", this_process.thread_id, this_process.rq0_job_queue[i].PID,
    this_process.rq0_job_queue[i].sched_policy, this_process.rq0_job_queue[i].static_prio, this_process.rq0_job_queue[i].dynamic_prio,
    this_process.rq0_job_queue[i].remain_time, this_process.rq0_job_queue[i].time_slice, this_process.rq0_job_queue[i].accu_time_slice);
}

// ----------------------------------------------------------------------------------------------------- FUNCTION 8
// Open File, reading file, and extracting the processes from withing the text file
// Then fill the data processes queue
void openFileCreateDataProcessQueue(){
    // Open the input text file containing the processes
    FILE *input_file;
    input_file = fopen(INPUT_FILE, "r");
    if (input_file == NULL){   
        // if input_file returns -1, it failed to establish access path to text file
        fprintf(stderr,"Failed to establish access path to input file ..\n");
        exit(EXIT_FAILURE);
    }
    // Reading the file's contents and saving it on the data array
    // It does two checks, first check ensures that it read all from file,
    // and second check ensures the maximum limit of lines is maintained
    int line = 0;
    while (!feof(input_file) && !ferror(input_file)){
        // fgets will return null when it reaches the end of the file
        // the if checks if there is more to read from the file
        if (fgets(data[line], MAX_LEN, input_file) != NULL){ 
            line++;
        }
        // Checks to see if we succeeded the maximum limit of lines in the text file
        if (line > MAX_LINES || line == 0){
            fprintf(stderr,"Error either more than 100 lines or 0 zeroes, check input file\n");
            exit(EXIT_FAILURE);
        }
    }
    // Close input_file
    fclose(input_file);
    
    // This counter will have the valid processes, and total number of processes processed from input_text file
    num_processes = 0;
    // Creating the Processes Queue
    for (int j = 0; j < line; j++){
        char string_sched[100]; // "FIFO" or "RR" or "NORMAL/OTHER"
        int prio_value;         // Priority Value
        int exec_time;          // Execution Value
        sscanf(data[j],"%[a-zA-Z ],%d,%d",string_sched, &prio_value, &exec_time);
        // Checks to see if enum is strictly either 0, 1, or 2. Otherwise process wont be added to queue
        int sched_enum = assignEnumSched(strupr(string_sched));
        if (sched_enum == 0 || sched_enum == 1 || sched_enum == 2){
            data_processes[j].sched_enum = sched_enum;                              // Setting the enum
            strcpy(data_processes[num_processes].sched_policy, string_sched);       // String Schedule Name
            data_processes[num_processes].static_prio = prio_value;                 // Priority value, SP
            data_processes[num_processes].execution_time = exec_time;               // Execution Time 
            data_processes[num_processes].PID = num_processes + PID_STARTING_VALUE; // PID value
            num_processes++;
        }else{    
            continue;
        }
    }
    /*
        Sorting the data_processes queue by priority using the function called sortingProcessQueue    
    */
    sortingProcessQueue(data_processes,num_processes);
    // Getting the number of iterations we need in order to distribute any number of processes inputted
    iteration.num_processes = num_processes;
    iteration.modulo_res = iteration.num_processes % NUM_THREADS;
    iteration.iterations = iteration.num_processes / NUM_THREADS;
    if (iteration.modulo_res == 0){iteration.iterations = iteration.num_processes/NUM_THREADS;}
    else{iteration.iterations = (iteration.num_processes/NUM_THREADS) + 1;}
    int counter_fifo = 0, counter_rr = 0, counter_normal = 0;
    for (int i = 0; i<iteration.num_processes; i++){
        if (data_processes[i].sched_enum == 0){
            counter_rr++;
        }else if (data_processes[i].sched_enum == 1){
            counter_fifo++;
        }else
            counter_normal++;
    }
    iteration.total_fifo = counter_fifo;
    iteration.total_rr = counter_rr;
    iteration.total_normal = counter_normal;
}

