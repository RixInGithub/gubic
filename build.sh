#!/bin/sh
set -eu

add() { sudo mkdir -p "$mnt"/"$(dirname "$1")" && sudo cp "${2:-$(basename "$1")}" "$mnt"/"$1"; }

disasmk() { objdump -b binary -m i386 -M intel -D kernel.x86 | sed -Ee "/^.*nop.*$/d"; }

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

noGrub() {
	echo "sanity check failed"
	exit $1
}

name=gubic
start=128
mnt=mnt
DEBUG="${DEBUG:-n}"
RUN="${RUN:-$DEBUG}"
GDB="${GDB:-$DEBUG}"
ANTICRASH="${ANTICRASH:-n}"
VNC="${VNC:-n}"
KDISASM="${KDISASM:-n}"
LOG="${LOG:-}"
kflags=
test "$DEBUG" = n || kflags="-DEBUG"
test "$ANTICRASH" = n || kflags="${kflags:+$kflags }-DISABLE_INT_0x20"
rm -f "$name".img
qemu-img create -f raw "$name".img 16M > /dev/null
printf "label: dos\nstart=$start, type=07, bootable\n" | sfdisk "$name".img > /dev/null
echo "disk creation okay"
rm -f k/mboot.bin
gcc genMultiboot.c k/kcommon.c -o genMultiboot.x86_64 -O9
cd extras/gso
CFLAGS="-DGSO_GUBIC_K -m32 -mabi=sysv -ffreestanding -no-pie -fno-pie -fno-pic -nostdlib" ./build.sh
mv gso.o ../../k
cd ../../k
../genMultiboot.x86_64 mboot.bin 2,0,8192,8192,65536,69632 3,0,8704 1,0,1,2,8,6 5,0,800,600,32 4,0,0
gcc -m32 -mabi=sysv -ffreestanding -no-pie -fno-pie -fno-pic -nostdlib -Wl,-Tkernel.ld,--build-id=none,--no-warn-rwx-segments *.c gso.o -o ../kernel.x86 -Oz -static -fdata-sections -ffunction-sections $kflags $@
rm -f ../genMultiboot.x86_64 mboot.bin
cd ..
grub-file --is-x86-multiboot2 kernel.x86 || noGrub $?
echo "kernel + multiboot2 header gen okay"
test "$KDISASM" = n || disasmk
dev="$(sudo losetup --find --partscan --show "$name".img)"
stuffWithLoop "$dev" & pid="$!"
okay=y
wait "$pid" || okay=n
sudo losetup -d "$dev"
sudo umount "$mnt" || true
rm -rf "$mnt" rd.tar || true
test $okay = y || exit 1
if [ "$RUN" = y ]; then
	qflags="qemu-system-i386 -drive format=raw,file=\"$name\".img -netdev user,id=mynet0 -device ne2k_pci,netdev=mynet0"
	anticrashExtra="-enable-kvm -m 512 -cpu host" # qemu-system-x86_64: warning: host doesn't support requested feature: CPUID.80000001H:ECX.svm [bit 2]
	qdis=gtk
	test "$VNC" = n || qdis="none -vnc :0" # :3
	qflags="$qflags -display $qdis"
	test "$GDB" = n || qflags="$qflags -S -s"
	test "$DEBUG" = n || qflags="$qflags -debugcon stdio"
	test "$ANTICRASH" = n || anticrashExtra="-d int -no-reboot" # kvm for some reason makes the anticrash logs not show, so i disable kvm to enable the anticrash.
	qflags="$qflags $anticrashExtra"
	# test "${QFLAGS:-}" = "" || qflags="$qflags $QFLAGS"
	loga="1>\"$LOG\".log 2>&1"
	[ "$LOG" = "" ] || qflags="$qflags $loga" # make sure this is always LAST
	qlog="$PS4$qflags" # make it look real
	echo "$qlog"
	exec sh -c "$qflags"
fi
