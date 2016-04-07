/*
 * syscalls.h
 *
 *  Created on: Mar 17, 2015
 *      Author: Kevin
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include "thread.h"

#include <stdint.h>

extern tid_t sys_get_tid();
extern void sys_yield();
extern int32_t sys_wait(tid_t tid);
extern bool sys_kill(tid_t tid);
extern bool sys_lock(lock_t* l);
extern void sys_unlock(lock_t* l);
extern uint32_t sys_sleep(uint32_t ms);
extern tid_t sys_fork();
extern tid_t sys_spawn(int (*entry)(void*), void* arg);

__attribute__((noreturn()))
extern void sys_exit(int status);

__attribute__((noreturn()))
extern void sys_reset();

extern void _exit(int status);

#endif /* SYSCALLS_H_ */
