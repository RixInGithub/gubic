#include <stdint.h>

int res;

__asm__ (
	".data\n"
	"syscallRet: .long 0\n"
	"syscallEax: .long 0\n"
	"\n"
	".text\n"
	".global syscall\n"
	"syscall:"
		"mov %eax, syscallEax\n"
		"pop %eax\n"
		"mov %eax, syscallRet\n"
		"int $0x80\n"
		"mov %eax, syscallEax\n"
		"mov syscallRet, %eax\n"
		"push %eax\n"
		"mov syscallEax, %eax\n"
		"ret"
);

extern uint32_t syscall(uint32_t a, uint8_t b, uint8_t c, uint32_t d);

uint32_t test(void) {
	static uint32_t a = 0;
	return 0x32+(a++);
}

int main(void) {
	char*a = "wow";
	res = *((int*)a);
	res |= (syscall(0xabcdef01,a[1]*test(),0,0)&(test()<<test()));
	return res;
}
