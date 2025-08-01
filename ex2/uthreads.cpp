//
// Created by yousefak on 4/19/23.
//

#include "uthreads.h"
#include "scheduler.h"
#include <vector>
#include <csetjmp>
#include "jmp.h"
#include <cstdio>
#include <csignal>
#include <sys/time.h>
#include <iostream>


#define SIGACTION_ERROR "sigaction error\n"
#define SET_TIMER_ERROR "set itimer error\n"
#define SYSTEM_CALL_ERROR "system error: "
#define USEC_TO_SEC 1000000;


#define LIBRARY_ERROR "thread library error: "

typedef void (*thread_entry_point)();
/* External interface */

static Scheduler *scheduler;
struct sigaction sa = {0};
struct itimerval timer;
sigset_t set;

sigjmp_buf* env;

void mask_alarm(){
    sigprocmask(SIG_BLOCK, &set, NULL);
}

void unmask_alarm() {
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}
/**
 * this function returns the first tid available
 * @return tid on success -1 otherwise
 */
int get_new_tid() {
    for(int i = 0; i < MAX_THREAD_NUM; i ++){
        if(scheduler->allThreads[i].tid == -1){
            return i;
        }
    }
    return -1;
}

void decrease_sleep(int tid){
    for(int i = 0; i < MAX_THREAD_NUM; i ++){
        if(scheduler->allThreads[i].tid != -1 && scheduler->allThreads[i].sleep > 0){
//            if(i != tid){
            scheduler->allThreads[i].sleep -= 1;
//            }
            if(scheduler->allThreads[i].is_sleep && scheduler->allThreads[i].sleep <= 0){
                scheduler->exit_sleep(i);
            }
        }
    }
}

/**
 * this function calls the jump function in jmp to switch between threads
 * increases the scheduler->quantum
 */
void jump(int tid, void (*func)(int, sigjmp_buf *), int prevtid) {
    decrease_sleep(prevtid);
    scheduler->quantum += 1; // check
    func(tid, env);
    scheduler->running->quantum += 1;
}

/**
 * preempting the next thread :
 * mask timer signals
 * update quantum of the thread
 * update the timer
 * update the data structure and jump to the next thread.
 * @return 0 upon success -1 upon failure
 */
int preempt(){
    scheduler->preempt();
    while(scheduler->running->sleep > 0){
        scheduler->preempt();
    }
    jump(scheduler->running->tid, &yield, -1);
    return 0;
}

/**
 * handles the timer signals (SIGVTALRM)
 * @param sig
 */
void timer_handler(int sig)
{
    mask_alarm();
    preempt();
    unmask_alarm();
}


int init_time(int quantum_usecs){

    // Install timer_handler as the signal handler for SIGVTALRM.
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        fprintf(stderr,SYSTEM_CALL_ERROR SIGACTION_ERROR);
    }

    int secs = quantum_usecs/USEC_TO_SEC;
    int usecs = quantum_usecs%USEC_TO_SEC;

    // Configure the timer to expire after 1 sec... */
    timer.it_value.tv_sec = secs;        // first time interval, seconds part
    timer.it_value.tv_usec = usecs;        // first time interval, microseconds part

    // configure the timer to expire every 3 sec after that.
    timer.it_interval.tv_sec = secs;    // following time intervals, seconds part
    timer.it_interval.tv_usec = usecs;    // following time intervals, microseconds part

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        fprintf(stderr, SYSTEM_CALL_ERROR SET_TIMER_ERROR);
    }

    return 0;
}

/**
 * @brief initializes the thread library.
 *
 * Once this function returns, the main thread (tid == 0) will be set as RUNNING. There is no need to
 * provide an entry_point or to create a stack for the main thread - it will be using the "regular" stack and PC.
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs){
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    if(quantum_usecs <= 0){
      fprintf(stderr, LIBRARY_ERROR "The value of quantum_usecs should be greater than 0\n");
      return -1;
    }
    scheduler = new Scheduler(MAX_THREAD_NUM, STACK_SIZE);
    env = new sigjmp_buf[MAX_THREAD_NUM];
    if(!env){
        fprintf(stderr, SYSTEM_CALL_ERROR "ERROR ALLOCATING MEMORY");
        exit(1);
    }
    if(scheduler->spawn(0, nullptr) == 0 && scheduler->schedule() == 0){
        init_time(quantum_usecs);
        scheduler->quantum += 1;
        return 0;}
    else{return -1;}
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 * It is an error to call this function with a null entry_point.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point){
    mask_alarm();
    int tid = get_new_tid();
    if(tid == -1 || tid >= MAX_THREAD_NUM) {
        fprintf(stderr, LIBRARY_ERROR "you reached the max number of threads\n");
        unmask_alarm();
        return -1;
    }
    if(entry_point == nullptr ){
        fprintf(stderr, LIBRARY_ERROR "The entry_point should not be a null pointer\n");
        unmask_alarm();
        return -1;}
    if(scheduler->spawn(tid ,&entry_point)==-1){
        return -1;}
    setup_thread(tid, scheduler->allThreads[tid].stack,entry_point, env, STACK_SIZE);
    unmask_alarm();
    return tid;

}


/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid){
    mask_alarm();
    if(tid < 0 || tid >= MAX_THREAD_NUM || scheduler->allThreads[tid].tid == -1){
        std::cerr << LIBRARY_ERROR<< "no thread with ID tid exists" << std::endl;
        unmask_alarm();
        return -1;}
    else if(tid == 0){
        delete scheduler;
        delete[] env;
        exit(0);
    }
    if(scheduler->allThreads[tid].status == RUNNING && !scheduler->allThreads[tid].is_sleep) {
        scheduler->terminate(tid);
        jump(scheduler->running->tid, &jump_to_thread,-1);
    }else{
//        decrease_sleep(-1);
        scheduler->terminate(tid);
    }
    unmask_alarm();
    return 0;
}


/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid){
    mask_alarm();
    if(tid < 0 || tid >= MAX_THREAD_NUM || scheduler->allThreads[tid].tid == -1){
        fprintf(stderr, LIBRARY_ERROR  "no thread with ID tid exists");
        unmask_alarm();
        return -1;
    }
    else if(tid == 0){
        fprintf( stderr, LIBRARY_ERROR "cannot block the main thread");
        unmask_alarm();
        return -1;
    }
    if(scheduler->allThreads[tid].status == BLOCKED){
        unmask_alarm();
        return 0;
    }
    if(scheduler->running->tid == tid){
        scheduler->block(tid);
        jump(scheduler->running->tid, &yield, -1);
        unmask_alarm(); // returning from a blocked position
        return 0;
    }
    scheduler->block(tid);
    unmask_alarm();
    return 0;
}


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid){
    mask_alarm();
    if(tid < 0 || tid >= MAX_THREAD_NUM){
        fprintf(stderr, LIBRARY_ERROR "no thread with this tid exists\n");
        return -1;
    }
    if (scheduler->allThreads[tid].tid == -1){
        fprintf(stderr,LIBRARY_ERROR "no thread to resume \n");
        return -1;
    }
    scheduler->resume(tid);
    unmask_alarm();
    return 0;
}


/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY queue.
 * If the thread which was just RUNNING should also be added to the READY queue, or if multiple threads wake up
 * at the same time, the order in which they're added to the end of the READY queue doesn't matter.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main thread (tid == 0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums){
    int tid = scheduler->running->tid;
    if (tid <= 0 || tid >= MAX_THREAD_NUM) {
        fprintf(stderr, LIBRARY_ERROR "can not put the main thread to sleep and can not exceed the max thread number\n");
        return -1;
    }
    if (num_quantums <= 0) {
        fprintf(stderr, LIBRARY_ERROR "num_quantums must be equal or grater than 0.\n");
        return -1;
    }
    mask_alarm();
    if(scheduler->sleep(tid, num_quantums) == 0){
        jump(scheduler->running->tid, &yield, -1);
        unmask_alarm();
        return 0;
    }
    unmask_alarm();
    return -1;
}


/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid(){
    return scheduler->running->tid;
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums(){
    return scheduler->quantum;
}


int uthread_get_quantums(int tid){
    mask_alarm();
    if (tid < 0){
        fprintf(stderr, LIBRARY_ERROR "thread %d does not exist (no get quantums)\n",tid);
        unmask_alarm();
        return -1;
    }
    if(scheduler->allThreads[tid].tid == -1){
        fprintf(stderr, LIBRARY_ERROR "thread %d does not exist (no get quantums)\n",tid);
        unmask_alarm();
        return -1;
    }
    unmask_alarm();
    return scheduler->allThreads[tid].quantum;
}
