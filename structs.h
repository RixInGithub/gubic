#ifndef __STRUCTS_H
#include <stdint.h>
#define STRU(name, inside, ...) typedef struct __VA_ARGS__ name inside name
#define PACKSTRU(name, inside) STRU(name, inside, __attribute__((packed)))

typedef uint32_t P32;

PACKSTRU(MBoot2FBInfo, {
	P32*fb;
	uint32_t fbHi;
	uint32_t p;
	uint32_t w;
	uint32_t h;
	uint8_t bpp;
	uint8_t t;
	uint16_t reserved;
	uint8_t rPos;
	uint8_t rSz;
	uint8_t gPos;
	uint8_t gSz;
	uint8_t bPos;
	uint8_t bSz;
});

#endif
