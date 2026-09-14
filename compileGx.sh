#!/bin/sh
set -eu
here="$(dirname "$0")"
whereToFind=compiled.gx
gcc -Wl,--no-warn-rwx-segments -o "$whereToFind" $@ -m32 -mabi=sysv -ffreestanding -nostdlib -Wl,-T"$here"/userland.ld,-no-pie -fno-pic -fno-pie
while [ $# -ge 2 ]; do
	test "$1" = "-o" && whereToFind="$2"
	shift
done
gcc "$here"/elf2gx.c -o "$here"/elf2gx.x86_64 -O9 $(pkg-config --cflags --libs libelf)
tmp="$(mktemp "$whereToFind".trueGx.XXXXXX)" || exit $?
e=0
objdump -m i386 -M intel -D "$whereToFind" -j .text
"$here"/elf2gx.x86_64 "$whereToFind" "$tmp" || e=$?
rm -f "$whereToFind" "$here"/elf2gx.x86_64
mv "$tmp" "$whereToFind"
exit $e
