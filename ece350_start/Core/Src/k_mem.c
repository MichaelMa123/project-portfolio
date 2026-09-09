#include "k_mem.h"
#define MAIN_STACK_SIZE 0x400
#define MAX_NET_STACK_SIZE 0x3C00
#ifndef THREAD_STACK_SIZE
#define THREAD_STACK_SIZE 0x400
#endif
#define ALLOCATED_MARKER ((mem_block*)0xFFFFFFFF)
#define IS_FREE(block)       ((block)->next_free != ALLOCATED_MARKER)
#define IS_ALLOCATED(block)  ((block)->next_free == ALLOCATED_MARKER)
#define SET_ALLOCATED(block) ((block)->next_free = ALLOCATED_MARKER)

#define KERNEL_MEM_ID 0XFF
#define FOUR_BYTE_ALIGN          0x03
#define FOUR_BYTE_ALIGN_UP(size) (((size) + FOUR_BYTE_ALIGN) & ~FOUR_BYTE_ALIGN)
#define EIGHT_BYTE_ALIGN          0x07
#define EIGHT_BYTE_ALIGN_UP(size) (((size) + EIGHT_BYTE_ALIGN) & ~EIGHT_BYTE_ALIGN)

extern uint32_t _img_end;
extern uint32_t _estack;
extern uint32_t _Min_Stack_Size;
static mem_block* heap_mem_head = NULL;
static mem_block* free_heap_head = NULL; // Explicit free list head for heap
static uint8_t* heap_begin;
static uint8_t* heap_end;

#define BLOCK_HEADER_SIZE   sizeof(mem_block)


// Reports whether the heap has been set up. Used by task creation to
// lazily initialize the heap: the public tests are allowed to skip the
// explicit k_mem_init() call (it is #ifdef'd out in public test 1), so the
// kernel must be able to allocate task stacks regardless. k_mem_alloc's own
// "NULL before k_mem_init" behavior (deliverable 2 spec) is unchanged.
int k_mem_initialized(void)
{
    return heap_mem_head != NULL;
}

int k_mem_init()
{
    uint8_t* top = (uint8_t*)((uint32_t)&_estack - (uint32_t)&_Min_Stack_Size);
    uint8_t* bot = (uint8_t*)&_img_end;
    heap_begin = (uint8_t*)FOUR_BYTE_ALIGN_UP((uint32_t)bot);
    heap_end = top;
    heap_mem_head = (mem_block*)heap_begin;
    heap_mem_head->size = heap_end - heap_begin;
    heap_mem_head->next_free = NULL; 
    heap_mem_head->owner_id = 0;
    heap_mem_head->prev_size = 0; 
    free_heap_head = heap_mem_head;
    
    return 0;
}

void *k_mem_alloc(size_t size)
{
    if(__get_IPSR() == 0){
        SVC_DATA SVC_data_var;
        SVC_data_var.task_id = k_osGetTID();
        SVC_data_var.mem_size = size;
        register void* r0 __asm__("r0") = &SVC_data_var;
        __asm volatile (
            "svc %1 \n"
            : "=r" (r0) 
            : "i" (MALLOC_SVC), "0" (r0) 
            : "memory" 
        );
        return r0;
    }
    else{
        return k_mem_alloc_SVC(size, KERNEL_MEM_ID);
    }
}

int k_mem_dealloc(void *ptr)
{
    if(__get_IPSR() ==0){
        SVC_DATA SVC_data_var;
        SVC_data_var.task_id = osGetTID();
        SVC_data_var.mem_ptr = ptr;
        register void* r0 __asm__("r0") = &SVC_data_var;
        __asm volatile (
            "svc %1 \n"
            : "=r" (r0) 
            : "i" (DALLOC_SVC), "0" (r0) 
            : "memory" 
        );
        return (int)(uintptr_t)r0;
    }
    else{
        return (int)(uintptr_t) k_mem_dealloc_SVC(ptr, KERNEL_MEM_ID);
    }
}

int k_mem_count_extfrag(size_t size)
{
    if (__get_IPSR() == 0) {
        register int r0 __asm__("r0") = (int)size;
        
        __asm volatile (
            "svc %1 \n"
            : "=r" (r0) 
            : "i" (COUNT_EXT_SVC), "0" (r0) 
            : "memory" 
        );
        return r0;
    } 
    else {
        return k_mem_count_extfrag_SVC(size);
    }
}

void * k_mem_alloc_SVC(size_t size, uint32_t caller_id)
{
    if (size == 0 || heap_mem_head == NULL) {
        return NULL;
    }

    uint32_t aligned_size = FOUR_BYTE_ALIGN_UP(size);

    if (aligned_size < size) {
        return NULL;
    }

    uint32_t required_size = aligned_size + BLOCK_HEADER_SIZE;

    if (required_size < aligned_size) {
        return NULL;
    }
    
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    mem_block* current_block = free_heap_head;
    mem_block* prev_free = NULL;

    while (current_block != NULL) {
        if (current_block->size >= required_size) {
            uint32_t remaining_size = current_block->size - required_size;
            if (prev_free == NULL) {
                free_heap_head = current_block->next_free;
            } else {
                prev_free->next_free = current_block->next_free;
            }

            if (remaining_size >= (BLOCK_HEADER_SIZE + 4)) {
                mem_block* new_block = (mem_block*)((uint8_t*)current_block + required_size);
                new_block->size = remaining_size;
                new_block->owner_id = 0;
                new_block->prev_size = required_size;

                mem_block* next_block = (mem_block*)((uint8_t*)new_block + remaining_size);
                if ((uint8_t*)next_block < heap_end) {
                    next_block->prev_size = remaining_size;
                }

                current_block->size = required_size;
                new_block->next_free = free_heap_head;
                free_heap_head = new_block;
            }
            SET_ALLOCATED(current_block);
            current_block->owner_id = caller_id;
            
            __set_PRIMASK(primask);
            return (void*)((uint8_t*)current_block + BLOCK_HEADER_SIZE);
        }
        prev_free = current_block;
        current_block = current_block->next_free;
    }

    __set_PRIMASK(primask);
    return NULL;
}

int k_mem_dealloc_SVC(void *ptr, uint32_t caller_id)
{
    if (ptr == NULL) return RTX_ERR;

    uint8_t* byte_ptr = (uint8_t*)ptr;
    if (byte_ptr < (heap_begin + BLOCK_HEADER_SIZE) || byte_ptr >= heap_end) {
        return RTX_ERR;
    }

    mem_block* block_to_be_freed = (mem_block*)(byte_ptr - BLOCK_HEADER_SIZE);
    if (IS_FREE(block_to_be_freed) || block_to_be_freed->owner_id != caller_id) {
        return RTX_ERR;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    mem_block* next_block = (mem_block*)((uint8_t*)block_to_be_freed + block_to_be_freed->size);
    if ((uint8_t*)next_block < heap_end && IS_FREE(next_block)) {
        mem_block* curr = free_heap_head;
        mem_block* prev = NULL;
        while(curr != NULL) {
            if(curr == next_block) {
                if(prev == NULL) free_heap_head = curr->next_free;
                else prev->next_free = curr->next_free;
                break;
            }
            prev = curr;
            curr = curr->next_free;
        }

        block_to_be_freed->size += next_block->size;
        mem_block* next_next = (mem_block*)((uint8_t*)next_block + next_block->size);
        if ((uint8_t*)next_next < heap_end) {
            next_next->prev_size = block_to_be_freed->size;
        }
    }
    if (block_to_be_freed->prev_size > 0) {
        mem_block* prev_block = (mem_block*)((uint8_t*)block_to_be_freed - block_to_be_freed->prev_size);

        if (IS_FREE(prev_block)) {
            mem_block* curr = free_heap_head;
            mem_block* prev = NULL;
            while(curr != NULL) {
                if(curr == prev_block) {
                    if(prev == NULL) free_heap_head = curr->next_free;
                    else prev->next_free = curr->next_free;
                    break;
                }
                prev = curr;
                curr = curr->next_free;
            }

            prev_block->size += block_to_be_freed->size;

            mem_block* next_block_after_merge = (mem_block*)((uint8_t*)prev_block + prev_block->size);
            if ((uint8_t*)next_block_after_merge < heap_end) {
                next_block_after_merge->prev_size = prev_block->size;
            }
            prev_block->owner_id = 0;
            prev_block->next_free = free_heap_head;
            free_heap_head = prev_block;

            __set_PRIMASK(primask);
            return RTX_OK;
        }
    }
    block_to_be_freed->owner_id = 0;
    block_to_be_freed->next_free = free_heap_head;
    free_heap_head = block_to_be_freed;

    __set_PRIMASK(primask);
    return RTX_OK;
}

int k_mem_count_extfrag_SVC(size_t size)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    int count = 0;
    mem_block* current = free_heap_head;
    
    while (current != NULL) {
        if (current->size < size) {
            count++;
        }
        current = current->next_free;
    }
    
    __set_PRIMASK(primask);
    
    return count;
}