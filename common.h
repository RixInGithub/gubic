#ifndef __COMMON_H
#define __COMMON_H
#include "structs.h"

int addPad(uint32_t o);

#endif

#ifndef __COMMON_H_IMPL
#define __COMMON_H_IMPL
int addPad(uint32_t o) {
	return 8*((o+7)/8); // close enough
}
#endif
