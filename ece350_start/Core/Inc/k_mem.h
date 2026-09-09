/*
 * k_mem.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_K_MEM_H_
#define INC_K_MEM_H_
#include "stm32f4xx_hal.h"
#include "common.h"
#include "k_task.h"

typedef struct SVC_DATA{
    void * mem_ptr;
    size_t mem_size;
    task_t task_id;
} SVC_DATA;

typedef struct mem_block{
    uint32_t size;
    struct mem_block* next_free;
    uint32_t owner_id;
    uint32_t prev_size;
}mem_block;
//stack memory management
// void mem_init(void);
// void *mem_alloc(uint32_t );
// void mem_free(void*);
// heap memory dec
int k_mem_init();
int k_mem_initialized(void); // 1 once k_mem_init has set up the heap
void* k_mem_alloc(size_t size);
int k_mem_dealloc(void* ptr);
int k_mem_count_extfrag(size_t size);
void *k_mem_alloc_SVC(size_t size, uint32_t caller_id);
int k_mem_dealloc_SVC(void *ptr, uint32_t caller_id);
int k_mem_count_extfrag_SVC(size_t size);
#endif /* INC_K_MEM_H_ */
