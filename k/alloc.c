#include "kcommon.h"
#include <stddef.h>
#include <string.h>

PACKSTRU(AllocHdr, {
	bool taken;
	size_t oneSz;
	size_t items;
});

typedef GreatPtr AllocMeta;

void*firstMem;
AllocHdr copy = {0}; // i don't trust myself.
#define MEM_2_CP(mem) memcpy(&copy, mem, sizeof(AllocHdr))
#define CP_2_MEM(mem) memcpy(mem, &copy, sizeof(AllocHdr))

AllocMeta*aMeta;
size_t aSz;

gubResp alloc(size_t oneSz, size_t items) {
	gubResp r = {0};
	size_t minCap = roundUp(oneSz*items,sizeof(AllocHdr));
	size_t cntCap;
	void*cnt = firstMem;
	while (true) { // TODO: use multiple memory segments?
		MEM_2_CP(cnt);
		cntCap = roundUp(copy.oneSz*copy.items,sizeof(AllocHdr));
		if ((!(copy.taken))&&(cntCap>=minCap)) break; // we found free mem!
		cnt += sizeof(AllocHdr)+cntCap;
	}
	void*o = cnt+sizeof(AllocHdr);
	copy.taken = true;
	copy.oneSz = oneSz;
	copy.items = items;
	CP_2_MEM(cnt);
	size_t diff = cntCap-minCap; // diff will be divisible by 8, except when it's 0.
	if (diff>0) {
		cnt += sizeof(AllocHdr)+minCap;
		MEM_2_CP(cnt);
		copy.taken = false;
		copy.oneSz = 1;
		copy.items = diff-sizeof(AllocHdr);
	}
	memset(o, 0, minCap);
	r.okay = true;
	r.resp = o;
	return r;
}

gubResp growAlloc(void*a, size_t newItems) {
	gubResp r = {0};
	size_t oneSz;
	size_t oldItems;
	size_t oldCap;
	size_t newCap;
	void*hdr = a-sizeof(AllocHdr);
	MEM_2_CP(hdr);
	oneSz = copy.oneSz;
	oldItems = copy.items;
	// no copy back is required since i did not modify anything.
	if (oldItems==newItems) { // bruh
		r.okay = true;
		r.resp = a;
		return r;
	}
	newCap = roundUp(newItems*oneSz,sizeof(AllocHdr));
	oldCap = roundUp(oldItems*oneSz,sizeof(AllocHdr));
	if (newCap<=oldCap) {
		// while newCap may be equal to oldCap,
		// so imagine this: oneSz=2, oldItems=1. oldCap will be 8.
		// now with newItems=1, newCap is also... 8...
		// so same aligned capacity, unequal item counts!
		// smth like that, idk.
		copy.items = newItems;
		CP_2_MEM(hdr);
		if (newCap<oldCap) {
			hdr = a+oldCap;
			MEM_2_CP(hdr);
			copy.taken = false;
			copy.oneSz = 1;
			copy.items = oldCap-newCap-sizeof(AllocHdr);
			CP_2_MEM(hdr);
		}
		r.okay = true;
		r.resp = a;
		return r;
	}
	hdr = a+oldCap;
	MEM_2_CP(hdr);
	switch ((char)copy.taken) {
		case 1:
			gubResp newAlloc = alloc(oneSz, newItems);
			if (!(newAlloc.okay)) return newAlloc;
			void*o = newAlloc.resp;
			memcpy(o, a, oldCap);
			r.okay = true;
			r.resp = o;
			hdr = a-sizeof(AllocHdr);
			MEM_2_CP(hdr);
			copy.taken = false;
			CP_2_MEM(hdr);
			return r;
		default:
			size_t neighborCap = roundUp(copy.items*copy.oneSz,sizeof(AllocHdr));
			memset(a+oldCap, 0, newCap-oldCap); // clear out memory (security reasons)
			hdr = a-sizeof(AllocHdr);
			MEM_2_CP(hdr);
			copy.items = newItems;
			CP_2_MEM(hdr);
			hdr += newCap;
			MEM_2_CP(hdr);
			copy.taken = false;
			copy.oneSz = 1;
			copy.items = neighborCap-(newCap-oldCap);
			CP_2_MEM(hdr);
			r.okay = true;
			r.resp = a;
			return r;
	}
	__builtin_unreachable();
}

bool setupAlloc() {
	size_t memSz;
	MBoot2Mem*mem = searchTag(6, &memSz);
	debugXXD(mem,memSz);
	while (!((mem->entSz==sizeof(MBoot2MemEnt))&&(mem->v==0))) {}
	MBoot2MemEnt*ptr = (MBoot2MemEnt*)((uint8_t*)(mem)+sizeof(MBoot2Mem));
	MBoot2MemEnt*end = (MBoot2MemEnt*)((uint8_t*)(mem)+memSz);
	uint32_t amnt = end-ptr;
	bool foundUsable = false;
	aSz = 0;
	while (ptr<end) {
		bool usable = (((ptr->t==1)&&(ptr->len>=(sizeof(AllocHdr)+(sizeof(AllocMeta)*amnt))))&&(ptr->baseHi==0));
		if (usable) {
			uint32_t len = roundDown(ptr->len,sizeof(AllocHdr));
			uint32_t base = ptr->baseLo;
			debugL("alloc: found memory!");
			debugN(base);
			debugN(len);
			void*seg = (void*)base;
			MEM_2_CP(seg);
			copy.taken = false; // ensure taken = false
			copy.oneSz = 1;
			copy.items = len-sizeof(AllocHdr);
			CP_2_MEM(seg);
			switch ((uint8_t)foundUsable) {
				case 0:
					foundUsable = true;
					firstMem = seg;
					gubResp a = alloc(sizeof(AllocMeta), aSz++);
					if (!(a.okay)) return false;
					aMeta = a.resp;
					break;
				default:
					gubResp newA = growAlloc(aMeta, ++aSz);
					if (!(newA.okay)) return false;
					aMeta = newA.resp;
					break;
			}
			aMeta[aSz-1].base = (uintptr_t)base;
			aMeta[aSz-1].size = (size_t)len;
			debugL("alloc: meta");
			debugXXD(aMeta,sizeof(AllocMeta)*aSz);
		}
		ptr++;
	}
	#undef meta
	return foundUsable;
}
