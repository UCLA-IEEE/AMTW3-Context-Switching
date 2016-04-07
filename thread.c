/**
 * @author Kevin Balke
 * @brief Defines the threading functionality of DankOS. Bracketed references
 * refer to the page or section of the datasheet corresponding to their tag.
 * Associations are given below:
 *
 * [PD: X] => Part Datasheet page/section X:
 *      http://www.ti.com/lit/ds/symlink/tm4c123gh6pm.pdf
 */

#include "thread.h"

#include <stdlib.h>
#include <string.h>

thread_t thread_table[MAX_THREADS] __attribute__((aligned(0x4)));

/**
 * Invalid index into the thread table
 */
#define TT_IDX_INVALID (MAX_THREADS)

thread_t* thread_current;
tid_t tid_counter;

/*
 * Needs to be aligned on the largest possible boundary, so that arbitrary data
 * can be pushed on to a stack starting at these addresses.
 */
__attribute__((aligned))
uint8_t thread_mem[MAX_THREADS][THREAD_MEM_SIZE];

/**
 * @brief Searches the thread table for an entry with a matching pid. Return the
 * entry if it's found, otherwise return NULL.
 */
thread_t* tt_entry_for_tid(tid_t id)
{
    int i;
    for(i = 0; i < MAX_THREADS; i++)
    {
        if(thread_table[i].id == id && thread_table[i].state != T_EMPTY)
        {
            return &thread_table[i];
        }
    }

    return NULL;
}

/**
 * @brief Finds the position of a thread in the thread table from a pointer to
 * that table entry.
 *
 * @param thread The thread to get the index of.
 * @return The index of the given thread; if the thread is not in the table,
 * TT_IDX_INVALID is returned.
 */
uint32_t thread_pos(const thread_t* thread)
{
    if(!thread)
        return MAX_THREADS;

    intptr_t pos = (((intptr_t)thread) - ((intptr_t)thread_table))/
                    sizeof(thread_t);

    if(pos >= 0 && pos < MAX_THREADS)
        return (uint32_t)pos;

    return TT_IDX_INVALID;
}

/**
 * @brief Finds the first free slot in the thread table.
 *
 * @return The index of the first free slot in the thread table. If there is no
 * free slot, TT_IDX_INVALID is returned.
 */
static uint32_t thread_first_empty(void)
{
    uint32_t ret = 0;
    do
    {
        if(thread_table[ret].state == T_EMPTY)
            return ret;
    } while((++ret) < MAX_THREADS);

    return TT_IDX_INVALID;
}

/**
 * @brief Returns a fresh thread ID.
 *
 * @return A fresh thread ID.
 */
static tid_t thread_fresh_tid(void)
{
    /*
     * tid_counter increments for every process spawned, but it cannot be zero
     * (it is allowed to roll over by itself).
     */
    ++tid_counter;
    if(tid_counter == 0)
        ++tid_counter;

    return tid_counter;
}

/**
 * @brief Checks if a given thread exists in the thread table.
 *
 * @param thread The thread to check.
 * @return true if the thread is valid; false otherwise.
 */
bool thread_in_table(const thread_t* thread)
{
    return (thread_pos(thread) != MAX_THREADS);
}

static void zero_thread(thread_t* thread)
{
    if(!thread_in_table(thread))
        return;

    thread->id = 0;
    thread->state = T_EMPTY;
    thread->scnt = 0;
    thread->waitstat = WAITSTATUS_NONE;

    // Zero-initialize registers and memory.
    memset(&thread->regs, 0, sizeof(registers_t));
    memset(thread_mem[thread_pos(thread)], 0, THREAD_MEM_SIZE);
}

/**
 * @brief Initializes all threads to empty; zero-initializes their memory and
 * registers.
 */
void thread_init(void)
{
    tid_counter = 0;
    int i, j;
    for(i = 0; i < MAX_THREADS; i++)
    {
        zero_thread(thread_table + i);
    }
}

/**
 * @brief Spawns a new thread with the given entry point and argument.
 *
 * @param entry The entry point for the thread. Accepts a void* argument and
 * returns an integer status.
 * @param arg The argument to pass to the thread when it is run.
 * @return The thread ID of the spawned thread.
 */
tid_t thread_spawn(const int (*entry)(void*), const void* arg)
{
    int i;
    thread_t* new_thread;

    // If there are no free thread spots, return invalid
    if((i = thread_first_empty()) == MAX_THREADS)
        return 0;

    new_thread = thread_table + i;

    /*
     * Zero out whatever might have been left by the last thread occupying this
     * slot.
     */
    zero_thread(new_thread);

    // Mark the thread runnable.
    new_thread->state = T_RUNNABLE;

    // Assign the tid.
    new_thread->id = thread_fresh_tid();

    // Clear the registers struct to all 0's.
    memset(&thread_table[i].regs, 0, sizeof(registers_t));

    /*
     * Set the new thread's program counter to the entry point, its R0 to the
     * argument, and its stack pointer to the beginning of the thread memory
     * entry allocated for it.
     */
    new_thread->regs.PC = (uint32_t)entry;
    new_thread->regs.R0 = (uint32_t)arg;
    new_thread->regs.SP = (uint32_t)&thread_mem[i+1];

    // Ensure that thumb state is enabled. [PD: 84]
    new_thread->regs.PSR = 0x01000000;

    return new_thread->id;
}

/**
 * @brief Kills a thread.
 *
 * @param thread The thread to kill.
 * @return true on success, false otherwise.
 */
bool thread_kill(thread_t* thread)
{
    if(!thread_in_table(thread))
        return false;

    thread->state = T_ZOMBIE;
    return true;
}

/**
 * @brief Kills a thread.
 *
 * @param tid The ID of the thread to kill.
 * @return true on success, false otherwise.
 */
bool thread_kill2(tid_t tid)
{
    thread_t* thread;

    if(!(thread = tt_entry_for_tid(tid)))
        return false;

    thread->state = T_ZOMBIE;
    return true;
}

/**
 * @brief Copy a thread. Copies the registers, thread state, and memory from one
 * thread slot to another.
 *
 * @param dest The thread table entry to copy into.
 * @param src The thread table entry to copy from.
 * @return bool true on success, false otherwise.
 */
bool thread_copy(thread_t* dest, const thread_t* src)
{
    if(!thread_in_table(dest) || !thread_in_table(src) ||
       dest->state != T_EMPTY || src->state == T_EMPTY)
        return false;

    // Get the table entry locations for the destination and source
    uint32_t d_index = thread_pos(dest), s_index = thread_pos(src);

    memcpy(dest, src, sizeof(thread_t));
    memcpy(thread_mem[d_index], thread_mem[s_index], THREAD_MEM_SIZE);

    thread_table[d_index].id = thread_fresh_tid();

    return true;
}

/*
 * @brief A shorthand for forking when the forked thread is not needed.
 */
bool thread_fork(const thread_t* thread)
{
    return thread_fork2(thread, (thread_t**)0);
}

/**
 * @brief Fork the calling thread. Return a pointer to the thread table entry
 * for the forked thread through rt.
 *
 * @param thread A pointer to the thread table entry for the thread to be
 * forked.
 * @outparam rt A pointer to the pointer to be set to the entry in the thread
 * table for the newly created thread.
 */
bool thread_fork2(const thread_t* thread, thread_t** rt)
{
    // If we got an invalid thread to fork, we can't do anything here. Fail out.
    if(!thread_in_table(thread))
        return false;

    // Find a free slot
    uint32_t destpos = thread_first_empty();

    // Couldn't find a free slot. Fail out.
    if(destpos == MAX_THREADS)
        return false;

    // Copy the thread, giving it a new tid; pass back the return status
    // and the thread table entry (if the outparam is a valid pointer).
    if(thread_copy(&thread_table[destpos], thread))
    {
        if(rt)
            (*rt) = &thread_table[destpos];
        return true;
    }

    return false;
}

/*
 * @brief Finds all threads waiting on the exiting thread, thread. For each
 * such thread, wake it up, and pass it the return status of the exiting thread.
 *
 * @param thread The exiting thread upon which other threads are waiting.
 */
void thread_notify_waiting(const thread_t* thread)
{
    // If we got an invalid thread, we can't do anything here. Return.
    if(!thread_in_table(thread))
        return;

    int i;

    for(i = 0; i < MAX_THREADS; i++)
    {
        /*
         * If this thread is blocked, waiting on another thread, and the thread
         * it is waiting on is the given thread, then we can wake it up and
         * pass it the return status.
         */
        if((thread_table[i].state == T_BLOCKED) &&
           (thread_table[i].waitstat == WAITSTATUS_THREAD) &&
           (((tid_t)thread_table[i].regs.R1) == thread->id))
        {
            thread_table[i].regs.R0 = thread->regs.R1;
            thread_table[i].state = T_RUNNABLE;
        }
    }
}
