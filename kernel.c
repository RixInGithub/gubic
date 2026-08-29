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
volatile int32_t mouseState[4] = {0};
volatile bool mouseFirstByte = true;
volatile uint32_t mouse[2] = {0};
P32*offscreen;
uint32_t memMapSz;
struct {
	uint32_t p;
	uint32_t w;
	uint32_t h;
	uint8_t rPos;
	uint8_t rSz;
	uint8_t gPos;
	uint8_t gSz;
	uint8_t bPos;
	uint8_t bSz;
} fbDim = {0};
volatile uint32_t msUp = 0;

__asm__ (
	".section .text\n"
	"movl %ebx, payload\n"
	"cli\n"
	"cmpl $0x36d76289, %eax\n"
	"je k\n"
	"jmp ."
);

static void outb(uint16_t port, uint8_t data) {
	// source, destination
	__asm__ volatile (
		"outb %b1, %w0"
		:
		: "d"(port), "a"(data)
	);
}

static uint8_t inb(uint16_t port) {
	uint8_t data;
	__asm__ volatile (
		"inb %w1, %b0"
		: "=a"(data)
		: "d"(port)
	);
	return data;
}

static void outbNWait(uint16_t port, uint8_t data) {
	outb(port,data);
	outb(0,0x80);
}

static void ps2Write(uint8_t cmd) {
	while (inb(0x64)&2) {}
	outb(0x64, cmd);
}

static void ps2WriteDat(uint8_t dat) {
	while (inb(0x64)&2) {}
	outb(0x60, dat);
}

static uint8_t ps2Read(void) {
	while (!(inb(0x64)&1)) {}
	return inb(0x60);
}

#define qemuDebugC(c)
#if EBUG
	#undef qemuDebugC
	#define qemuDebugC(c) outb(0xe9, c)
#endif

static void qemuDebugS(char*s) {
	#if EBUG
		if (s==NULL) {qemuDebugS("(null)");return;}
		while (*s) {
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

static void __internal__qemuDebugNNewlineless__(uint32_t n, uint8_t shl) {
	shl--;
	while (true) {
		uint32_t shBy = shl<<2;
		uint32_t o = ((n>>shBy)&15)+48;
		if (o>57) o += 39;
		qemuDebugC(o);
		if (shl==0) return;
		shl--;
	}
	__builtin_unreachable();
}

static void qemuDebugN(uint32_t n) {
	qemuDebugS("0x");
	__internal__qemuDebugNNewlineless__(n, 8);
	qemuDebugC(10);
}

void qemuDebugXXD(void*_, uint32_t len) {
	uint8_t*buf = _;
	uint8_t*cnt = buf;
	uint8_t*end = buf+len;
	while (cnt<end) {
		__internal__qemuDebugNNewlineless__((uint32_t)cnt,8);
		qemuDebugC(58);
		uint8_t tmp1 = 0;
		while (tmp1<8) {
			uint8_t tmp2 = 0;
			while (tmp2<2) {
				if (cnt<end) {
					if (tmp2==0) qemuDebugC(32);
					__internal__qemuDebugNNewlineless__((uint32_t)(*cnt),2);
				}
				tmp2++;
				cnt++;
			}
			tmp1++;
		}
		qemuDebugC(10);
	}
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

static P32 p32FromRGBA(uint32_t col) {
	col >>= 8; // discard a
	uint16_t r = (col>>16)&0xff;
	uint16_t g = (col>>8)&0xff;
	uint16_t b = col&0xff;
	P32 o = 0;
	#define MASK_OFF(v) v &= (1u<<fbDim.v##Sz)-1;o |= (v&0xffu)<<(fbDim.v##Pos)
	MASK_OFF(r);
	MASK_OFF(g);
	MASK_OFF(b);
	#undef MASK_OFF
	return o;
}

static uint32_t rgbaFromP32(P32 col) {
	#define CAPTURE(v) uint16_t v = ((col>>(fbDim.v##Pos))&((1u<<fbDim.v##Sz)-1)&0xff)
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
	uint32_t w = fbDim.w;
	uint32_t h = fbDim.h;
	uint32_t p = fbDim.p;
	uint32_t area = h*p;
	uint32_t blank = p-w;
	while (pxCnt<area) {
		offscreen[pxCnt] = abcd;
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

static int32_t clamp(int32_t n, int32_t min, int32_t max) {
	return (min>n)?min:((max<n)?max:n);
}

static uint32_t blend(uint32_t rgb, uint32_t col, uint8_t amnt) {
	if (amnt >= 4) return col;
	if (amnt == 0) return rgb;
	rgb >>= 8;
	col >>= 8;
	uint8_t r1 = (rgb>>16)&0xff;
	r1 += ((((col>>16)&0xff)-r1)*amnt)/4;
	uint8_t g1 = (rgb>>8)&0xff;
	g1 += ((((col>>8)&0xff)-g1)*amnt)/4;
	uint8_t b1 = rgb&0xff;
	b1 += (((col&0xff)-b1)*amnt)/4;
	return ((((uint32_t)r1)&0xff)<<24)|((((uint32_t)g1)&0xff)<<16)|((((uint32_t)b1)&0xff)<<8);
}

static int32_t i32Abs(int32_t a) {
	return (a<0)?(0-a):a;
}

#define MIN(a,b) ((a)<(b))?(a):(b)
#define FASTERSIDE(n) (a##n*(int32_t)(minX))+(b##n*(int32_t)(minY))+c##n

void tri(uint32_t col, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3) {
	P32 p32Col = p32FromRGBA(col);
	uint32_t p = fbDim.p;
	uint32_t minX = MIN(min3(x1,x2,x3),p);
	uint32_t maxX = MIN(max3(x1,x2,x3),p);
	uint32_t minY = MIN(min3(y1,y2,y3),fbDim.h);
	uint32_t maxY = MIN(max3(y1,y2,y3),fbDim.h);
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
		bool gotNonTrans = false;
		bool stop = false;
		while (xCnt<maxX) {
			// msaa
			#define TRUE_SIDER(n,aM,bM) do { \
				if ((((4*e##n)+(aM*a##n)+(bM*b##n))>0)==(area>0)) aa##n++; \
			} while (false)
			#define SIDER(n) TRUE_SIDER(n, 1, 1);TRUE_SIDER(n, 3, 1);TRUE_SIDER(n, 3, 3);TRUE_SIDER(n, 1, 3)
			uint8_t aa1 = 0;
			uint8_t aa2 = 0;
			uint8_t aa3 = 0;
			SIDER(1);
			SIDER(2);
			SIDER(3);
			bool needBlend = true;
			P32 abcd;
			switch (aa1+aa2+aa3) {
				case 0: break;
				case 12:
					needBlend = false;
					abcd = p32Col;
				default:
					if (needBlend) abcd = p32FromRGBA(blend(rgbaFromP32(offscreen[(yCnt*fbDim.p)+xCnt]),col,min3(aa1,aa2,aa3))); // osdev wiki says: "Reading from the video memory is slooow! Use double buffering instead." [sic]
					offscreen[(yCnt*p)+xCnt] = abcd;
					break;
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

void copyOffscreen(void) {
	// most of this shite be copied from `clear` now
	uint32_t cnt = 0;
	uint32_t h = fbDim.h;
	uint32_t p = fbDim.p;
	uint32_t area = h*p;
	while (cnt<area) {
		fb[cnt] = offscreen[cnt];
		cnt++;
	}
}

IDT idt[256] = {0};

SimplePtr idtr = {sizeof(IDT)*256-1,(uint32_t)&idt};

#define SETIRQ(irq,h) do { \
	uint32_t addr = (uint32_t)(h); \
	idt[irq].fnLo = addr&0xffff; \
	idt[irq].fnHi = (addr>>16)&0xffff; \
	idt[irq].always8 = 0x10; \
	idt[irq].typeAttr = /*BIT(0, 1, 1, 1, 0, 0, 0, 1)*/ 0x8e; \
} while (false)

void mousierH(void) {
	uint32_t byte = inb(0x60);
	byte &= 0xff;
	if (mouseFirstByte) {
		mouseFirstByte = false;
		if (byte==0xfa) { // nope
			//qemuDebugL("mouse: had to skip byte 0xfa!");
			return;
		}
	}
	uint8_t toFill = mouseState[0]+1;
	/*qemuDebugS("mouse: got byte 0x");
	__internal__qemuDebugNNewlineless__(byte,2);
	qemuDebugC(10);*/
	if ((toFill==1)&&(!(byte&8))) return;
	mouseState[toFill] = byte;
	mouseState[0]++;
	if (toFill==3) {
		int32_t xMov = mouseState[2];
		int32_t yMov = mouseState[3];
		if ((mouseState[1]>>4)&1) xMov |= 0xffffff00;
		if ((mouseState[1]>>5)&1) yMov |= 0xffffff00;
		mouse[0] = (uint32_t)clamp((int32_t)(mouse[0])+xMov,0,(int32_t)fbDim.w);
		mouse[1] = (uint32_t)clamp((int32_t)(mouse[1])-yMov,0,(int32_t)fbDim.h);
		mouseState[0] = 0;
	}
}

void timerH(void) {
	msUp++;
}

__asm__ (
	".section .text\n"
	".global mouseH\n"
	"mouseH:\n"
		"pusha\n" // pusha pusha yo
		"call mousierH\n"
		"movb $0x20, %al\n"
		"outb %al, $0xa0\n"
		"outb %al, $0x20\n"
		"popa\n"
		"iret" // bye!
);
extern void mouseH(void);

__asm__ (
	".section .text\n"
	".global stubH\n"
	"stubH:\n"
		"pusha\n"
		"movb $0x20, %al\n"
		"outb %al, $0x20\n"
		//"outb %al, $0xa0\n"
		"popa\n"
		"iret\n"
);
extern void stubH(void);

__asm__ (
	".section .text\n"
	".global tH\n"
	"tH:\n"
		"pusha\n"
		"call timerH\n"
		"movb $0x20, %al\n"
		"outb %al, $0x20\n"
		"popa\n"
		"iret\n"
);
extern void tH(void);

void interruptsSetup(void) {
	uint16_t irqCnt = 0;
	while (irqCnt<0xff) {
		SETIRQ((uint8_t)irqCnt,stubH);
		irqCnt++;
	}
	SETIRQ(0x20, tH);
	SETIRQ(0x2c, mouseH);
	__asm__ volatile (
		"lidt %0"
		: 
		: "m"(idtr)
	);
	// interrupts, mouse, and everything in between.
	outbNWait(0x20, 0x11);
	outbNWait(0xa0, 0x11);
	outbNWait(0x21, 0x20);
	outbNWait(0xa1, 0x28);
	outbNWait(0x21, 0x04);
	outbNWait(0xa1, 0x02);
	outbNWait(0x21, 0x01);
	outbNWait(0xa1, 0x01);
	outb(0x21, 0xff);
	outb(0xa1, 0xff);
	ps2Write(0xad);
	ps2Write(0xa7);
	while (inb(0x64) & 1) {
		inb(0x60);
	}
	ps2Write(0x20);
	uint8_t status = ps2Read();
	status |= (1 << 1);
	status &= ~(1 << 5);
	ps2Write(0x60);
	ps2WriteDat(status);
	ps2Write(0xa8);
	ps2Write(0xd4);
	ps2WriteDat(0xf4);
	outb(0x21, 0xfa);
	outb(0xa1, 0xef);
	// end
	uint16_t div = 1193; // Math.round(1193182/x)
	outb(0x43, 0x36);
    outb(0x40, (uint8_t)(div&0xFF));
    outb(0x40, (uint8_t)((div>>8)&0xFF));
	__asm__ volatile ("sti");
}

void k(void) {
	qemuDebugL("gubic kernel starting!");
	qemuDebugS("ebx: ");
	qemuDebugN((uint32_t)payload);
	char*cmdline = searchTag(1,NULL);
	qemuDebugS("command line: ");
	qemuDebugL(cmdline);
	char*bootloader = searchTag(2,NULL);
	qemuDebugS("bootloader: ");
	qemuDebugL(bootloader);
	mb2FB = searchTag(8, NULL);
	while (!((mb2FB->fbHi==0)&&((mb2FB->bpp==32)&&(mb2FB->t==1)))) {}
	qemuDebugL("32bit addr, 32bit bpp, type 1 fb, can double buffer!");
	fb = mb2FB->fb; // holy framebuffer
	fbDim.p = (mb2FB->p)/4;
	fbDim.w = mb2FB->w;
	fbDim.h = mb2FB->h;
	fbDim.rPos = mb2FB->rPos;
	fbDim.rSz = mb2FB->rSz;
	fbDim.gPos = mb2FB->gPos;
	fbDim.gSz = mb2FB->gSz;
	fbDim.bPos = mb2FB->bPos;
	fbDim.bSz = mb2FB->bSz;
	// interruption from standard programme: memory map finding! yay!
	uint32_t offscreenSz = fbDim.p*fbDim.h*4;
	uint32_t memSz;
	bool foundMem = false;
	MBoot2Mem*mem = searchTag(6, &memSz);
	qemuDebugXXD(mem,memSz);
	while (!((mem->entSz==sizeof(MBoot2MemEnt))&&(mem->v==0))) {}
	MBoot2MemEnt*ptr = (MBoot2MemEnt*)((uint8_t*)(mem)+sizeof(MBoot2Mem));
	MBoot2MemEnt*end = (MBoot2MemEnt*)((uint8_t*)(mem)+memSz);
	while (ptr<end) {
		if (ptr->t==1) {
			if (ptr->len>=offscreenSz) {
				qemuDebugL("found it!");
				foundMem = true;
				break;
			}
		}
		ptr++;
	}
	if (!(foundMem)) {
		qemuDebugL("cant find memory for offscreen framebuffer!");
		while (true) {}
	}
	offscreen = (P32*)ptr->baseLo;
	// your programme will resume as usual now.
	mouse[0] = fbDim.w/2;
	mouse[1] = fbDim.h/2;
	interruptsSetup();
	uint32_t minY = 20;
	uint32_t maxY = 100;
	uint32_t y = 60;
	int32_t yInc = 1;
	uint32_t fps = 0;
	uint32_t lastUp = 0;
	while (true) {
		clear(0x22448800);
		tri(0xff800000, 20, 20, 80, 100, 140, y);
		tri(0xffff0000,mouse[0],mouse[1],mouse[0]+16,mouse[1],mouse[0],mouse[1]+16);
		copyOffscreen();
		fps++;
		uint32_t now = msUp;
		if (now-lastUp>=1e3) {
			qemuDebugS("fps: ");
			qemuDebugN(fps);
			fps = 0;
			lastUp = now;
		}
		y += yInc*11;
		if (y>=maxY) {
			// lets say
			// y = 10
			// max y = 9
			// i want y to become 8
			// 10-9=1
			// 9-1=8
			// 9-(10-9) = 9-10+9 = (2*9)-10?
			// replace 9 with maxY, 10 with y
			y = (2*maxY)-y;
			yInc = -1;
			continue;
		}
		if (y<=minY) {
			// lets say
			// y = 3
			// min y = 4
			// i want y to become 5
			// 4-3=1
			// 4+1=5
			// 4+(4-3) = 4+4-3 = (2*4)-3? huh?
			// replace 4 with minY, 3 with y
			y = (minY*2)-y;
			yInc = 1;
		}
	}
	__builtin_unreachable();
}
