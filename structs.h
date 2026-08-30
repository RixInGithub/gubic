#ifndef __STRUCTS_H
#include <stdint.h>
#define STRU(name, inside, ...) typedef struct __VA_ARGS__ name inside name
#define PACKSTRU(name, inside) STRU(name, inside, __attribute__((packed)))

#define BIT(a,b,c,d,e,f,g,h) (h<<7)|(g<<6)|(f<<5)|(e<<4)|(d<<3)|(c<<2)|(b<<1)|a // idk why i write bits in big endian, fuck you.

typedef uint32_t P32;

PACKSTRU(IDT, {
	uint16_t fnLo;
	uint16_t always8;
	uint8_t reserved;
	// p: present, 1 if cpu should not ignore this entry.
	// dpl: descriptor privilege level. set to 0 if only kernel can invoke this.
	// gate type: 0 1 1 1 = 32bit interrupt gate, 1 1 1 1 = 32bit trap gate
	//  gate type  0   dpl  p
	// 0, 1, 1, 1, 0, 0, 0, 1
	uint8_t typeAttr;
	uint16_t fnHi;
});

_Static_assert(sizeof(IDT)==8, "idt fucked");

PACKSTRU(SimplePtr, {
    uint16_t limit;
    uint32_t base;
});

_Static_assert(sizeof(SimplePtr)==6, "idtr fucked");

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

STRU(Pnt3, {
	int32_t a;
	int32_t b;
	int32_t c;
});

STRU(Tri3D, {
	Pnt3 a;
	Pnt3 b;
	Pnt3 c;
});

STRU(Pnt2, {
	int32_t a;
	int32_t b;
});

STRU(Tri32D, {
	Pnt3 a;
	Pnt3 b;
});

PACKSTRU(MBoot2MemEnt, {
	uint32_t baseLo;
	uint32_t baseHi;
	uint64_t len;
	uint32_t t;
	uint32_t reserved;
});

_Static_assert(sizeof(MBoot2MemEnt)==24, "multiboot 2 entry 6 size fucked, should be 24");

PACKSTRU(MBoot2Mem, {
	uint32_t entSz;
	uint32_t v;
});

STRU(FBDim, {
	uint32_t p;
	uint32_t w;
	uint32_t h;
	uint8_t rPos;
	uint8_t rSz;
	uint8_t gPos;
	uint8_t gSz;
	uint8_t bPos;
	uint8_t bSz;
});

STRU(MouseBtns, {
	bool left;
	bool mid;
	bool right;
});
#endif
