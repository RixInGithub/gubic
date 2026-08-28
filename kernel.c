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

static uint32_t min(uint32_t a, uint32_t b) {
	return (a<b)?a:b;
}

static float fMin3(float a, float b, float c) {
	float o = a;
	if (b<o) o=b;
	if (c<o) o=c;
	return o;
}

static uint32_t blend(uint32_t x, uint32_t y, uint32_t col, float amnt) {
	uint32_t rgb = rgbaFromP32(fb[(y*(mb2FB->p/4))+x])>>8;
	col >>= 8;
	if (amnt > 1) amnt = 1;
	if (amnt < 0) amnt = 0;
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

float fAbs(float a) {
	return (a<0)?(0-a):a;
}

#define FSIDE(ax,ay,bx,by,px,py) (((float)(bx)-(float)(ax))*((float)(py)-(float)(ay)))-(((float)(by)-(float)(ay))*((float)(px)-(float)(ax)))
#define I32SIDE(ax,ay,bx,by,px,py) (((int32_t)(bx)-(int32_t)(ax))*((int32_t)(py)-(int32_t)(ay)))-(((int32_t)(by)-(int32_t)(ay))*((int32_t)(px)-(int32_t)(ax)))
#define I32SIGN(d) ((d)/i32Abs(d))

void tri(uint32_t col, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3) {
	uint32_t minX = min(min3(x1,x2,x3),(mb2FB->p/4));
	uint32_t maxX = min(max3(x1,x2,x3),(mb2FB->p/4));
	uint32_t yCnt = min(min3(y1,y2,y3),mb2FB->h);
	uint32_t maxY = min(max3(y1,y2,y3),mb2FB->h);
	int32_t area = I32SIGN(I32SIDE(x1,y1,x2,y2,x3,y3));
	while (yCnt<maxY) {
		uint32_t xCnt = minX;
		while (xCnt<maxX) {
			// msaa
			#define TRUE_SIDER(n,p1N,p2N,xEx,yEx,nn) do { \
				int32_t side##n##_##nn = I32SIGN(FSIDE(x##p1N,y##p1N,x##p2N,y##p2N,(float)(xCnt)+(float)(xEx),(float)(yCnt)+(float)(yEx))); \
				if (side##n##_##nn==area) a##n += 1.0; \
			} while (false)
			#define SIDER(n,p1N,p2N) TRUE_SIDER(n,p1N,p2N, 0.25, 0.25, 1);TRUE_SIDER(n,p1N,p2N, 0.75, 0.25, 2);TRUE_SIDER(n,p1N,p2N, 0.75, 0.75, 3);TRUE_SIDER(n,p1N,p2N, 0.25, 0.75, 4)
			float a1 = 0;
			float a2 = 0;
			float a3 = 0;
			SIDER(1,1,2);
			SIDER(2,2,3);
			SIDER(3,3,1);
			if ((a1+a2+a3)>0) fb[(yCnt*(mb2FB->p/4))+xCnt] = p32FromRGBA(blend(xCnt,yCnt,col,fMin3(a1,a2,a3)/4.0));
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
	clear(0x22448800);
	qemuDebugL("rendering...");
	tri(0xff800000, 200, 200, 800, 1000, 1400, 600);
	qemuDebugL("done!");
	while (true) {}
	__builtin_unreachable();
}