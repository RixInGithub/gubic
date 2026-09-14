#include <stdint.h>

int res;

__attribute__((naked)) uint32_t syscall(uint32_t a, uint8_t b, uint8_t c, uint32_t d) {
	// my model of the syscall. :)
	static uint32_t ret;
	static uint32_t tmpEax;
	__asm__ volatile (
		"mov %%eax, %0\n"
		"pop %%eax\n"
		"mov %%eax, %1\n"
		"syscall\n"
		"mov %%eax, %0\n"
		"mov %1, %%eax\n"
		"push %%eax\n"
		"mov %0, %%eax\n"
		"ret"
		: "=m"(tmpEax), "=m"(ret)
	);
	__builtin_unreachable();
}

uint32_t test(void) {
	static uint32_t a = 0;
	return 0x32+(a++);
}

int main(void) {
	char*a = "wow";
	res = *((int*)a);
	res |= (syscall(0xabcdef01,a[1],0,0,0)&(test()<<test()));
	return res;
}
