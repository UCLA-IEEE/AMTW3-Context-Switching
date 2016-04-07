.thumb
.syntax unified

#include "syscall_numbers.h"

.global kernel_entry
.global kernel_exit

.global sys_get_tid
.global sys_yield
.global sys_wait
.global sys_kill
.global sys_lock
.global sys_unlock
.global sys_sleep
.global sys_fork
.global sys_spawn
.global sys_exit
.global sys_reset

// Refer to page 110 of the datasheet

kernel_exit:
    // Pop the software-saved registers out of the struct
    // Need a pointer to R4 in the struct
    ldr r1, =thread_current
    ldr r1, [r1]

    // r1 now points to the start of the same thread_t struct as thread_current

    // YOUR CODE GOES HERE; you need to restore the registers onto the stack
    // for thread_current from thread_current->regs; then, pop all of the
    // software-saved registers. Once that's done, pop the stack pointer. What
    // will happen next is a cache and instruction pipeline flush, followed by
    // exceptional return, and (hopefully) a different thread running!

    // Flush cache and instruction pipeline
    dsb
    isb

    // Perform exceptional return; this will pop r0-r3, r12, lr, pc, and psr in
    // hardware
    mov lr, #0xFFFFFFF9
    bx lr


kernel_entry:
    // At this point, the exception frame has been pushed, and sp points to the
    // end of that frame. You need to push the remaining registers, and then
    // copy them into the thread_current->regs struct. After that, the kernel
    // stack will be loaded into sp, and the remaining kernel_entry code will
    // determine what to do next based upon what exception triggered entry into
    // the kernel.

    // Store the stack pointer just above the exception frame
    add r0, sp, #32

    // Get the address of the pointer to the current thread struct
    ldr r1,=thread_current

    // Get the address of the current thread struct
    ldr r1, [r1]

    // YOUR CODE GOES HERE

    ldr r0,=kernel_stack_top
    ldr sp,[r0]

    mrs r12, xpsr
    and r12, #0x1F
    //lsl r12, #23
    //lsr r12, #23
    subs r12, #15
    bne _systick_dont_jump
    bl kernel_tick_counter
    bl kernel_schedule
_systick_dont_jump:

    bl kernel_handle_syscall

/*
 * All of these system calls conform to the ARM C calling convention (arguments
 * passed by registers, lr loaded with return address). The DankOS kernel
 * expects to see the system call number in r0, so we have to move the rest of
 * the arguments over (r0 -> r1, r1 -> r2, etc...). This is done in reverse
 * order so as not to propagate r0 to all of the arguments.
 */

/*
 * extern tid_t sys_get_tid();
 */
sys_get_tid:
    ldr r0, =SYSCALL_GET_TID
    svc #0x80
    bx lr

/*
 * extern void sys_yield();
 */
sys_yield:
    ldr r0, =SYSCALL_YIELD
    svc #0x80
    bx lr

/*
 * extern int32_t sys_wait(tid_t tid);
 */
sys_wait:
    push {r1}
    mov r1, r0
    ldr r0, =SYSCALL_WAIT
    svc #0x80
    pop {r1}
    bx lr

/*
 * extern bool sys_kill(tid_t tid);
 */
sys_kill:
    push {r1}
    mov r1, r0
    ldr r0, =SYSCALL_KILL
    svc #0x80
    pop {r1}
    bx lr

/*
 * extern bool sys_lock(lock_t* lock);
 */
sys_lock:
    push {r1}
    mov r1, r0
    ldr r0, =SYSCALL_LOCK
    svc #0x80
    pop {r1}
    bx lr

/*
 * extern void sys_unlock(lock_t* lock);
 */
sys_unlock:
    push {r1}
    mov r1, r0
    ldr r0, =SYSCALL_UNLOCK
    svc #0x80
    pop {r1}
    bx lr

/*
 * extern uint32_t sys_sleep(uint32_t ms);
 */
sys_sleep:
    push {r1}
    mov r1, r0
    ldr r0, =SYSCALL_SLEEP
    svc #0x80
    pop {r1}
    bx lr

/*
 * extern tid_t sys_fork();
 */
sys_fork:
    ldr r0, =SYSCALL_FORK
    svc #0x80
    bx lr

/*
 * extern tid_t sys_spawn(int (*entry)(void*), void* arg);
 */
sys_spawn:
    push {r2}
    mov r2, r1
    mov r1, r0
    ldr r0, =SYSCALL_SPAWN
    svc #0x80
    pop {r2}
    bx lr

/*
 * _exit and sys_exit have the same calling convention, so why not combine them?
 *
 * extern void sys_exit(int status);
 */
_exit:
sys_exit:
    mov r1, r0
    ldr r0, =SYSCALL_EXIT
    svc #0x80

/*
 * extern void sys_reset();
 */
sys_reset:
    ldr r0, =SYSCALL_RESET
    svc #0x80
