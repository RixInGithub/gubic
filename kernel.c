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

void clear(uint32_t col) {
	P32 abcd = p32FromRGBA(col);
	uint32_t pxCnt = 0;
	uint32_t rowCnt = 0;
	uint32_t w = mb2FB->w;
	uint32_t h = mb2FB->h;
	uint32_t p = (mb2FB->p/4);
	uint32_t area = h*p;
	uint32_t blank = p-w;
	while (pxCnt<area) {
		fb[pxCnt] = abcd;
		pxCnt++;
		rowCnt++;
		if (rowCnt==w) {
			pxCnt += blank;
			rowCnt = 0;
		}
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

static float fMin3(float a, float b, float c) {
	float o = a;
	if (b<o) o=b;
	if (c<o) o=c;
	return o;
}

static uint32_t blend(uint32_t x, uint32_t y, uint32_t col, float amnt) {
	uint32_t rgb = rgbaFromP32(fb[(y*(mb2FB->p/4))+x]);
	if (amnt >= 1) return col;
	if (amnt <= 0) return rgb;
	rgb >>= 8;
	col >>= 8;
	float r1 = (rgb>>16)&0xff;
	float r2 = (col>>16)&0xff;
	r1 += (r2-r1) * amnt;
	float g1 = (rgb>>8)&0xff;
	float g2 = (col>>8)&0xff;
	g1 += (g2-g1) * amnt;
	float b1 = rgb&0xff;
	float b2 = col&0xff;
	b1 += (b2-b1) * amnt;
	uint32_t r = ((uint32_t)r1)&0xff;
	uint32_t g = ((uint32_t)g1)&0xff;
	uint32_t b = ((uint32_t)b1)&0xff;
	return (r<<24)|(g<<16)|(b<<8);
}

int32_t i32Abs(int32_t a) {
	return (a<0)?(0-a):a;
}

#define MIN(a,b) ((a)<(b))?(a):(b)
#define FASTERSIDE(n) (a##n*(int32_t)(minX))+(b##n*(int32_t)(minY))+c##n

void tri(uint32_t col, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3) {
	P32 p32Col = p32FromRGBA(col);
	uint32_t p = mb2FB->p/4;
	uint32_t minX = MIN(min3(x1,x2,x3),p);
	uint32_t maxX = MIN(max3(x1,x2,x3),p);
	uint32_t minY = MIN(min3(y1,y2,y3),mb2FB->h);
	uint32_t maxY = MIN(max3(y1,y2,y3),mb2FB->h);
	int32_t a1 = y1-y2;
	int32_t b1 = x2-x1;
	int32_t c1 = (x1*y2)-(x2*y1);
	int32_t a2 = y2-y3;
	int32_t b2 = x3-x2;
	int32_t c2 = (x2*y3)-(x3*y2);
	int32_t a3 = y3-y1;
	int32_t b3 = x1-x3;
	int32_t c3 = (x3*y1)-(x1*y3);
	int32_t base1 = FASTERSIDE(1);
	int32_t base2 = FASTERSIDE(2);
	int32_t base3 = FASTERSIDE(3);
	int32_t area = base1+a1*x3+b1*y3;
	uint32_t yCnt = minY;
	while (yCnt<maxY) {
		int32_t e1 = base1;
		int32_t e2 = base2;
		int32_t e3 = base3;
		uint32_t xCnt = minX;
		while (xCnt<maxX) {
			// msaa
			#define TRUE_SIDER(n,aM,bM) do { \
				if ((((4*e##n)+(aM*a##n)+(bM*b##n))>0)==(area>0)) aa##n++; \
			} while (false)
			#define SIDER(n) TRUE_SIDER(n, 1, 1);TRUE_SIDER(n, 3, 1);TRUE_SIDER(n, 3, 3);TRUE_SIDER(n, 1, 3)
			int32_t aa1 = 0;
			int32_t aa2 = 0;
			int32_t aa3 = 0;
			SIDER(1);
			SIDER(2);
			SIDER(3);
			if ((aa1+aa2+aa3)>0) {
				P32 abcd = p32Col;
				int32_t a = min3(aa1,aa2,aa3);
				if (a<4) abcd = p32FromRGBA(blend(xCnt,yCnt,col,(float)a/4.0));
				fb[(yCnt*p)+xCnt] = abcd;
			}
			xCnt++;
			e1 += a1;
			e2 += a2;
			e3 += a3;
		}
		yCnt++;
		base1 += b1;
		base2 += b2;
		base3 += b3;
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
	clear(0x22448800);
	tri(0xff800000, 100, 100, 400, 500, 700, 300);
	while (true) {}
	__builtin_unreachable();
}