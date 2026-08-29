# gubic (GUH-bih-ck)

TODO: ss

a huge work in progress, successful, cool:

 - kernel
 - operating system
 - desktop environment + window manager + 3d renderer
 - userland(?)
 - and so much more!

## why "gubic"?

well, first of all, i used the [totro fantasy name generator](https://dwheeler.com/totro.html) made by [a kind lad](https://dwheeler.com/).

second of all, it kind of sounds like "cubic" which sits well with the fact the whole desktop is a glorified 3d scene. :trollface:

thirdly, it sounds funny! can't a man have some fun with a random namegen? no? then go poop yourself! >:'(

## why a 3d desktop?

i see, that most desktops try to look 3d, so i thought, if i wanted to have a 3d looking desktop, then why not just do one which is actually 3d?

## where should i expect gubic to work nicely?

 1. the [copy.sh v86](https://copy.sh/v86/#setup) emulator made with wasm. **why?**: because it's cool that an os i made could run in just a browser tab. :)
 2. qemu. **why?**: it's the industry standard!

### support not guaranteed, but probably works

 1. bochs. if you enable the vesa extensions for bochs, there might be some chance of gubic working.
 2. virt-manager. although would be cool, i haven't truly tried that out so soon, even though virt-manager virtualizes with the help of kvm/qemu.
 3. real hardware. if you disable secure boot (unless you build grub with uefi support? havent done that yet), gubic *might* just work, but don't expect zero missing drivers for your specific hardware.

## todo

 1. do the gdt

## requirements...

### ...for building:

 1. qemu (if you set the `RUN` flag to `1`)
 2. gcc
 3. binutils
 4. grub tools (`grub-file`, `grub-install`, ...) (or similar tools that insert a multiboot2 compatible bootloader)

```sh
RUN=1 ./build.sh
```

### ...for running

 1. normal hardware (emulators should work best)

### ...for debugging...

 1. qemu
 2. gdb (or a gdb compatible debugger) (unless you set `GDB=0`)

the `RUN` flag's default value is `1` if `DEBUG` is `1`

#### ...with gdb

```sh
DEBUG=1 ./build.sh
```

#### ...without gdb

```sh
DEBUG=1 GDB=0 ./build.sh
```

qemu debugcon logs will still be visible, however, you will not be able to intercept the program with a gdb-compatible debugger (`lldb`, ...)

## huge thanks to...

 1. the [catk](https://github.com/Rodmatronic/CatK/) maintainers, who showed me examples of grub configuration and installation, qemu debugcon, and so much more!

note: ***i do NOT vibecode.***