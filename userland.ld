SECTIONS {
	. = 0x8000;
	.rodata : {
		*(.rodata*)
	}
	.data : {
		*(.data*)
	}
	.bss : {
		*(.bss*)
	}
	# TODO: more compliant to doc
	.text : {
		*(.text*)
	}
}
