#include "BFW.h"

#define FLAG(b) (1<<(b))
#define malloc(size) UUrMemory_Block_New(size)
#define free(ptr) UUrMemory_Block_Delete(ptr)

#ifndef assert
#define assert(expr) UUmAssert(expr)
#endif

#define vassert(expr, string) UUmAssert(expr)
#define NONE -1

#if defined(DEBUGGING) && DEBUGGING
#define DEBUG
#endif
