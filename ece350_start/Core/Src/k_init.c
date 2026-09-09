#include "k_init.h"
#include "k_mem.h"
#include "common.h"
#include "k_task.h" // task_init_ptr, used to build the null task's first frame

kernel_state_t kernel_state = UNINITIALIZED;
TCB tcb_array[MAX_TASKS];
static U32 null_stack[STACK_SIZE / 4] __attribute__((aligned(8)));
ScheduleQueue Scheduler;
volatile  TCB* p_current_task;
//null task routine to do nothing
void null_task(void *args) {
	while (1) {
		__WFI();
	}
}

void osKernelInit(void) {
	// Guard against multiple calls
	if (kernel_state != UNINITIALIZED) {
		return;
	}

	// Initialize all TCBs to DORMANT
	for (int i = 0; i < MAX_TASKS; i++) {
		tcb_array[i].tid    = i;
		tcb_array[i].state  = DORMANT;
		tcb_array[i].ptask  = NULL;
		tcb_array[i].stack_high = 0;
		tcb_array[i].stack_size = 0;
        tcb_array[i].used = FREE_TID;
		tcb_array[i].p_next = NULL;
		tcb_array[i].deadline         = DEFAULT_DEADLINE;
		tcb_array[i].initial_deadline = DEFAULT_DEADLINE;
		tcb_array[i].time_remaining   = DEFAULT_DEADLINE;
		tcb_array[i].sleep_remaining  = 0;
	}

	p_current_task = NULL;

	tcb_array[TID_NULL].tid        = TID_NULL;
	tcb_array[TID_NULL].ptask      = null_task;
	tcb_array[TID_NULL].stack_size = STACK_SIZE;
	tcb_array[TID_NULL].stack_high = (U32)(null_stack + (STACK_SIZE / 4));
	tcb_array[TID_NULL].state      = READY;
	tcb_array[TID_NULL].used       = 1;
	// The null task must only run when nothing else is READY, so it gets an
	// effectively-infinite deadline and never wins an EDF comparison.
	tcb_array[TID_NULL].deadline         = NULL_TASK_DEADLINE;
	tcb_array[TID_NULL].initial_deadline = NULL_TASK_DEADLINE;
	tcb_array[TID_NULL].time_remaining   = NULL_TASK_DEADLINE;
	tcb_array[TID_NULL].sleep_remaining  = 0;
	// CRITICAL: build the null task's initial exception frame. The first time
	// every real task is SLEEPING, PendSV switches TO the null task by loading
	// its p_stack and unstacking a full frame from it -- without this frame the
	// very first "all tasks asleep" moment loads a garbage PSP and hard-faults
	// (symptom: osSleep/osPeriodYield tests print one iteration, then freeze).
	// This could never trigger before lab 3: with only osYield/osTaskExit, the
	// ready queue was never empty while tasks existed, so null never ran.
	tcb_array[TID_NULL].p_stack = (U32*)task_init_ptr(
		(uint32_t*)tcb_array[TID_NULL].stack_high, null_task);

	// Exception priorities: PendSV must be the LOWEST priority so a context
	// switch only runs as a "tail" after every other handler (SysTick, SVC)
	// has returned. This is what prevents the yield-vs-timeout race the manual
	// warns about -- PendSV can never interrupt an in-progress SysTick/SVC.
	NVIC_SetPriority(PendSV_IRQn,  0xFF); // lowest
	NVIC_SetPriority(SysTick_IRQn, 0xFE); // just above PendSV

	schedule_init(&Scheduler);
	k_mem_init();
	kernel_state = INITIALIZED;
}


// Kernel-side implementation: checks/mutates kernel_state and tcb_array, and
// performs the first task launch. Must only run inside the SVC handler. On
// success it never returns -- it jumps directly into the first task.
int k_osKernelStart(void){
	if (kernel_state == UNINITIALIZED || kernel_state == RUNNING) {
		return RTX_ERR;
	}

	// Reset every task's timers so timing starts fresh at kernel start: each
	// task's deadline countdown is reloaded and no stale sleep values remain.
	for (int i = 0; i < MAX_TASKS; i++) {
		if (tcb_array[i].used == USED_TID) {
			tcb_array[i].time_remaining  = tcb_array[i].deadline;
			tcb_array[i].sleep_remaining = 0;
		}
	}

	get_next_task(&Scheduler);
	p_current_task->state = RUNNING;
	p_current_task->time_remaining = p_current_task->deadline;

	// Reset the SysTick counter so timeslice accounting starts from zero the
	// instant the first task runs. We flip kernel_state to RUNNING last, so the
	// SysTick_Handler ignores ticks until we are actually about to launch --
	// otherwise a stray tick could pend PendSV before any task exists (hardfault).
	// Interrupts are re-enabled in case they were disabled at any point before.
	SysTick->VAL = 0;
	__enable_irq();
	kernel_state = RUNNING;

	__set_PSP((U32)p_current_task->p_stack);
	__set_CONTROL(2);
	__ISB();
	init_First_Task();

	// Unreachable, but satisfies compiler
	return RTX_OK;
}

int osKernelStart(void){
	int result;
	__asm volatile (
		"svc %1          \n" // %1 is the 1st input: OSKERNELSTART_SVC
		"mov %0, r0      \n" // %0 is the output: result
		: "=r" (result)
		: "i" (OSKERNELSTART_SVC)
		: "r0", "memory"
	);
	return result;
}

void schedule_push(ScheduleQueue* List, TCB* Node){
		if (List == NULL || Node == NULL){
			return;
		}
		if (List->p_head == NULL){
			List->p_head = Node;
			List->p_tail = Node;
			Node->p_next = NULL;
			List->count++;
		} else {
			List->p_tail->p_next = Node;
			List->p_tail = Node;
			Node->p_next = NULL;
			List->count++;
		}
}
void schedule_pop(ScheduleQueue* List, TCB* Node){
	TCB* p_curr = List->p_head;
	TCB* p_prev = 0;
	if (List == NULL || p_curr == NULL || Node == NULL){
		return;
	} else if (p_curr == Node){
		List->p_head = p_curr->p_next;
		if (List->p_head == NULL) {
            List->p_tail = NULL;
        }   
        Node->p_next = NULL;
		List->count--;
		return;
	}
	while (p_curr != NULL){
		p_prev = p_curr;
		p_curr = p_curr->p_next;
		if (p_curr == Node){
			p_prev->p_next = p_curr->p_next;
			if (p_curr == List->p_tail) {
				List->p_tail = p_prev;
			}
			Node->p_next = NULL;
			List->count--;
			return;
		}
	}
	return;
}
void schedule_init(ScheduleQueue* List){
	List->p_head = NULL;
	List->p_tail = NULL;
	List->count = 0;
}
// EDF scheduler (Ch.6): fetch the READY task with the EARLIEST deadline, i.e.
// the smallest time_remaining (every task's countdown ticks in SysTick, so
// comparing countdowns compares absolute deadlines). Ties are broken by
// choosing the lowest TID. The chosen task is removed from the ready queue.
// If no task is READY, the null task runs.
TCB* get_next_task(ScheduleQueue* List){
	if (List->p_head == NULL){	// If queue empty, run null task
		p_current_task = &tcb_array[TID_NULL];
		return &tcb_array[TID_NULL];
	}
	TCB* best = List->p_head;
	for (TCB* it = List->p_head->p_next; it != NULL; it = it->p_next){
		if (it->time_remaining < best->time_remaining ||
		    (it->time_remaining == best->time_remaining && it->tid < best->tid)){
			best = it;
		}
	}
	schedule_pop(List, best);
	p_current_task = best;
	return best;
}
