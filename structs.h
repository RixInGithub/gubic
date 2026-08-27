#ifndef __STRUCTS_H
#include <stdint.h>
#define STRU(name, inside, ...) typedef struct __VA_ARGS__ name inside name
#define PACKSTRU(name, inside) STRU(name, inside, __attribute__((packed)))

PACKSTRU(FAT32Meta, {
	char oemId[8];
	uint16_t bytesPerSector;
	uint8_t sectorsPerCluster;
	uint16_t reservedSectors;
	uint8_t fats;
	uint16_t rootDirEnt;
	uint16_t totalSectors;
	uint8_t mediaDescriptor;
	uint16_t sectorsPerFAT; // DO NOT USE!
	uint16_t sectorsPerTrack; // ??
	uint16_t headsOrSidesOnStorage;
	uint32_t hiddenSectors;
	uint32_t largeSectorCount;
	// extended boot record here...
	uint32_t sectorsPerFAT32;
	uint16_t flags;
	uint16_t fatVer;
	uint32_t rootCluster;
	uint16_t fsInfoSector;
	uint16_t backupBootSector;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3; // 12 bytes of reserved = 3 u32 reserveds
	uint8_t driveNumber; // ???
	uint8_t reserved4; // nt flags but idgaf im just tryna boot here lmao
	uint8_t sig;
	uint32_t serial;
	char volLbl[11];
	char justSaysFAT32[8];
	// the rest is the boot code that is currently being compiled, and the bootable partition sig.
});

// https://wiki.osdev.org/User:Omarrx024/VESA_Tutorial
PACKSTRU(VBE2Info, {
	char sig[4];
	uint16_t ver;
	uint32_t oem;
	uint32_t capabilities;
	uint32_t videos;
	uint16_t vram; // unit: 64kb
	uint16_t softRev;
	uint32_t vendor;
	uint32_t productName;
	uint32_t productRev;
	uint8_t reserved[222+256];
});

PACKSTRU(VBE2Video, {
	uint16_t attr; // "deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame"
	uint8_t depr1;
	uint8_t depr2;
	uint16_t depr3;
	uint16_t windowSize;
	uint16_t segA;
	uint16_t segB;
	uint32_t depr4;
	uint16_t p;
	uint16_t w;
	uint16_t h;
	uint8_t wChar; // unused
	uint8_t yChar; // unused
	uint8_t planes;
	uint8_t bpp;
	uint8_t depr5;
	uint8_t memModel;
	uint8_t depr6;
	uint8_t imgPages;
	uint8_t rsrvd1;
	uint8_t rMask;
	uint8_t rPos;
	uint8_t gMask;
	uint8_t gPos;
	uint8_t bMask;
	uint8_t bPos;
	uint8_t rsrvdMask;
	uint8_t rsrvdPos;
	uint8_t directColAttr;
	uint32_t fb;
	uint32_t offScreenMemOff;
	uint16_t offScreenMemSz;
	uint8_t rsrvd2[206];
});

STRU(BasicVideo, {
	uint32_t accu;
	uint16_t mode;
	uint8_t bpp;
	uint32_t fb;
	uint16_t p;
	uint16_t w;
	uint16_t h;
	uint8_t r;
	uint8_t g;
	uint8_t b;
});

STRU(Pixel24, {
	uint8_t a;
	uint8_t b;
	uint8_t c;
});

STRU(Pixel32, {
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
});

PACKSTRU(GDTR, {
	uint16_t limit;
	uint32_t base;
});
#endif
