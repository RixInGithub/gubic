OUTPUT_FORMAT(elf32-i386);

SECTIONS {
	. = 0x8000;
	.data ALIGN(1) : SUBALIGN(0) { # never align memory!
		KEEP(*(.data*))
		KEEP(*(.rodata*))
	}
	.bss ALIGN(1) : SUBALIGN(0) {
		KEEP(*(.bss*))
	}
	. = ALIGN(0x100); # align it!
	.text ALIGN(1) : SUBALIGN(0) { # don't align in here too just in case…
		KEEP(*(.__gubic__start))
		KEEP(*(.text*))
	}
}
