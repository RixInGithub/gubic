// assumes system is le.
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#define die(s, ...) do {fprintf(stderr, "%s: " s "\n", prog, ##__VA_ARGS__);return 1;} while (false)

PACKSTRU(MBoot2Hdr, {
	uint32_t magic; 
	uint32_t always0;
	uint32_t len; // dynamic
	uint32_t checksum; // dynamic
});

PACKSTRU(MBoot2Tag, {
	uint16_t t;
	uint16_t f;
	uint32_t len;
	uint32_t*req;
});

#define MBOOTTAGSIZE sizeof(MBoot2Tag)-sizeof(uint32_t*) // omit loose pointer
int argc;
char**argv;
FILE*o;
MBoot2Hdr hdr = {0};
MBoot2Tag*tags = NULL;
size_t tagLen = 0;
char*prog;

void shift() {
	if (argc<=0) return;
	argc--;
	argv++;
}

uint32_t readA(char**aa) {
	uint32_t o = 0;
	while (!((**aa==44)||(**aa==0))) {
		o*=10;
		o+=(**aa)-48;
		(*aa)++;
	}
	if (**aa==44) (*aa)++;
	return o;
}

MBoot2Tag*newTag() {
	size_t pTagLen = tagLen;
	tagLen++;
	tags = realloc(tags,sizeof(MBoot2Tag)*tagLen);
	MBoot2Tag*t = tags+pTagLen;
	memset(t,0,sizeof(MBoot2Tag));
	return t;
}

int main(int _ac, char**_al) {
	argc=_ac;
	argv=_al;
	prog=*argv;
	shift();
	if (argc<1) die("need output");
	o = fopen(*argv,"wb");
	shift();
	hdr.magic = 0xe85250d6;
	hdr.len = sizeof(MBoot2Hdr);
	while (argc>0) {
		char*a=*argv;
		uint8_t mode = 0;
		MBoot2Tag*t = newTag();
		while (*a!=0) {
			uint32_t n = readA(&a);
			switch (mode) {
				case 0:
					t->t=n;
					break;
				case 1:
					t->f=n;
					break;
				default:
					uint32_t idx = t->len/sizeof(uint32_t);
					t->len += sizeof(uint32_t);
					t->req = realloc(t->req,t->len);
					t->req[idx] = n;
					break;
			}
			if (mode<2) mode++;
		}
		hdr.len += addPad(t->len+MBOOTTAGSIZE);
		shift();
	}
	MBoot2Tag t = {0};
	hdr.len += (t.len = MBOOTTAGSIZE);
	hdr.checksum = 0-(hdr.magic+hdr.always0+hdr.len);
	fwrite(&hdr, sizeof(MBoot2Hdr), 1, o);
	size_t cnt = 0;
	size_t pad = 0;
	while (cnt<tagLen) {
		size_t pLen = tags[cnt].len;
		(tags+cnt)->len += MBOOTTAGSIZE;
		size_t newLen = tags[cnt].len;
		fwrite(tags+cnt,MBOOTTAGSIZE,1,o);
		fwrite(tags[cnt].req,pLen,1,o);
		size_t c = 0;
		while (c<addPad(newLen)-newLen) {
			fputc(0,o);
			c++;
			pad++;
		}
		free(tags[cnt].req);
		cnt++;
	}
	fwrite(&t,MBOOTTAGSIZE,1,o);
	fclose(o);
	free(tags);
	printf("tagLen + 1 = %zu, hdr.len = %u, pad = %zu\n", tagLen+1, hdr.len, pad);
	return 0;
}