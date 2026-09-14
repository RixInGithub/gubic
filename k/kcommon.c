#include "kcommon.h"
#include <stddef.h>

void*payload = NULL;

uint32_t roundUp(uint32_t a, uint32_t b) {
	return b*((a+b-1)/b); // close enough
}

uint32_t addPad(uint32_t o) {
	return roundUp(o,8);
}

void*searchTag(uint32_t t, uint32_t*size) {
	#define HDRSZ (2*sizeof(uint32_t))
	char*src = payload+HDRSZ;
	uint32_t foundT = -1;
	uint32_t add = 0;
	while (foundT!=0) {
		src += add;
		foundT = *((uint32_t*)src);
		if (foundT==t) {
			if (size!=NULL) {
				uint32_t*where = (uint32_t*)src;
				where++; // skip over type
				*size = (*where)-HDRSZ;
			}
			return src+HDRSZ;
		}
		add = addPad(*(((uint32_t*)src)+1));
	};
	return NULL;
	#undef HDRSZ
}

void outb(uint16_t port, uint8_t data) {
	// source, destination
	__asm__ volatile (
		"outb %b1, %w0"
		:
		: "d"(port), "a"(data)
	);
}

uint8_t inb(uint16_t port) {
	uint8_t data;
	__asm__ volatile (
		"inb %w1, %b0"
		: "=a"(data)
		: "d"(port)
	);
	return data;
}

void outbNWait(uint16_t port, uint8_t data) {
	outb(port,data);
	outb(0,0x80);
}

void ps2Write(uint8_t cmd) {
	while (inb(0x64)&2) {}
	outb(0x64, cmd);
}

void ps2WriteDat(uint8_t dat) {
	while (inb(0x64)&2) {}
	outb(0x60, dat);
}

uint8_t ps2Read(void) {
	while (!(inb(0x64)&1)) {}
	return inb(0x60);
}

void debugS(char*s) {
	if (s==NULL) {debugS("(null)");return;}
	while (*s) {
		debugC(*s);
		s++;
	}
}

void __internal__debugNNewlineless__(uint32_t n, uint8_t shl) {
	shl--;
	while (true) {
		uint32_t shBy = shl<<2;
		uint32_t o = ((n>>shBy)&15)+48;
		if (o>57) o += 39;
		debugC(o);
		if (shl==0) return;
		shl--;
	}
	__builtin_unreachable();
}

void debugBin(uint8_t n) {
	debugS("0b");
	uint8_t sh = 0;
	while (sh<8) {
		debugC(48+((n>>(7-sh))&1));
		sh++;
	}
	debugC(10);
}

void debugXXD(void*_, uint32_t len) {
	uint8_t*buf = _;
	uint8_t*cnt = buf;
	uint8_t*end = buf+len;
	while (cnt<end) {
		__internal__debugNNewlineless__((uint32_t)(uintptr_t)cnt,8);
		debugC(58);
		uint8_t tmp1 = 0;
		while (tmp1<8) {
			uint8_t tmp2 = 0;
			while (tmp2<2) {
				if (cnt<end) {
					if (tmp2==0) debugC(32);
					__internal__debugNNewlineless__((uint32_t)(*cnt),2);
				}
				tmp2++;
				cnt++;
			}
			tmp1++;
		}
		debugC(10);
	}
}
