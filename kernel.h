/*
 * kernel.h
 *
 *  Created on: Mar 16, 2015
 *      Author: Kevin
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include "thread.h"
#include "syscall_numbers.h"

#define KERNEL_PREEMPTION (1)
#define KERNEL_SCHEDULER_IRQ_FREQ (1000)
#define SYSTIME_CYCLES_PER_MS (1000/KERNEL_SCHEDULER_IRQ_FREQ)
#define KERNEL_STACKSIZE (1024)

extern uint8_t kernel_stack[KERNEL_STACKSIZE] __attribute((aligned(8)));

void kernel_init(void* current_stack_top);

#endif /* KERNEL_H_ */
