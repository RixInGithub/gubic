#!/bin/sh
gcc -m32 -ffreestanding -nostdlib -Wl,-Tuserland.ld -fdata-sections -ffunction-sections $@
while [ $0 != "-o" ]; do
  [ $# -lt 2 ] && break
  shift
done
whereToFind=a.out
[ $# != 0 ] && whereToFind=$1
gcc elf2gx.c -o elf2gx.x86_64 -O9
./elf2gx.x86_64 $whereToFind
# elf2gx.c expects an input file which will be modified to be a .gx file
