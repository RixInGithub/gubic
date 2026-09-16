#ifndef __ALLOC_H
#define __ALLOC_H
#include <stdbool.h>

typedef GreatPtr AllocMeta;

STRU(AllocUsage, {
	size_t used;
	size_t total;
});

extern AllocMeta*aMeta;
extern size_t aSz;

gubResp alloc(size_t, size_t);
gubResp growAlloc(void*, size_t);
bool setupAlloc();
AllocUsage aUsage();
#endif
