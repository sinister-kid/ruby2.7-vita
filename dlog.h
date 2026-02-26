#ifndef DLOG_H
#define DLOG_H

#ifdef NO_DEBUG
#define DLOG(fmt, ...)
#else
#ifdef __vita__
#include <psp2/kernel/clib.h>
#define DLOG(fmt, ...) sceClibPrintf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DLOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#endif

#endif