OUTPUT_FORMAT(binary)

SECTIONS {
	. = 0x2000;
	.multiboot : {
		KEEP(*(.multiboot*))
	}
	. = 0x2200;
	.text : {
		*(.text*)
	}
	.rodata : {
		*(.rodata*)
	}
	.data : {
		*(.data*)
	}
	. = 0x10000;
	.bss : {
		*(.bss*)
	}
	. = 0x11000;
}
