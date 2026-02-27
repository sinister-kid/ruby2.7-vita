
#include "Context.h"

#include <psp2/kernel/clib.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/sysmodule.h>
#include <stdint.h>

uint32_t fiber_counter = 1; // Root is 0, doesnt use fiber but does have context

COROUTINE vita_fiber_entry(uint32_t argOnInitialize, uint32_t argOnRun) {
    ctx_t *self = to_ruby_ctx(argOnInitialize);
    ctx_t *from = to_ruby_ctx(argOnRun);

    //current_ctx = self;
    from->flags &= ~CURRENT;
    self->flags |= CURRENT;
    self->entry(from, self);

    abort(); // Never look back
}

void Init_SceFiber() {
    DLOG("Init_SceFiber!");
    sceSysmoduleLoadModule(SCE_SYSMODULE_FIBER);
}
