#ifndef COROUTINE_VITA_CONTEXT_H
#define COROUTINE_VITA_CONTEXT_H

#include <sys/types.h>
#include <psp2/fiber.h>
#include <stdint.h>
#include <stdlib.h>
#include "vita_mem_impl.h"
#include "dlog.h"

// How knows bitmask operations? Not me.
#define TOGGLE_MASK(x, m) ((x) ^= (m))
#define ADD_MASK(x, m) ((x) |= (m))
#define SUB_MASK(x, m) ((x) &= ~(m))
#define ANY_MASK(x, m) ((x) & (m))
#define NONE_MASK(x, m) (((x) & (m)) == 0)
#define ALL_MASK(x, m) (((x) & (m)) == (m))
#define ONLY_MASK(x, m) (((x) & (m)) == (x))
#define ANY_BUT_MASK(x, m, n) (((x) & ((m) | (n))) == (m))

#define SCE_FIBER_CONTEXT_MINIMUM_SIZE  (512)
#define SCE_FIBER_ALIGNMENT (8)
#define SCE_FIBER_SIZE      (128)
#define SCE_FIBER_MAX_NAME_LENGTH (31)
#define SCE_OK (0)
#define sceFiberInit _sceFiberInitializeImpl

#if INTPTR_MAX <= INT32_MAX
#define COROUTINE_LIMITED_ADDRESS_SPACE
#endif

#define COROUTINE __attribute__((noreturn)) void
#define CORO_INLINE inline __attribute__((always_inline))

struct coroutine_context; // forward decls

typedef enum FIBER_FLAGS {
    ROOT = (1 << 0),       // 1
    INITILIZED = (1 << 1), // 2
    PENDING = (1 << 2),    // 4
    FINALIZED = (1 << 4),  // 8
    CURRENT = (1 << 8)     //16
} fiber_flags_t;

typedef COROUTINE(*coroutine_start)(struct coroutine_context *from, struct coroutine_context *self);

typedef struct coroutine_context {
    SceFiber fiber; // 128 bytes
    fiber_flags_t flags;
    uint32_t fiber_id;
    coroutine_start entry;
} ctx_t SCE_ALIGN(8);

extern uint32_t fiber_counter;

void Init_SceFiber();
COROUTINE vita_fiber_entry(uint32_t argOnInitialize, uint32_t argOnRun);

static CORO_INLINE ctx_t *to_ruby_ctx(uint32_t arg) { return (ctx_t *)(uintptr_t)arg; }
static CORO_INLINE uint32_t to_vita_arg(void *p) { return (uint32_t)(uintptr_t)p; }

static CORO_INLINE void coroutine_initialize_main(ctx_t *context) {
    context->flags = ( ROOT | CURRENT );
    context->fiber_id = 0;
}

static CORO_INLINE void coroutine_initialize(ctx_t *context, coroutine_start start,
                                        void *stack_base, size_t stack_size) {
    context->fiber_id = fiber_counter++;
    context->flags = INITILIZED;
    context->entry = start;
    
    DLOG("coroutine_initialize: ctx=%p id=%u", context, context->fiber_id);
    
    int status = sceFiberInit(&context->fiber, "ruby_scefiber", vita_fiber_entry,
                            to_vita_arg(context), //self
                            stack_base, stack_size, NULL);
    
    if (status != SCE_OK) {
        DLOG("coroutine_initialize: sceFiberInit failed with status %d", status);
        abort();
    }
}

static CORO_INLINE void fiber_transfer(ctx_t *from, ctx_t *to) {
    if (from->flags & ROOT) return (void)sceFiberRun(&to->fiber, to_vita_arg(from), NULL);
    if (to->flags & ROOT)   return (void)sceFiberReturnToThread(0, NULL);
    sceFiberSwitch(&to->fiber, to_vita_arg(from), NULL);
}

static CORO_INLINE void fiber_finalize(ctx_t *c) {
    DLOG("FINALIZE: v=%p v_id=%u", c, c->fiber_id);
    sceFiberFinalize(&c->fiber);
    c->flags = FINALIZED;
}

static CORO_INLINE void coroutine_transfer(ctx_t *from, ctx_t *to) {
    fiber_transfer(from, to);

    from->flags |= CURRENT;

    if (to->flags & PENDING) { 
        fiber_finalize(to);
        return;
    }

    to->flags &= ~CURRENT;
}


static CORO_INLINE void coroutine_destroy(ctx_t *context) {
    DLOG("coroutine_destroy: v=%p v_id=%u", context, context->fiber_id);
    if (ANY_BUT_MASK(context->flags, (ROOT | FINALIZED), INITILIZED)) return;
  
    if (context->flags & CURRENT) {
        context->flags |= PENDING;
        return;
    }
    
    fiber_finalize(context);
}

static inline size_t vita_adjust_fiber_machine_stack_size(size_t size) {
    if (size < SCE_FIBER_CONTEXT_MINIMUM_SIZE) {
        size = SCE_FIBER_CONTEXT_MINIMUM_SIZE;
    }
    return ALIGN_UP(size, SCE_FIBER_ALIGNMENT);
}

#endif
