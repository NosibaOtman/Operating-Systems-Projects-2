//
// Created by yousefak on 4/24/23.
//

#ifndef SCHEDULER_CPP_JMP_H
#define SCHEDULER_CPP_JMP_H

#include <setjmp.h>
#include "scheduler.h"

void setup_thread(int tid, char *stack, thread_entry_point entry_point, sigjmp_buf* env, int stack_size);

/**
 * @brief Saves the current thread state, and jumps to the other thread.
 */
void yield(int tid, sigjmp_buf* env);


void jump_to_thread(int tid, sigjmp_buf* env);



#endif //SCHEDULER_CPP_JMP_H

