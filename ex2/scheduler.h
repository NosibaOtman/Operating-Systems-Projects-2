//
// Created by yousefak on 4/19/23.
//
#include <cstdlib>
#include <queue>
#include <sys/time.h>

#ifndef UTHREADS_H_SCHEDULER_H
#define UTHREADS_H_SCHEDULER_H

enum Status {RUNNING, READY, BLOCKED};

typedef void (*thread_entry_point)();

typedef struct Thread{
    int tid = -1;
    Status status = READY;
    long quantum = 0;
    int sleep = 0;
    bool is_sleep;
    char* stack;
    thread_entry_point *entry_point = nullptr;
}Thread;

using vec = std::queue<Thread*>;

class Scheduler{

    int max_stack_size;
    vec readyVec;
    vec sleepVec;

    void removeFromReadyVec(int tid);
public :

    int quantum = 0;

    int is_readyVec_empty();

    int sleep(int tid, int sleep_quantum);

    int exit_sleep(int tid);

    struct Thread * running;

    struct Thread* allThreads;

    Scheduler(int max_size, int max_stack_size);

    int schedule();

    int preempt();

    int block(int tid);

    int resume(int tid);

    int spawn(int tid, thread_entry_point * entry_point);

    int terminate(int tid);

    ~Scheduler();

    void remove_from_sleepVec(int tid);
};

/**
 * this function changes the status of a Running thread to Ready
 */


#endif //UTHREADS_H_SCHEDULER_H
