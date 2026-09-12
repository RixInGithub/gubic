#!/bin/sh
set -eu
here="$(dirname "$0")"
whereToFind=compiled.gx
gcc -o "$whereToFind" $@ -m32 -mabi=sysv -ffreestanding -nostdlib -Wl,-T"$here"/userland.ld,--no-warn-rwx-segments
while [ $# -ge 2 ]; do
	test "$1" = "-o" && whereToFind="$2"
	shift
done
gcc "$here"/elf2gx.c -o "$here"/elf2gx.x86_64 -O9 $(pkg-config --cflags --libs libelf)
tmp="$(mktemp "$whereToFind".trueGx.XXXXXX)" || exit $?
e=0
"$here"/elf2gx.x86_64 "$whereToFind" "$tmp" || e=$?
rm -f "$whereToFind" "$here"/elf2gx.x86_64
mv "$tmp" "$whereToFind"
exit $e
