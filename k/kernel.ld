OUTPUT_FORMAT(binary)

SECTIONS {
	. = 0x2000;
	__K_START = .;
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
	__K_END = .;
	/DISCARD/ : {
		*(.eh_frame*)
	}
}
