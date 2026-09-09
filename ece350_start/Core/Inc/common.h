/*
 * common.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: If you feel that there are common
 *      C functions corresponding to this
 *      header, then any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_
#include <stdio.h>
#define TID_NULL 0 //predefined Task ID for the NULL task
#ifndef NULL
#define NULL 0
#endif

#define MAX_TASKS 16 //maximum number of tasks in the system
#define STACK_SIZE 0x200 //min. size of each task’s stack
#define THREAD_STACK_SIZE 0x400
#define DORMANT 0 //state of terminated task
#define READY 1 //state of task that can be scheduled but is not running
#define RUNNING 2 //state of running task
#define SLEEPING 3 //state of task removed from scheduling until its sleep timer expires
#define DEFAULT_DEADLINE 5 //default task deadline/timeslice in ms (see Ch.6)
#define NULL_TASK_DEADLINE 0x7FFFFFFF //null task never competes with real tasks in EDF
#define USED_TID 1
#define FREE_TID 0
#define UNINITIALIZED 0
#define INITIALIZED 1
#define RTX_ERR -1
#define RTX_OK 0
//SVC ID COUNT
#define OSTASKEXIT_SVC 1
#define OSCREATETASK_SVC 2
#define OSGETTID_SVC 3
#define OSTASKINFO_SVC 4
#define OSYIELD_SVC 5
#define MALLOC_SVC 6
#define DALLOC_SVC 7
#define COUNT_EXT_SVC 8
#define OSSLEEP_SVC 9
#define OSPERIODYIELD_SVC 10
#define OSSETDEADLINE_SVC 11
#define OSCREATEDEADLINETASK_SVC 12
#define OSKERNELSTART_SVC 17

typedef unsigned int U32;
typedef unsigned short U16;
typedef char U8;
typedef unsigned int task_t;
typedef char  kernel_state_t;

typedef struct task_control_block{
    void (*ptask)(void* args); //entry address
	  U32 stack_high; //starting address of stack (high address)
	  task_t tid; //task ID
	  U8 state; //task's state
	  U16 stack_size; //stack size. Must be a multiple of 8
      U8 used; //represent if current tid is used.
	  struct task_control_block* p_next;
      U32* p_stack;
      U32 deadline;         //current deadline in ms; scheduler picks the smallest (EDF priority)
      U32 initial_deadline; //deadline set at creation; a task's deadline resets to this on wake
      U32 time_remaining;   //ms left in the current timeslice; counts down while RUNNING
      U32 sleep_remaining;  //ms left until a SLEEPING task wakes
 }TCB;

typedef struct scheduler {
	TCB* p_head;
	TCB* p_tail;
	U32 count;
 } ScheduleQueue;

void schedule_push(ScheduleQueue* List, TCB* Node);
void schedule_pop(ScheduleQueue* List, TCB* Node);
void schedule_init(ScheduleQueue* List);
TCB* get_next_task(ScheduleQueue* List);
extern void init_First_Task(void );

extern TCB tcb_array[MAX_TASKS];
extern kernel_state_t kernel_state;
extern volatile TCB* p_current_task;
extern volatile TCB* p_next_task;
extern ScheduleQueue Scheduler;


#include "k_init.h"
#include "k_mem.h"
#endif /* INC_COMMON_H_ */
