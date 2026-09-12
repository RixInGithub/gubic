#include <stdint.h>

int res;

__attribute__((naked)) uint32_t syscall(uint8_t a1, uint8_t a2, uint16_t a3, uint32_t a4) {
	// my model of the syscall. :)
	__asm__ volatile (
		"syscall\n"
		"ret"
	);
}

int main(void) {
	char*a = "wow";
	res = *((int*)a);
	syscall(0,"wow"[0],0,0);
	return res;
}
