//
// Created by yousefak on 4/19/23.
//

#include <cstdio>
#include "scheduler.h"
#define SYSTEM_CALL_ERROR "system error: "

/**
 * spawn a Thread by changing allThreads[tid] to tid instead of -1
 * add the Thread to the readyVed
 * set the quantum to quantum
 * @param tid
 * @param entry_point
 * @return 0 on success -1 otherwise
 */
int Scheduler::spawn(int tid, thread_entry_point * entry_point){
    if(allThreads[tid].tid != -1){return -1;}
    readyVec.push(&allThreads[tid]);
    allThreads[tid].tid = tid;
    allThreads[tid].status = READY;
    allThreads[tid].entry_point = entry_point;
    allThreads[tid].quantum = 1;
    allThreads[tid].sleep = 0;
    allThreads[tid].is_sleep = false;
    if(tid != 0){

        allThreads[tid].stack = new char[max_stack_size];
        if(!allThreads[tid].stack){
            fprintf(stderr, SYSTEM_CALL_ERROR "ERROR ALLOCATING MEMORY");
        }
    }
    return 0;
}

/**
 * update the running Thread to READY status
 * if it's sleep <= 0
 * schedule the next thread to running and add the running Thread to the back of the readyVec queue
 * else if sleep > 0 schedule another thread to run
 * @return 0
 */
int Scheduler::preempt(){
    if(running->sleep <= 0){
        running->status = READY;
        readyVec.push(running);
    }
    schedule();
    return 0;
}

/**
 *
 * update the front ready Thread in the readyVec to running pointer
 * change its state RUNNING state
 * pop the readyVec queue
 * @return 0 upon success -1 otherwise
 */
int Scheduler::schedule(){
    if(readyVec.empty() || readyVec.front()->tid == -1){
        return -1;
    }
    running = readyVec.front();
    running->status = RUNNING;
    readyVec.pop();
    return 0;
}

/**
 * blocks the Thread by changing the state of the Thread in the allThreads list to BLOCKED
 * and if it was running schedule the next Thread
 * and remove the Thread from the ready queue
 * @param tid the id of the Thread to block
 * @return 0 on success
 *          -1 on failure
 */
int Scheduler::block(int tid){
    if(tid < 0 || tid >= max_stack_size){
        return -1;
    }
    if(running && tid == running->tid && !running->is_sleep){
        allThreads[tid].status = BLOCKED;
        schedule();
        return 0;
    }if(running->is_sleep){
        allThreads[tid].status = BLOCKED;
        return 0;
    }
    allThreads[tid].status = BLOCKED;
    removeFromReadyVec(tid);
    return 0;
}

 /**
  * resumes the Tread by changing the state of the Thread in the allThreads list to READY
  * and add it to the ready queue
  * if the thread is does not exist return an error
  * if the thread is in RUNNING OR READY status does nothing
  * if  the thread is sleeping change the status to ready and do not add the thread to the ready queue
  * @param tid
  * @return 0 on success -1 otherwise
*/
int Scheduler::resume(int tid){
    if(allThreads[tid].tid == -1){
        return -1;
    }
    if(allThreads[tid].is_sleep){
        allThreads[tid].status = READY;
        return 0;
    }
    if(allThreads[tid].status == RUNNING || allThreads[tid].status == READY){return 0;}
    allThreads[tid].status = READY;
    readyVec.push(&allThreads[tid]);
    return 0;
}

/**
 * set the tid of the thread at allThreads[tid] to -1
 * set the quantum to 0
 * and remove it from the readyVec
 * if tid does not exist return -1
 * if the running thread is asked to be terminated return 1
 * @param tid
 * @return  1 if the thread to be terminated is the running thread
 *          0 if the thread is terminated successfully
 *          -1 if the thread does not exist
 */
int Scheduler::terminate(int tid){
    if(allThreads[tid].tid == -1){
        return -1;
    }
    if(allThreads[tid].is_sleep){
        remove_from_sleepVec(tid);
    }
    else if(allThreads[tid].status == READY){
        removeFromReadyVec(tid);
    }
    delete[] allThreads[tid].stack;
    allThreads[tid].stack = nullptr;
    allThreads[tid].tid = -1;
    allThreads[tid].quantum = 0;
    allThreads[tid].sleep = 0;
    if(allThreads[tid].status == RUNNING && !allThreads[tid].is_sleep){
        schedule();
        return 1;
    }
    allThreads[tid].is_sleep = false;
    allThreads[tid].status = READY;
    return 0;
}


/**
 * this function updates the thread sleep quantum and the data structure in Scheduler by :
 * if the thread is in ready status -> remove it from the ready queue, and add it to the sleep queue
 * if the thread is in blocked status -> just add it to the sleep queue,
 * if the thread is in running status -> add it to the sleep queue and preempt the next thread.
 * @param tid
 * @return 0 upon success
 *         -1 otherwise
 */
int Scheduler::sleep(int tid, int sleep_quantum) {
    if(tid <= 0){
        return -1;
    }
    allThreads[tid].sleep = sleep_quantum;
    allThreads[tid].is_sleep = true;
    if(allThreads[tid].status == RUNNING){
        sleepVec.push(running);
        schedule();
        return 0;
    }
    sleepVec.push(&allThreads[tid]);
    return 0;
}

/**
 * iterate over the queue of readyVec and remove tid and blocked thread from the queue
 * @param tid
 */
void Scheduler::removeFromReadyVec(int tid)
{
    if(readyVec.empty()){
        return;
    }
    if(readyVec.front()->tid == tid){
        readyVec.pop();
        return;
    }
    Thread* temp = readyVec.front();
    readyVec.push(readyVec.front());
    readyVec.pop();
    while(temp != readyVec.front()){
        if(readyVec.front()->tid == tid){
            readyVec.pop();
        }
        readyVec.push(readyVec.front());
        readyVec.pop();
    }
}

/**
 * constructor
 * @param max_size
 */
Scheduler::Scheduler(int max_size, int max_stack_size)
{
    this->max_stack_size = max_stack_size;
    allThreads = new Thread[max_size];
    if(!allThreads){
        fprintf(stderr, SYSTEM_CALL_ERROR "ERROR ALLOCATING MEMORY");
    }
    running = nullptr;
    quantum = 0;
}

/**
 * destructor
 */
Scheduler::~Scheduler()
{
    delete[] allThreads;
    allThreads = nullptr;
}

int Scheduler::is_readyVec_empty() {
    return readyVec.empty();
}

void Scheduler::remove_from_sleepVec(int tid){
    if(sleepVec.empty()){return;}
    if(sleepVec.front()->tid == tid){
        sleepVec.pop();
        return;
    }
    Thread* temp = sleepVec.front();
    sleepVec.push(sleepVec.front());
    sleepVec.pop();
    while(temp != sleepVec.front()){
        if(sleepVec.front()->tid == tid){
            sleepVec.pop();
        }
        sleepVec.push(sleepVec.front());
        sleepVec.pop();
    }
}

/**
 * get the thread out of the sleep queue and add it to the ready queue if it is not blocked
 * @param tid
 * @return 0 upon success
 *         -1 otherwise
 */
int Scheduler::exit_sleep(int tid) {
    if(tid < 0){
        return -1;
    }
    if(allThreads[tid].status == RUNNING){
        remove_from_sleepVec(tid);
        readyVec.push(&allThreads[tid]);
        allThreads[tid].status = READY;
    }else if(allThreads[tid].status == BLOCKED){
        remove_from_sleepVec(tid);
    }
    allThreads[tid].is_sleep = false;
    return 0;
}

