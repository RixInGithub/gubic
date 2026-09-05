#ifndef __COMMON_H
#define __COMMON_H
#include "structs.h"

int addPad(uint32_t o);

#define KEY(meaning, ...) __VA_ARGS__, meaning
#define _STR(a) #a
#define STR(a) _STR(a)
#define PRTKEY(l, ...) KEY((STR(l))[0],__VA_ARGS__)

#define KEY_MIN 0x100
#define KEY_CTRL			KEY_MIN
#define KEY_SHIFT			KEY_MIN+1
#define KEY_ALT				KEY_MIN+2
#define KEY_ALTGR			KEY_MIN+3
#define KEY_BKSP			KEY_MIN+4
#define KEY_L				KEY_MIN+5
#define KEY_R				KEY_MIN+6
#define KEY_U				KEY_MIN+7
#define KEY_D				KEY_MIN+8
#define KEY_MAX KEY_D

#endif
#ifndef __COMMON_H_IMPL
#define __COMMON_H_IMPL
int addPad(uint32_t o) {
	return 8*((o+7)/8); // close enough
}
#endif
