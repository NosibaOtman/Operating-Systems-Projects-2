//
// Created by yousefak on 4/24/23.
//
/*
 * sigsetjmp/siglongjmp demo program.
 * Hebrew University OS course.
 * Author: OS, os@cs.huji.ac.il
 */

#include <csetjmp>
#include <csignal>
#include "jmp.h"


#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
        "rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
            : "=g" (ret)
            : "0" (addr));
    return ret;
}


#endif


typedef void (*thread_entry_point)(void);

int current_thread = 0;


void jump_to_thread(int tid, sigjmp_buf* env)
{
    current_thread = tid;
     siglongjmp(env[tid], 1);
}

/**
 * @brief Saves the current thread state, and jumps to the other thread.
 */
void yield(int tid, sigjmp_buf* env)
{
//    mask_alarm2();
    int ret_val = sigsetjmp(env[current_thread], 1);
    bool did_just_save_bookmark = ret_val == 0;
//    bool did_jump_from_another_thread = ret_val != 0;
    if (did_just_save_bookmark)
    {
        jump_to_thread(tid, env);
    }
}



void setup_thread(int tid, char *stack, thread_entry_point entry_point, sigjmp_buf* env, int stack_size)
{
    // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
    // siglongjmp to jump into the thread.
    address_t sp = (address_t) stack + stack_size - sizeof(address_t);
    address_t pc = (address_t) entry_point;
    sigsetjmp(env[tid], 1);
    (env[tid]->__jmpbuf)[JB_SP] = translate_address(sp);
    (env[tid]->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env[tid]->__saved_mask);
}



