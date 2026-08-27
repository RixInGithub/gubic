#!/bin/sh
set -eu

add() { sudo mkdir -p "$mnt"/"$(dirname "$1")" && sudo cp "${2:-$(basename "$1")}" "$mnt"/"$1"; }

disasm() { objdump -b binary -m i386 -M intel -D "$1" | sed -Ee "/^.*nop.*$/d"; }

stuffWithLoop() {
	set -eu
	dev="$1"
	sudo mkfs.exfat "$dev"p1
	rm -rf "$mnt"
	mkdir -p "$mnt"
	sudo mount "$dev"p1 "$mnt"
	add hey.txt
	add grub/grub.cfg
	add gubic.x86 kernel.x86
	tar -cf rd.tar -C rd .
	add rd.img rd.tar
	sudo grub-install --target=i386-pc --boot-directory="$mnt" --recheck "$dev"
}

name=gubic
start=128
mnt=mnt
DEBUG="${DEBUG:-0}"
RUN="${RUN:-$DEBUG}"
kflags=""
test "$DEBUG" = 0 || kflags="-DEBUG=1"
rm -f "$name".img
qemu-img create -f raw "$name".img 64M > /dev/null
printf "label: dos\nstart=$start, type=07, bootable\n" | sfdisk "$name".img > /dev/null
echo "disk creation okay"
rm -f mboot.bin
gcc genMultiboot.c -o genMultiboot.x86_64 -O9
./genMultiboot.x86_64 mboot.bin 2,1,16384,16384,65536,69632 3,1,16896 1,1,1,2,8,16 5,1,0,600,32 4,1,0
rm -f mboot
gcc -m32 -ffreestanding -fno-pie -fno-pic -nostdlib -Wl,-Tkernel.ld -Wl,--build-id=none kernel.c -o kernel.x86 -Os -static -fdata-sections -ffunction-sections $kflags
grub-file --is-x86-multiboot2 kernel.x86
echo "kernel okay"
disasm kernel.x86
dev="$(sudo losetup --find --partscan --show "$name".img)"
stuffWithLoop "$dev" & pid="$!"
okay=y
wait "$pid" || okay=n
sudo losetup -d "$dev"
sudo umount "$mnt" || true
rm -rf "$mnt" rd.tar || true
test $okay = y || exit 1
qflags=""
test "$DEBUG" = 0 || qflags="-S -s -debugcon stdio"
if [ "$RUN" = 1 ]; then
	echo "running gubic..."
	qemu-system-x86_64 -drive format=raw,file="$name".img $qflags
fi
