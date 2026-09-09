#ifndef INC_K_INIT_H_
#define INC_K_INIT_H_
#include "k_mem.h"
#ifndef __WFI
#define __WFI()                             __asm volatile ("wfi")
#endif


void osKernelInit(void);
int osKernelStart(void);

// Kernel-side implementation of osKernelStart: touches tcb_array/p_current_task
// directly and performs the first context switch. Must only be called from
// within the SVC handler (privileged kernel context).
int k_osKernelStart(void);

#endif /* INC_K_INIT_H_ */
