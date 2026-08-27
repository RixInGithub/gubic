#include <stdbool.h>
#include <stddef.h>
#include "common.h"
__asm__ (
	".section .multiboot, \"a\"\n"
	".incbin \"mboot.bin\""
);

__asm__ (".section .data");
void*payload = NULL;
P32*fb;
MBoot2FBInfo*mb2FB;

__asm__ (
	".section .text\n"
	"movl %ebx, payload\n"
	"cmpl $0x36d76289, %eax\n"
	"je k\n"
	"jmp ."
);

void qemuDebugC(char c) {
	#if EBUG
		__asm__ volatile (
			"outb %0, $0xe9"
			: 
			: "a"(c)
		);
	#endif
}

void qemuDebugS(char*s) {
	#if EBUG
		if (s==NULL) {qemuDebugS("(null)");return;}
		while (*s) {
			// source, destination
			qemuDebugC(*s);
			s++;
		}
	#endif
}

#define qemuDebugL(s)
#if EBUG
	#undef qemuDebugL
	#define qemuDebugL(s) do {qemuDebugS(s);qemuDebugC(10);} while (false)
#endif

void qemuDebugN(uint32_t n) {
	qemuDebugS("0x");
	uint8_t shl = 7;
	while (true) {
		uint32_t shBy = shl<<2;
		uint32_t o = ((n>>shBy)&15)+48;
		if (o>57) o += 39;
		qemuDebugC(o);
		if (shl==0) {
			qemuDebugC(10);
			return;
		}
		shl--;
	}
	__builtin_unreachable();
}

void*searchTag(uint32_t t) {
	char*src = payload+(2*sizeof(uint32_t));
	uint32_t foundT = -1;
	uint32_t add = 0;
	while (foundT!=0) {
		src += add;
		foundT = *((uint32_t*)src);
		if (foundT==t) return src+(2*sizeof(uint32_t));
		add = addPad(*(((uint32_t*)src)+1));
	};
	return NULL;
}

P32 p32FromRGBA(uint32_t col) {
	col >>= 8; // discard a
	uint16_t r = (col>>16)&0xff;
	uint16_t g = (col>>8)&0xff;
	uint16_t b = col&0xff;
	P32 o = 0;
	#define MASK_OFF(v) v &= (1u<<mb2FB->v##Sz)-1;o |= (v&0xffu)<<(mb2FB->v##Pos)
	MASK_OFF(r);
	MASK_OFF(g);
	MASK_OFF(b);
	#undef MASK_OFF
	return o;
}

uint32_t rgbaFromP32(P32 col) {
	#define CAPTURE(v) uint16_t v = ((col>>(mb2FB->v##Pos))&((1u<<mb2FB->v##Sz)-1)&0xff)
	CAPTURE(r);
	CAPTURE(g);
	CAPTURE(b);
	#undef CAPTURE
	return (r<<24)|(g<<16)|(b<<8);
}

void rect(uint32_t col, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
	P32 abcd = p32FromRGBA(col);
	uint32_t yCnt = y;
	while ((yCnt<h+y)&&(yCnt<mb2FB->h)) {
		uint32_t xCnt = x;
		while ((xCnt<w+x)&&(xCnt<mb2FB->p)) {
			fb[(yCnt*(mb2FB->p/4))+xCnt] = abcd;
			xCnt++;
		}
		yCnt++;
	}
}

static uint32_t min3(uint32_t a, uint32_t b, uint32_t c) {
	uint32_t o = a;
	if (b<o) o=b;
	if (c<o) o=c;
	return o;
}

static uint32_t max3(uint32_t a, uint32_t b, uint32_t c) {
	uint32_t o = a;
	if (b>o) o=b;
	if (c>o) o=c;
	return o;
}

double dAbs(double a) {
	return (a<0)?(0-a):a;
}

#define SIDE(ax,ay,bx,by,px,py) (((int32_t)(bx)-(int32_t)(ax))*((int32_t)(py)-(int32_t)(ay)))-(((int32_t)(by)-(int32_t)(ay))*((int32_t)(px)-(int32_t)(ax)))

void tri(uint32_t col, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3) {
	P32 abcd = p32FromRGBA(col);
	uint32_t minX = min3(x1,x2,x3);
	uint32_t maxX = max3(x1,x2,x3);
	uint32_t yCnt = min3(y1,y2,y3);
	uint32_t maxY = max3(y1,y2,y3);
	while ((yCnt<maxY)&&(yCnt<mb2FB->h)) {
		uint32_t xCnt = minX;
		while ((xCnt<maxX)&&(xCnt<mb2FB->p)) {
			#define SIDER(n,p1N,p2N) int32_t side##n = SIDE(x##p1N,y##p1N,x##p2N,y##p2N,xCnt,yCnt)
			SIDER(1,1,2);
			SIDER(2,2,3);
			SIDER(3,3,1);
			if (((side1>=0)&&((side2>=0)&&(side3>=0)))||((side1<=0)&&((side2<=0)&&(side3<=0)))) fb[(yCnt*(mb2FB->p/4))+xCnt] = abcd;
			xCnt++;
		}
		yCnt++;
	}
}

void k(void) {
	qemuDebugL("gubic kernel starting!");
	qemuDebugS("ebx: ");
	qemuDebugN((uint32_t)payload);
	char*cmdline = searchTag(1);
	qemuDebugS("command line: ");
	qemuDebugL(cmdline);
	char*bootloader = searchTag(2);
	qemuDebugS("bootloader: ");
	qemuDebugL(bootloader);
	mb2FB = searchTag(8);
	while (!((mb2FB->fbHi==0)&&((mb2FB->bpp==32)&&(mb2FB->t==1)))) {}
	qemuDebugL("32bit addr, 32bit bpp & type 1 fb!");
	fb = mb2FB->fb; // holy framebuffer
	rect(0x22448800, 0, 0, mb2FB->w, mb2FB->h);
	tri(0xff000000, 100, 100, 400, 500, 700, 300);
	while (true) {}
	__builtin_unreachable();
}