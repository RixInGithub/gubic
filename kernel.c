#include <stdbool.h>
__asm__ (
	".section .multiboot, \"a\"\n"
	".incbin \"mboot.bin\""
);

__asm__ (".section .data");
char*thing = "32bit kernel";

__asm__ (
	".section .text\n"
	"jmp _start"
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

void qemuDebugL(char*s) {
	#if EBUG
		while (*s) {
			// source, destination
			qemuDebugC(*s);
			s++;
		}
		qemuDebugC(10);
	#endif
}

void _start(void) {
	qemuDebugL("gubic kernel starting!");
	while (true) {}
}