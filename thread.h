/*
 * thread.h
 *
 *  Created on: Mar 16, 2015
 *      Author: Kevin
 */

/**
 * Thread ID 0 is reserved
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_THREADS (12)
#define LOG2_THREAD_MEM_SIZE (10)
#define THREAD_MEM_SIZE (1<<LOG2_THREAD_MEM_SIZE)

// Type for a thread ID
typedef uint32_t tid_t;

// Type for a thread sleep counter
typedef uint32_t tsleep_t;

// Type for a lock object
typedef enum
{
    LOCK_UNLOCKED = 0,
    LOCK_LOCKED = 1
} lock_t;

// Type for a thread state
typedef enum
{
	T_EMPTY,

	T_RUNNABLE, T_BLOCKED, T_ZOMBIE, T_SLEEPING
} tstate_t;

// Type for a registers store
typedef struct
{
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;

	uint32_t SP;

	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R12;
	uint32_t LR;
	uint32_t PC;
	uint32_t PSR;
}__attribute__((aligned(0x4))) registers_t;

#define THREAD_SAVED_REGISTERS_NUM (sizeof(registers_t)/sizeof(uint32_t))

// Type for a thread wait status
typedef enum
{
    // Not waiting on anything
    WAITSTATUS_NONE = 0,
    // Waiting on another thread
    WAITSTATUS_THREAD = 1
} twait_status_t;

typedef struct
{
	// Thread ID
	tid_t id;

	// Thread state
	tstate_t state;

	// Thread sleep counter
	tsleep_t scnt;

	// Thread registers
	registers_t regs;

    // Thread wait status
	twait_status_t waitstat;
} thread_t;

// Declare a global thread table, current thread index, and thread memory array.
extern thread_t thread_table[];
extern thread_t* thread_current;
extern uint8_t thread_mem[][THREAD_MEM_SIZE];

// Declare the thread manipulation functions.
bool thread_copy(thread_t* dest, const thread_t* src);
bool thread_fork(const thread_t* thread);
bool thread_fork2(const thread_t* thread, thread_t** rthread);
bool thread_kill(thread_t* thread);
bool thread_kill2(tid_t tid);
bool thread_in_table(const thread_t* thread);
thread_t* tt_entry_for_tid(tid_t id);
tid_t thread_spawn(const int (*entry)(void*), const void* arg);
uint32_t thread_pos(const thread_t* thread);
void thread_init(void);
void thread_notify_waiting(const thread_t* thread);

#endif /* THREAD_H_ */
