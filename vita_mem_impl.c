#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/clib.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <stdint.h>

#include "vita_mem_impl.h"
#include "dlog.h"

#ifdef __vita__

#define MAX_MEM_BLOCKS 512 // How many are too many?

static int block_count = 0;

//typedef struct MemBlock {
//    void *addr;
//    size_t length;
//    SceUID uid;
//} MemBlock;

//static MemBlock blocks[MAX_MEM_BLOCKS];
static SceKernelLwMutexWork mutex;
static int initialized = 0;

static inline void vita_initialize_mem() {
#ifndef VITA_MMAP_USE_MEMALIGN
    sceKernelCreateLwMutex(&mutex, "vita_mem_mutex", 0, 0, NULL);
#endif
    initialized = 1;
}

void *vita_mmap(void *addr, size_t length) {
    DLOG("vita_mmap: addr=%p length=%zu", addr, length);
    if (!initialized) vita_initialize_mem();
    if (length == 0) {
        errno = EINVAL;
        return MAP_FAILED;
    }

#ifdef VITA_MMAP_USE_MEMALIGN
    void *base = NULL;
    base = memalign(PAGE_SIZE, length);
    if (!base) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
#else
    if (block_count >= MAX_MEM_BLOCKS) {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    SceKernelAllocMemBlockOpt opt;
    sceClibMemset(&opt, 0, sizeof(opt));
    opt.size = sizeof(opt);
    opt.attr = SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT;
    opt.alignment = PAGE_SIZE;

    sceKernelLockLwMutex(&mutex, 1, NULL);
    SceUID uid = sceKernelAllocMemBlock("RubyMem", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, length, &opt);
    if (uid < 0) {
        sceKernelUnlockLwMutex(&mutex, 1);
        errno = ENOMEM;
        return MAP_FAILED;
    }
    void *base = NULL;
    sceKernelGetMemBlockBase(uid, &base);
    sceKernelUnlockLwMutex(&mutex, 1);
    block_count++;
#endif
    DLOG("vita_mmap: addr=%p length=%zu", base, length);
    sceClibMemset(base, 0, length);
    return base;
}

int vita_munmap(void *addr, size_t length) {
    DLOG("vita_munmap: addr=%p length=%zu", addr, length);
    if (!addr || !initialized) { //|| length == 0) {
        errno = EINVAL;
        return -1;
    }

#ifdef VITA_MMAP_USE_MEMALIGN
    free(addr);
    return 0;
#else

    sceKernelLockLwMutex(&mutex, 1, NULL);
    SceUID uid = sceKernelFindMemBlockByAddr(addr, length);
    if (uid < 0) {
        sceKernelUnlockLwMutex(&mutex, 1);
        return -1; 
    }
    sceKernelFreeMemBlock(uid);
    sceKernelUnlockLwMutex(&mutex, 1);
    block_count--;
    return 0;
#endif
}


void vita_update_ruby_stack_limits(void *current_sp, void **out_start, void **out_end) {
    DLOG("vita_update_ruby_stack_limits: current_sp=%p", current_sp);
#ifdef VITA_MMAP_USE_MEMALIGN
        *out_start = NULL;
        *out_end = NULL;
        return;
#else
    if (!initialized) {
        *out_start = NULL;
        *out_end = NULL;
        return;
    }

    sceKernelLockLwMutex(&mutex, 1, NULL);
    SceKernelMemBlockInfo info;
    sceClibMemset(&info, 0, sizeof(info));
    int result = sceKernelGetMemBlockInfoByRange(current_sp, 1, &info);
    if (result < 0) {
        sceKernelUnlockLwMutex(&mutex, 1);
        *out_start = NULL;
        *out_end = NULL;
        return;
    }

    if (out_start) *out_start = info.mappedBase;
    if (out_end) *out_end = info.mappedBase + info.mappedSize;
    sceKernelUnlockLwMutex(&mutex, 1);
    return;
#endif
}

void vita_get_thread_stack_bounds(void **out_base, size_t *out_size) {
    SceKernelThreadInfo info;
    sceClibMemset(&info, 0, sizeof(info));
    info.size = sizeof(info);

    if (sceKernelGetThreadInfo(sceKernelGetThreadId(), &info) == 0 && info.stack && info.stackSize > 0) {
        if (out_base) *out_base = info.stack;
        if (out_size) *out_size = info.stackSize;
        return;
    }
    if (out_base) *out_base = NULL;
    if (out_size) *out_size = 0;
}

#endif