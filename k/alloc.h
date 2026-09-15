#ifndef __ALLOC_H
#define __ALLOC_H
#include <stdbool.h>

gubResp alloc(size_t, size_t);
gubResp growAlloc(void*, size_t);
bool setupAlloc();
#endif
