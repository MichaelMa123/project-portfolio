/*
 * k_task.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_K_TASK_H_
#define INC_K_TASK_H_
#include "stm32f4xx_hal.h"
#include "k_mem.h"
#include "common.h"
#include "k_task.h"
extern int task_count;
extern unsigned int current_running_task;

int osCreateTask(TCB* task);
task_t osFindTid();
task_t osGetTID(void);
int osTaskInfo(task_t TID, TCB* task_copy);
int osTaskExit(void);
void osYield(void);
// ---- Deliverable 3 additions (Ch.6: pre-emptive EDF multitasking) ----
void osSleep(int timeInMs);
void osPeriodYield(void);
int osSetDeadline(int deadline, task_t TID);
int osCreateDeadlineTask(int deadline, TCB* task);
uint32_t* task_init_ptr(uint32_t*, void (*)(void*));
void SVC_Handler_Main(unsigned int*);

// Kernel-side implementations: touch tcb_array/p_current_task directly.
// Must only be called from within the SVC handler (privileged kernel context).
task_t k_osGetTID(void);
int k_osTaskInfo(task_t TID, TCB* task_copy);
int k_osTaskExit(void);
int k_osCreateTask(TCB* task);
void k_osYield(void);
void k_osSleep(int timeInMs);
void k_osPeriodYield(void);
int k_osSetDeadline(int deadline, task_t TID);
int k_osCreateDeadlineTask(int deadline, TCB* task);

void PVC_osScheule(void);
#endif /* INC_K_TASK_H_ */
