/*
 * kernel.c
 *
 *  Created on: Mar 16, 2015
 *      Author: Kevin
 */

#include "os_utils.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Kernel stack. Used while in kernel space.
 */
uint8_t kernel_stack[KERNEL_STACKSIZE] __attribute((aligned(8)));
uint32_t kernel_stack_top;

/*
 * Scheduler timekeeping.
 */
tsleep_t systime_ms;
tsleep_t next_to_run_ms;

/*
 * Forward declarations
 */
void kernel_schedule();
void kernel_run(thread_t* thread);
void kernel_panic();
inline void kernel_assert(bool cond);
void kernel_set_scheduler_freq(uint32_t freq);
__attribute__((noreturn))
extern void kernel_exit(void);

/**
 * @brief Initializes the kernel.
 *
 * This function calls thread_init() first, which initializes the thread table.
 * Then, it initializes the 0th thread to runnable, and sets it as the current
 * thread. The semantics here is that the caller of kernel_init() is now the 0th
 * thread in the system. After this, the kernel sets the system time counter
 * to 0 and sets the next thread invocation to the maximum possible integer
 * (effectively never). This is used to inhibit the scheduler when threads are
 * sleeping.
 */
void kernel_init(void* current_stack_top)
{
    kernel_stack_top = (uint32_t)kernel_stack + sizeof(kernel_stack);
    thread_init();

    // Relocate the caller's stack into the thread 0 stack slot. The caller
    // should not have created any pointers into their stack, otherwise this
    // will result in catastrophe.
    asm volatile(
        "mov r3,%0\r\n"
        "mov r0,%1\r\n"
        "mov r1,sp\r\n"
        "sub r2,r3,r1\r\n"
        "sub r0,r2\r\n"
        "push {r0}\r\n"
        "bl memcpy\r\n"
        "pop {r0}\r\n"
        "dsb\r\n"
        "isb\r\n"
        "mov sp,r0\r\n"
     : : "r" (current_stack_top), "r" (&(thread_mem[0][THREAD_MEM_SIZE])) :
     "memory", "0", "1", "2", "3" );

    thread_table[0].state = T_RUNNABLE;
    thread_table[0].id = 0;
    thread_current = &thread_table[0];

    systime_ms = 0;
    next_to_run_ms = UINT32_MAX;

    int i;
    for(i = 0; i < MAX_THREADS; i++)
    {
        // Assert the initialization conditions; this will catch some bad
        // behavior after the kernel starts
        kernel_assert(thread_pos(&thread_table[i]) == i);
        kernel_assert(thread_in_table(&thread_table[i]));
    }

    kernel_set_scheduler_freq(KERNEL_SCHEDULER_IRQ_FREQ);
}

/**
 * @brief Disables all interrupts and traps the processor in a loop so that
 * a debugger can inspect its state.
 */
void kernel_panic()
{
    // Disable interrupts
    while(1)
    {
        ;
    }
}

inline void kernel_assert(bool cond)
{
    if(!cond)
        kernel_panic();
}

/**
 * @brief Schedule a different thread to run. This is invoked from kernel space.
 *
 * This is a simple round-robin scheduler. It checks all threads for one that
 * is runnable, and runs the first such one.
 */
__attribute__((noreturn))
void kernel_schedule()
{
    uint32_t i = thread_pos(thread_current), original_i;

    // If the current thread has an invalid index, then something went wrong...
    if(i >= MAX_THREADS)
        kernel_panic();

    i++;
    if (i == MAX_THREADS)
        i = 0;

    original_i = i;

    while (1)
    {
        if (thread_table[i].state == T_RUNNABLE)
        {
            kernel_run(&thread_table[i]);
        }

        i++;
        if(i == MAX_THREADS)
            i = 0;
        if(i == original_i)
            break;
    }

    /*
     * No runnable threads found. This can occur if all threads are sleeping.
     * There are more graceful ways to handle this situation, e.g., waiting for
     * a thread to be marked runnable. This is left as an exercise for the
     * reader.
     */
    kernel_panic();

    // Convince GCC that this function does not return.
    while(1)
        ;
}

__attribute__((noreturn))
void kernel_run(thread_t* thread)
{
    thread_current = thread;

    kernel_exit();

    // Convince GCC that this function does not return.
    while(1)
        ;
}

__attribute__((noreturn))
void kernel_handle_syscall()
{
    thread_t* child_thread;
    switch (thread_current->regs.R0)
    {
    // Get the thread ID of the calling process
    case SYSCALL_GET_TID:
        thread_current->regs.R0 = thread_current->id;
        kernel_run(thread_current);
        break;

    case SYSCALL_EXIT:
        thread_notify_waiting(thread_current);
        thread_kill(thread_current);

        kernel_schedule();
        break;

    case SYSCALL_YIELD:
        kernel_schedule();
        break;

    case SYSCALL_LOCK:
        // If the lock is already taken, return 0 and resume the spinlock
        if (*((lock_t*) thread_current->regs.R1))
        {
            thread_current->regs.R0 = false;
        }
        else
        {
            // Otherwise, lock it
            *((lock_t*) thread_current->regs.R1) = LOCK_LOCKED;
            thread_current->regs.R0 = true;
        }
        kernel_run(thread_current);
        break;

    case SYSCALL_UNLOCK:
        // Release the lock
        *((lock_t*) thread_current->regs.R1) = LOCK_UNLOCKED;
        kernel_schedule();
        break;

    case SYSCALL_FORK:
        // Find a free thread slot and clone the thread into it
        if (thread_fork2(thread_current, &child_thread))
        {
            // Set the correct return values
            child_thread->regs.R0 = 0;
            thread_current->regs.R0 = child_thread->id;
        }
        else
        {
            thread_current->regs.R0 = 0;
        }
        kernel_run(thread_current);
        break;

    case SYSCALL_SLEEP:
        if (thread_current->regs.R1 > 0)
        {
            thread_current->scnt = thread_current->regs.R0 =
                    (thread_current->regs.R1 / SYSTIME_CYCLES_PER_MS);

            thread_current->scnt += systime_ms;

            if((thread_current->scnt - systime_ms) < (next_to_run_ms - systime_ms))
                next_to_run_ms = thread_current->scnt;

            thread_current->state = T_SLEEPING;
            kernel_schedule();
        }
        else
        {
            thread_current->regs.R0 = 0;
            kernel_run(thread_current);
        }
        break;

    case SYSCALL_KILL:
        child_thread = tt_entry_for_tid((tid_t)thread_current->regs.R1);
        if(child_thread)
        {
            thread_notify_waiting(child_thread);
            thread_current->regs.R0 = thread_kill(child_thread);
        }
        else
        {
            thread_current->regs.R0 = 0;
        }
        kernel_run(thread_current);

    case SYSCALL_RESET:
        dptr(0xE000ED0C) = 0x05FA0004;
        break;

    case SYSCALL_SPAWN:
        thread_current->regs.R0 = (uint32_t) thread_spawn(
                (const int(*)(void*)) thread_current->regs.R1,
                (const void*) thread_current->regs.R2);

        kernel_schedule();
        break;

    case SYSCALL_WAIT:
        if(tt_entry_for_tid(thread_current->regs.R1))
        {
        thread_current->state = T_BLOCKED;
        thread_current->waitstat = WAITSTATUS_THREAD;

        // The thread id that thread_current is waiting on
        // is stored in R1
        kernel_schedule();
        }
        kernel_run(thread_current);
        break;
    }

    // Unknown system call
    kernel_panic();

    // Convince GCC that this function never returns.
    while(1)
        ;
}

void kernel_tick_counter(void)
{
    systime_ms++;

    if(systime_ms != next_to_run_ms)
        return;

    next_to_run_ms = UINT32_MAX;

    int i;
    for (i = 0; i < MAX_THREADS; i++)
    {
        if (thread_table[i].state == T_SLEEPING)
        {
            // If this tick is the thread's wakeup time
            if (systime_ms == thread_table[i].scnt)
            {
                // Wake it up
                thread_table[i].state = T_RUNNABLE;
            }
            // If this is not the thread's wakeup time
            // sort wakeup times to determine next runtime
            else
            {
                // Normalize the time (modulo UINT32_MAX + 1)
                tsleep_t normtime = thread_table[i].scnt - systime_ms;
                if(normtime < next_to_run_ms)
                {
                    next_to_run_ms = normtime;
                }
            }
        }
    }

    // Denormalize (relative to current systime_ms)
    next_to_run_ms += systime_ms;
}

/**
 * @brief Gets the system clock frequency.
 *
 * @return The system clock frequency, in Hertz.
 */
uint32_t kernel_get_system_freq(void)
{
    return 80000000ul;
}

/**
 * @brief Set the kernel scheduler interrupt (SysTick) frequency.
 *
 * @param freq The desired scheduler frequency, in Hertz.
 */
void kernel_set_scheduler_freq(uint32_t freq)
{
#if (!KERNEL_PREEMPTION)
    return;
#endif

    // Set the SysTick current value register to 0.
    dptr(0xE000E018) = 0;

    /*
     * Set the SysTick reload value to the system clock frequency divided by the
     * desired frequency, minus 1. We subtract 1 because the cycles counted by
     * the timer includes 0 and the reload value.
     */
    dptr(0xE000E014) = ((kernel_get_system_freq() / freq) - 1);

    /*
     * Set the SysTick source to the system clock, enable the interrupt, and
     * start counting.
     */
    dptr(0xE000E010) |= 0x00000007;
}

#ifdef __cplusplus
}
#endif
