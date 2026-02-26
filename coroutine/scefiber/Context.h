#ifndef COROUTINE_VITA_CONTEXT_H
#define COROUTINE_VITA_CONTEXT_H

#include <sys/types.h>
#include <setjmp.h>
#include <psp2/fiber.h>
#include <stdint.h>
#include "vita_mem_impl.h"

#define SCE_FIBER_CONTEXT_MINIMUM_SIZE  (512)
#define SCE_FIBER_ALIGNMENT (8)
#define SCE_FIBER_SIZE      (128)
#define SCE_FIBER_MAX_NAME_LENGTH (31)
#define SCE_OK (0)

#if INTPTR_MAX <= INT32_MAX
#define COROUTINE_LIMITED_ADDRESS_SPACE
#endif

#define COROUTINE __attribute__((noreturn)) void

struct coroutine_context; // forward decls

typedef enum FIBER_FLAGS {
    ROOT = (1 << 0),       // 1
    INITILIZED = (1 << 1), // 2
    PENDING = (1 << 2),    // 4
    FINALIZED = (1 << 4)   // 8
} fiber_flags_t;

typedef COROUTINE(*coroutine_start)(struct coroutine_context *from, struct coroutine_context *self);

typedef struct coroutine_context {
    SceFiber fiber; // 128 bytes
    fiber_flags_t flags;
    uint32_t fiber_id;
    uint32_t switch_count;
    uint32_t magic_cookie;
    coroutine_start entry;
} ctx_t SCE_ALIGN(8); //__attribute__((aligned(8)));

//void coroutine_initialize_main(ctx_t *ctx);

static inline void coroutine_initialize_main(ctx_t *context) {
	context->flags = ROOT;
	context->magic_cookie = 0xDEADBEEF;
	context->switch_count = 0;
	context->fiber_id = 0;
}

void coroutine_initialize(ctx_t *ctx,
                          coroutine_start start,
                          void *stack,
                          size_t size);

void coroutine_transfer(ctx_t *from, ctx_t *to);
void coroutine_destroy(ctx_t *ctx);
void Init_SceFiber();

static inline size_t vita_adjust_fiber_machine_stack_size(size_t size) {
    if (size < SCE_FIBER_CONTEXT_MINIMUM_SIZE) {
        size = SCE_FIBER_CONTEXT_MINIMUM_SIZE;
    }
    return ALIGN_UP(size, SCE_FIBER_ALIGNMENT);
}

#endif
