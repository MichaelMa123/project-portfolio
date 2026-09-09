#include "k_task.h"

#include "common.h"
int task_count = 0;
extern void PendSV_Handler(void);
task_t osFindTid(){
    for(unsigned int i=0;i<MAX_TASKS;i++){
        if(tcb_array[i].used==FREE_TID){
            return i;
        }
    }
    return -1;
}

// Kernel-side implementation: reads p_current_task, must only run inside the SVC handler
task_t k_osGetTID(void)
{
    if (kernel_state == UNINITIALIZED || p_current_task == NULL){
        return 0;
    }
    return p_current_task->tid;
}

task_t osGetTID(void)
{
    task_t result;
    __asm volatile (
        "svc %1          \n" // %1 is the 1st input: OSGETTID_SVC
        "mov %0, r0      \n" // %0 is the output: result
        : "=r" (result)
        : "i" (OSGETTID_SVC)
        : "r0", "memory"
    );
    return result;
}



// Kernel-side implementation: reads/writes tcb_array and triggers the context
// switch. Must only run inside the SVC handler.
int k_osTaskExit(void)
{
    if (kernel_state == UNINITIALIZED || task_count <= 0 || task_count > MAX_TASKS) {
        return RTX_ERR;
    }

    task_t task_to_end = k_osGetTID();
    if (task_to_end >= MAX_TASKS) {
        return RTX_ERR;
    }

    uint32_t stack_base = tcb_array[task_to_end].stack_high - tcb_array[task_to_end].stack_size;
    k_mem_dealloc_SVC((void*)stack_base, task_to_end);

    tcb_array[task_to_end].ptask = NULL;
    tcb_array[task_to_end].stack_high = 0;
    tcb_array[task_to_end].stack_size = 0;
    tcb_array[task_to_end].state = DORMANT;
    tcb_array[task_to_end].used = FREE_TID;
    tcb_array[task_to_end].deadline         = DEFAULT_DEADLINE;
    tcb_array[task_to_end].initial_deadline = DEFAULT_DEADLINE;
    tcb_array[task_to_end].time_remaining   = DEFAULT_DEADLINE;
    tcb_array[task_to_end].sleep_remaining  = 0;
    task_count--;
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    return RTX_OK;
}

int osTaskExit(void)
{
    int result;
    __asm volatile (
        "svc %1          \n" // %1 is the 1st input: OSTASKEXIT_SVC
        "mov %0, r0      \n" // %0 is the output: result
        : "=r" (result)
        : "i" (OSTASKEXIT_SVC)
        : "r0", "memory"
    );
    return result;
}
uint32_t* task_init_ptr(uint32_t* stackptr, void (*ptask)(void* args)){
  *(--stackptr) = 0x01000000;
  *(--stackptr) = (uint32_t)ptask;
  *(--stackptr) = 0xFFFFFFFD;
  *(--stackptr) = 0x12121213;
  *(--stackptr) = 0x03030304;
  *(--stackptr) = 0x02020207;
  *(--stackptr) = 0x01010109;
  *(--stackptr) = 0x00000002;

  for(int i = 0; i < 8; i++){
      *(--stackptr) = 0x0A0A0A0A;
  }
  return stackptr;
}

/**
 * @brief Shared kernel-side task creation used by both osCreateTask (default
 * 5ms deadline) and osCreateDeadlineTask (custom deadline). Must only run
 * inside the SVC handler.
 *
 * The stack is allocated dynamically from the deliverable-2 heap, and the
 * OWNER of that allocation is the newly created task (so osTaskExit frees it).
 * If the caller is a running task and the new task has a sooner deadline, a
 * pre-emptive context switch to the new task is triggered.
 */
static int k_osCreateTaskCommon(TCB* task, U32 deadline_ms) {
    if (task == NULL || task->ptask == NULL || task->stack_size < STACK_SIZE
        || task_count >= MAX_TASKS || kernel_state == UNINITIALIZED
        || task->stack_size % 8 != 0) {
        return RTX_ERR;
    }

    task_t free_tid = osFindTid();
    if (free_tid == (task_t)-1) {
        return RTX_ERR;
    }

    // The public tests may skip the explicit k_mem_init() call (it is
    // #ifdef'd out in public test 1), so the kernel initializes the heap
    // on demand before the first stack allocation.
    if (!k_mem_initialized()) {
        k_mem_init();
    }
    uint32_t mem_requested = (uint32_t)task->stack_size;
    uint32_t* allocated_mem = (uint32_t*)k_mem_alloc_SVC(mem_requested, free_tid);
    if (allocated_mem == NULL) {
        return RTX_ERR;
    }

    tcb_array[free_tid].ptask = task->ptask;
    tcb_array[free_tid].tid = free_tid;
    tcb_array[free_tid].stack_high = (uint32_t)allocated_mem + task->stack_size;
    tcb_array[free_tid].stack_size = task->stack_size;
    tcb_array[free_tid].state = READY;
    tcb_array[free_tid].used = USED_TID;
    tcb_array[free_tid].deadline         = deadline_ms;
    tcb_array[free_tid].initial_deadline = deadline_ms;
    tcb_array[free_tid].time_remaining   = deadline_ms; // countdown to this task's deadline
    tcb_array[free_tid].sleep_remaining  = 0;
    tcb_array[free_tid].p_stack = (U32*)task_init_ptr((uint32_t*)tcb_array[free_tid].stack_high, tcb_array[free_tid].ptask);
    task_count++;
    schedule_push(&Scheduler, &tcb_array[free_tid]);

    // Copy the assigned information back into the caller's TCB
    task->tid        = free_tid;
    task->stack_high = tcb_array[free_tid].stack_high;
    task->state      = READY;
    task->deadline         = deadline_ms;
    task->initial_deadline = deadline_ms;
    task->time_remaining   = deadline_ms;
    task->sleep_remaining  = 0;

    if (kernel_state == RUNNING && p_current_task != NULL
        && p_current_task->state == RUNNING
        && tcb_array[free_tid].time_remaining < p_current_task->time_remaining) {
        p_current_task->state = READY;
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    }

    return RTX_OK;
}

/**
 * @brief Kernel-side implementation of osCreateTask. Must only run inside the
 * SVC handler. Creates the task with the default deadline of 5ms.
 * @param task TCB of task to be added into Scheduler
 * @return RTX_OK if successful, RTX_ERR if failure
 */
int k_osCreateTask(TCB* task) {
    return k_osCreateTaskCommon(task, DEFAULT_DEADLINE);
}

/**
 * @brief Kernel-side implementation of osCreateDeadlineTask. Must only run
 * inside the SVC handler. Same as osCreateTask but with a custom deadline.
 * @return RTX_OK on success, RTX_ERR on invalid inputs (deadline <= 0, bad
 * stack size, no free TID) or if the stack cannot be allocated
 */
int k_osCreateDeadlineTask(int deadline, TCB* task) {
    if (deadline <= 0) {
        return RTX_ERR;
    }
    return k_osCreateTaskCommon(task, (U32)deadline);
}

int osCreateTask(TCB* task) {
    int result;

    __asm volatile (
        "mov r0, %2      \n" // %2 is the 2nd input: task
        "svc %1          \n" // %1 is the 1st input: OSCREATETASK_SVC
        "mov %0, r0      \n" // %0 is the output: result
        : "=r" (result)
        : "i" (OSCREATETASK_SVC), "r" (task)
        : "r0", "memory"
    );

    return result;
}

int osCreateDeadlineTask(int deadline, TCB* task) {
    int result;

    __asm volatile (
        "mov r0, %2      \n" // %2 is the 2nd input: deadline
        "mov r1, %3      \n" // %3 is the 3rd input: task
        "svc %1          \n" // %1 is the 1st input: OSCREATEDEADLINETASK_SVC
        "mov %0, r0      \n" // %0 is the output: result
        : "=r" (result)
        : "i" (OSCREATEDEADLINETASK_SVC), "r" (deadline), "r" (task)
        : "r0", "r1", "memory"
    );

    return result;
}

// Kernel-side implementation: reads tcb_array, must only run inside the SVC handler
int k_osTaskInfo(task_t TID, TCB* task_copy) {
    if (TID >= MAX_TASKS) return RTX_ERR;
    if (tcb_array[TID].used == 0) return RTX_ERR;

    task_copy->ptask = tcb_array[TID].ptask;
    task_copy->stack_high = tcb_array[TID].stack_high;
    task_copy->tid = tcb_array[TID].tid;
    task_copy->state = tcb_array[TID].state;
    task_copy->stack_size = tcb_array[TID].stack_size;
    task_copy->used = tcb_array[TID].used;
    task_copy->p_next = tcb_array[TID].p_next;
    task_copy->p_stack = tcb_array[TID].p_stack;
    // Ch.6: everything added to the TCB must be copied here too
    task_copy->deadline         = tcb_array[TID].deadline;
    task_copy->initial_deadline = tcb_array[TID].initial_deadline;
    task_copy->time_remaining   = tcb_array[TID].time_remaining;
    task_copy->sleep_remaining  = tcb_array[TID].sleep_remaining;

    return RTX_OK;
}

int osTaskInfo(task_t TID, TCB* task_copy) {
    int result;
    __asm volatile (
        "mov r0, %1      \n" // %1 is the 1st input: TID
        "mov r1, %2      \n" // %2 is the 2nd input: task_copy
        "svc %3          \n" // %3 is the 3rd input: OSTASKINFO_SVC
        "mov %0, r0      \n" // %0 is the output: result
        : "=r" (result)
        : "r" (TID), "r" (task_copy), "i" (OSTASKINFO_SVC)
        : "r0", "r1", "memory"
    );
    return result;
}

void PVC_osScheule(void){
    // Use the kernel-side helper: osGetTID() issues its own SVC, and PendSV
    // must not trigger a nested SVC exception from within its handler.
    task_t current_tid = k_osGetTID();
    if(current_tid>=0&&current_tid<MAX_TASKS){
        uint32_t current_psp = __get_PSP();
        tcb_array[current_tid].p_stack= (U32 *)current_psp;
        if(tcb_array[current_tid].state==READY){
            schedule_push(&Scheduler,&tcb_array[current_tid]);
        }
    }
    get_next_task(&Scheduler); // EDF: picks the READY task with the earliest deadline
    p_current_task->state=RUNNING;

    __set_PSP((uint32_t)p_current_task->p_stack);

}

// Kernel-side implementation: marks the running task READY (so PVC_osSchedule
// re-queues it) and triggers PendSV, which performs the actual context save/
// restore (steps 1, 3, 4 of osYield) by running PVC_osSchedule (step 2).
// Must only run inside the SVC handler.
void k_osYield(void){
    if (p_current_task != NULL && p_current_task->state == RUNNING){
        p_current_task->time_remaining = p_current_task->deadline;
        p_current_task->state = READY;
    }
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

void osYield(void){
    __asm volatile (
        "svc %0          \n" // %0 is the 1st input: OSYIELD_SVC
        :
        : "i" (OSYIELD_SVC)
        : "memory"
    );
}

// Kernel-side implementation of osSleep: immediately halts execution of the
// running task and removes it from scheduling until the sleep time expires.
// SysTick decrements sleep_remaining each ms and wakes the task (its deadline
// countdown is reset to its initial value) when it reaches zero. If no task
// is READY in the meantime, the NULL task runs. Must only run inside the SVC
// handler.
void k_osSleep(int timeInMs){
    if (p_current_task == NULL || p_current_task->state != RUNNING){
        return;
    }
    if (timeInMs <= 0){
        // Nothing to sleep for -- behave like a plain yield
        k_osYield();
        return;
    }
    p_current_task->sleep_remaining = (U32)timeInMs;
    p_current_task->state = SLEEPING; // PendSV will NOT re-queue a sleeping task
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

void osSleep(int timeInMs){
    __asm volatile (
        "mov r0, %1      \n" // %1 is the 2nd input: timeInMs
        "svc %0          \n" // %0 is the 1st input: OSSLEEP_SVC
        :
        : "i" (OSSLEEP_SVC), "r" (timeInMs)
        : "r0", "memory"
    );
}

// Kernel-side implementation of osPeriodYield: sleeps the running task until
// its current period (deadline) elapses. The sleep time is computed by the
// kernel as the task's remaining time-to-deadline, which keeps periodic tasks
// drift-free: however long the task ran this period, it wakes exactly at the
// next period boundary. Must only run inside the SVC handler.
void k_osPeriodYield(void){
    if (p_current_task == NULL || p_current_task->state != RUNNING){
        return;
    }
    U32 remaining = p_current_task->time_remaining;
    if (remaining == 0){
        // Period boundary reached exactly: start the next period immediately
        p_current_task->time_remaining = p_current_task->deadline;
        p_current_task->state = READY;
    } else {
        p_current_task->sleep_remaining = remaining;
        p_current_task->state = SLEEPING;
    }
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

void osPeriodYield(void){
    __asm volatile (
        "svc %0          \n" // %0 is the 1st input: OSPERIODYIELD_SVC
        :
        : "i" (OSPERIODYIELD_SVC)
        : "memory"
    );
}

// Kernel-side implementation of osSetDeadline. Runs inside the SVC handler,
// which executes at a higher exception priority than SysTick -- so the whole
// operation is atomic with respect to timer interrupts, satisfying the
// "this function must block" requirement. Must only run inside the SVC handler.
int k_osSetDeadline(int deadline, task_t TID){
    if (deadline <= 0 || TID >= MAX_TASKS){
        return RTX_ERR;
    }
    TCB* target = &tcb_array[TID];
    // The target must exist, must not be the caller, and must be READY
    if (target->used == FREE_TID || target->state != READY){
        return RTX_ERR;
    }
    if (p_current_task != NULL && TID == p_current_task->tid){
        return RTX_ERR;
    }

    // Permanently change the task's deadline (all future periods use it too)
    target->deadline         = (U32)deadline;
    target->initial_deadline = (U32)deadline;
    target->time_remaining   = (U32)deadline;

    // If the target is now more urgent than the running task, pre-empt
    if (p_current_task != NULL && p_current_task->state == RUNNING
        && target->time_remaining < p_current_task->time_remaining){
        p_current_task->state = READY;
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    }
    return RTX_OK;
}

int osSetDeadline(int deadline, task_t TID){
    int result;
    __asm volatile (
        "mov r0, %1      \n" // %1 is the 2nd input: deadline
        "mov r1, %2      \n" // %2 is the 3rd input: TID
        "svc %3          \n" // %3 is the 4th input: OSSETDEADLINE_SVC
        "mov %0, r0      \n" // %0 is the output: result
        : "=r" (result)
        : "r" (deadline), "r" (TID), "i" (OSSETDEADLINE_SVC)
        : "r0", "r1", "memory"
    );
    return result;
}

void SVC_Handler_Main(unsigned int* svc_args){
  int svc_number = ( ( char * )svc_args[ 6 ] )[ -2 ] ;
  switch (svc_number){
    case 0:
      break;
    case OSTASKEXIT_SVC:
        svc_args[0] = (uint32_t)k_osTaskExit();
      break;
    case OSCREATETASK_SVC:
        svc_args[0] = (uint32_t)k_osCreateTask((TCB*)svc_args[0]);
      break;
    case OSGETTID_SVC:
        svc_args[0] = (uint32_t)k_osGetTID();
      break;
    case OSTASKINFO_SVC:
        svc_args[0] = (uint32_t)k_osTaskInfo((task_t)svc_args[0], (TCB*)svc_args[1]);
      break;
    case OSYIELD_SVC:
        k_osYield();
      break;
    case OSSLEEP_SVC:
        k_osSleep((int)svc_args[0]);
      break;
    case OSPERIODYIELD_SVC:
        k_osPeriodYield();
      break;
    case OSSETDEADLINE_SVC:
        svc_args[0] = (uint32_t)k_osSetDeadline((int)svc_args[0], (task_t)svc_args[1]);
      break;
    case OSCREATEDEADLINETASK_SVC:
        svc_args[0] = (uint32_t)k_osCreateDeadlineTask((int)svc_args[0], (TCB*)svc_args[1]);
      break;
    case MALLOC_SVC: {
        SVC_DATA* data_mal= (SVC_DATA*)svc_args[0];
        svc_args[0] = (uint32_t)k_mem_alloc_SVC(data_mal->mem_size, data_mal->task_id);
        break;
    }
    case DALLOC_SVC: {
        SVC_DATA* data_dal= (SVC_DATA*)svc_args[0];
        svc_args[0] = k_mem_dealloc_SVC(data_dal->mem_ptr, data_dal->task_id);
        break;
    }
    case COUNT_EXT_SVC:
        svc_args[0] = (uint32_t)k_mem_count_extfrag_SVC((size_t)svc_args[0]);
        break;

    case OSKERNELSTART_SVC:
        // On success, k_osKernelStart never returns -- it jumps directly into
        // the first task. It only returns here on failure (RTX_ERR).
        svc_args[0] = (uint32_t)k_osKernelStart();
      break;
    default:
      break;
  }
  return;
}
