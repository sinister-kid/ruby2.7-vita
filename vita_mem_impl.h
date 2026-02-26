#ifndef VITA_MEM_IMPL_H
#define VITA_MEM_IMPL_H

#ifdef __vita__

#include <psp2common/types.h>
#include <sys/types.h>
#include <stddef.h>

#define PAGE_SIZE 0x1000 // 4096
#define PAGE_MASK (PAGE_SIZE - 1)

#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *)-1)

#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))


void *vita_mmap(void *addr, size_t length);
int vita_munmap(void *addr, size_t length);
void vita_update_ruby_stack_limits(void* current_sp, void** out_start, void** out_end);
void vita_get_thread_stack_bounds(void **out_base, size_t *out_size);
/* NOOP */
static inline int vita_mprotect(void *addr, size_t length, int prot) {
    return 0;
}


#endif

#endif
