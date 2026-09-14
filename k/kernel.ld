OUTPUT_FORMAT(binary)

SECTIONS {
	. = 0x2000;
	KSTART = .;
	.multiboot : {
		KEEP(*(.multiboot*))
	}
	. = 0x2200;
	.text : {
		*(.startup)
		*(.text*)
	}
	.data : {
		*(.data*)
		*(.rodata*)
	}
	. = 0x10000;
	.bss : {
		*(.bss*)
	}
	. = 0x11000;
	KEND = .;
	/DISCARD/ : {
		*(.eh_frame*)
	}
}
