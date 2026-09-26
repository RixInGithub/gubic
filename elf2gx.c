// elf2gx.c
//
// tool for gubic kernel
// ONLY to be used with compileGx.sh. expect incompatabilities when used improperly!
// doesn't often assume system is le
#include "k/kcommon.h"
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <elf.h>
#include <libelf.h>

#define die(e,...) do {fprintf(stderr,"%s: ", argv[0]);fprintf(stderr,e,##__VA_ARGS__);return 1;} while (false)
#define ass(c,e,...) do { \
	if (!(c)) die(e,##__VA_ARGS__); \
} while (false)
#define MK_PUT_FN(sz) ssize_t true__putU##sz(int fd, uint##sz##_t n) {n=htole##sz(n);return write(fd,&n,sizeof(uint##sz##_t));}

#define htole8(a) (a) // basically an identity func
MK_PUT_FN(8)
MK_PUT_FN(16)
MK_PUT_FN(32)

#define putU(fd,sz,n) ass((true__putU##sz(fd,n)==((sz)/8)), "error while writing!!")

bool true__putTag(int fd, bool notEnd, uint8_t type, uint32_t sz, void*stuff) {
	if (true__putU8(fd,notEnd)!=1) return false;
	if (notEnd) {
		if (true__putU8(fd,type)!=1) return false;
		if (true__putU32(fd,sz)!=4) return false;
		return (write(fd, stuff, sz)==sz);
	}
	return true;
}

#define putTag(fd,...) ass(true__putTag(fd, ##__VA_ARGS__), "error while writing!")

PACKSTRU(DataTag, {
	uint32_t minAlloc;
	uint8_t t;
	uint8_t data[0];
});

int main(int argc, char**argv) {
	char magic[16] = "GUBIC EXE AHEAD!";
	uint16_t majmin = 0x0000;
	uint8_t patch = 0x00;
	ass((argc==3),"provide only a file to convert and an output file!\n");
	printf("converting intermediate elf to gx…\n");
	int fd = open(argv[1], O_RDONLY);
	ass((fd>=0),"could not open file \"%s\"!\n", argv[1]);
	elf_version(EV_CURRENT);
	Elf*onTheShelf = elf_begin(fd, ELF_C_READ, NULL); // haha get it
	ass((elf_kind(onTheShelf)==ELF_K_ELF),"\"%s\" isn't an elf formatted file!\n",argv[1]);
	Elf32_Half shstrtab = elf32_getehdr(onTheShelf)->e_shstrndx;
	ass((elf_getscn(onTheShelf, shstrtab)),"no .shstrtab found what\n");
	Elf_Scn*sect = NULL;
	void*code = NULL;
	uint32_t codeSz;
	int o = open(argv[2], O_WRONLY|O_TRUNC);
	ass((o>=0),"could not open file \"%s\"!\n", argv[2]);
	ass((write(o, magic, sizeof(magic))==sizeof(magic)), "error while writing!");
	putU(o, 16, majmin);
	putU(o, 8, patch);
	while ((sect=elf_nextscn(onTheShelf, sect))!=NULL) {
		bool proc = false;
		Elf32_Shdr*hdr = elf32_getshdr(sect);
		char*n = elf_strptr(onTheShelf,shstrtab,hdr->sh_name);
		if (n==NULL) continue;
		void*raw = NULL;
		uint32_t rawSz = 0;
		#define OBTAIN_DAT() do { \
			Elf_Data*datStru = elf_getdata(sect, NULL); \
			if (!((datStru==NULL)||(datStru->d_size==0))) { \
				rawSz = datStru->d_size; \
				raw = datStru->d_buf; \
			} \
		} while (false)
		#define IFS(inside,obtainDat,...) do { \
			if (!(proc)) { \
				char*__a[] = {__VA_ARGS__}; \
				size_t matched = 0; \
				while (matched<(sizeof(__a)/sizeof(*__a))) { \
					if (strcmp(__a[matched],n)==0) { \
						printf("*** processing section %s…\n", n); \
						if (obtainDat) OBTAIN_DAT(); \
						if (true) inside; \
						proc = true; \
						break; \
					} \
					matched++; \
				} \
			} \
		} while (false)
		/* TODO: interpret these sections:
			.dynamic/.interp,
			.dynsym/.dynstr,
			.strtab (export tag)
		*/
		IFS({
			ass((code==NULL), "multiple .text segments!\n");
			code = raw;
			codeSz = rawSz;
			putTag(o, true, 3, codeSz, code);
		}, true, ".text");
		IFS({
			bool isntBSS = (matched!=2);
			size_t sz = rawSz;
			if (!(isntBSS)) sz = hdr->sh_size;
			size_t alloc = sizeof(DataTag)+(isntBSS*sz);
			DataTag*t = calloc(1, alloc);
			t->t = isntBSS;
			t->minAlloc = sz;
			if (isntBSS) memcpy(t->data, raw, rawSz);
			putTag(o, true, 1, alloc, t);
			free(t);
		}, true, ".data", ".rodata", ".bss");
		if (!(proc)) {
			printf("ignoring section %s!\n", n); \
		}
	}
	putTag(o, false, 0, 0, NULL);
	close(fd);
	elf_end(onTheShelf);
}
