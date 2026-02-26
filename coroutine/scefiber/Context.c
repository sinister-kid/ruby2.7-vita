
#include "Context.h"

#include <psp2/kernel/clib.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/sysmodule.h>
#include <stdint.h>
#include <stdlib.h>

#include "dlog.h"
#include "vita_mem_impl.h"

// How knows bitmask operations? Not me.
#define TOGGLE_MASK(x, m) ((x) ^= (m))
#define ADD_MASK(x, m) ((x) |= (m))
#define SET_MASK(x, m) ((x) = (m))   /* Sets x to EXACTLY m, discarding everything else */
#define SUB_MASK(x, m) ((x) &= ~(m))
#define ANY_MASK(x, m) ((x) & (m))
#define NONE_MASK(x, m) (((x) & (m)) == 0)
#define ALL_MASK(x, m) (((x) & (m)) == (m))
#define ONLY_MASK(x, m) (((x) & (m)) == (x))
#define EXACT_MASK(x, m) ((x) == (m))
#define ANY_BUT_MASK(x, m, n) (((x) & ((m) | (n))) == (m))

#define sceFiberInit _sceFiberInitializeImpl

static ctx_t *current_ctx = NULL;
uint32_t fiber_counter = 0;

static inline ctx_t *to_ruby_ctx(uint32_t arg) { return (ctx_t *)(uintptr_t)arg; }
static inline uint32_t to_vita_arg(void *p) { return (uint32_t)(uintptr_t)p; }


static COROUTINE vita_fiber_entry(uint32_t argOnInitialize, uint32_t argOnRun) {
    ctx_t *self = to_ruby_ctx(argOnInitialize);
    ctx_t *from = to_ruby_ctx(argOnRun);    
    current_ctx = self;
    self->entry(from, self);    
    abort(); // Never look back
}

void coroutine_initialize(ctx_t *context, coroutine_start start,
                          void *stack_base, size_t stack_size) {

	//sceClibMemset(context, 0, sizeof(ctx_t));
	
	context->magic_cookie = 0xDEADBEEF;
	context->switch_count = 0;
	context->fiber_id = fiber_counter++;
	SET_MASK(context->flags, INITILIZED);
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

static inline void fiber_finalize(ctx_t *c) {
	DLOG("FINALIZE: v=%p v_id=%u", c, c->fiber_id);
    sceFiberFinalize(&c->fiber);
	SET_MASK(c->flags, FINALIZED);
}

void coroutine_destroy(ctx_t *context) {
	DLOG("coroutine_destroy: v=%p v_id=%u", context, context->fiber_id);
  	if (ANY_BUT_MASK(context->flags, (ROOT | FINALIZED), INITILIZED)) {
  	  	return;
  	}
  
  	if (context == current_ctx) {
  	  	ADD_MASK(context->flags, PENDING);
  	  	return;
  	}
	fiber_finalize(context);
}

static inline void fiber_transfer(ctx_t *from, ctx_t *to) {
  	if (ANY_MASK(from->flags, ROOT)) {
  	  	sceFiberRun(&to->fiber, to_vita_arg(from), NULL);
  	  	return;
  	}
  
  	if (ANY_MASK(to->flags, ROOT)) {
  	  	sceFiberReturnToThread(0, NULL);
  	  	return;
  	}
  	sceFiberSwitch(&to->fiber, to_vita_arg(from), NULL);
}

void coroutine_transfer(ctx_t *from, ctx_t *to) {
  	fiber_transfer(from, to);
  	current_ctx = from;
  	if (ANY_MASK(to->flags, PENDING)) { 
		fiber_finalize(to);
	}
}

void Init_SceFiber() {
  	DLOG("Init_SceFiber!");
  	sceSysmoduleLoadModule(SCE_SYSMODULE_FIBER);
}
