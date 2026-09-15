#ifndef __COMMON_H
#define __COMMON_H
#include "kstructs.h"

uint32_t roundUp(uint32_t, uint32_t);
uint32_t roundDown(uint32_t, uint32_t);
uint32_t addPad(uint32_t);
void*searchTag(uint32_t, uint32_t*);

extern void*payload;

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

#define NO_DBG 1
#define DEBUG_PORT 0x3f8 // log to com1 if no debug
#define debugC(c) outb(DEBUG_PORT, c)
#if EBUG
	#undef NO_DBG
	#undef DEBUG_PORT
	#define DEBUG_PORT 0xe9
#endif

void debugS(char*);

#define debugL(s) do {debugS(s);debugC(10);} while (false)

void __internal__debugNNewlineless__(uint32_t n, uint8_t shl);

#define __internal__debugNWithCustomLen__(n,l) do {debugS("0x");__internal__debugNNewlineless__(n, l);debugC(10);} while (false)
#define debugN(n) __internal__debugNWithCustomLen__(n,8)
#define debugN16(n) __internal__debugNWithCustomLen__(n,4)
#define debugN8(n) __internal__debugNWithCustomLen__(n,2)

void debugBin(uint8_t n);
void debugXXD(void*_, uint32_t len);

#define debugBool(b) debugL((b)?"yes":"no")

void outb(uint16_t, uint8_t);
uint8_t inb(uint16_t);
void outbNWait(uint16_t, uint8_t);
void ps2Write(uint8_t);
void ps2WriteDat(uint8_t);
uint8_t ps2Read(void);

#endif
