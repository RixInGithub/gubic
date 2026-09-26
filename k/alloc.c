#include "kcommon.h"
#include "alloc.h"
#include <stddef.h>
#include <string.h>

PACKSTRU(AllocHdr, {
	size_t oneSz;
	size_t items;
});

AllocHdr copy = {0}; // i don't trust myself.
#define MEM_2_CP(mem) memcpy(&copy, mem, sizeof(AllocHdr))
#define CP_2_MEM(mem) memcpy(mem, &copy, sizeof(AllocHdr))

struct {
	bool canUse;
	AllocMeta extra;
} extraMem = {0};
AllocMeta*aMeta;
size_t aSz;

size_t getSz(size_t oneSz, size_t items) {
	switch (oneSz) {
		case 0:
			return items;
		default:
			return oneSz*items;
	}
	__builtin_unreachable();
}

gubResp findAlloc(AllocMeta where, size_t minCap, size_t*cntCap) {
	gubResp r = {0};
	void*cnt = (void*)where.base;
	void*end = (void*)((uintptr_t)cnt+(uintptr_t)where.size);
	while (cnt<end) {
		MEM_2_CP(cnt);
		*cntCap = roundUp(getSz(copy.oneSz,copy.items),sizeof(AllocHdr));
		if ((copy.oneSz==0)&&((*cntCap)>=minCap)) { // we found free mem!
			r.okay = true;
			r.resp = cnt;
			return r;
		}
		cnt += sizeof(AllocHdr)+(*cntCap);
	}
	*cntCap = 0;
	r.okay = false;
	return r;
}

gubResp alloc(size_t oneSz, size_t items) {
	gubResp r = {0};
	if (oneSz==0) {
		r.okay = false;
		return r;
	}
	gubResp find;
	bool canTryExtra = extraMem.canUse;
	size_t minCap = roundUp(getSz(oneSz,items),sizeof(AllocHdr));
	size_t cntCap;
	size_t idx = 0;
	size_t max = aSz;
	void*hdr;
	while ((idx<max)||(canTryExtra)) {
		if (canTryExtra) {
			find = findAlloc(extraMem.extra, minCap, &cntCap);
			hdr = find.resp;
			canTryExtra = false;
			if (find.okay) break;
		}
		if (idx<max) {
			find = findAlloc(aMeta[idx], minCap, &cntCap);
			hdr = find.resp;
			if (find.okay) break;
			idx++;
		}
	}
	if (!(find.okay)) return find;
	void*o = hdr+sizeof(AllocHdr);
	copy.oneSz = oneSz;
	copy.items = items;
	CP_2_MEM(hdr);
	size_t diff = cntCap-minCap; // diff will be divisible by 8, except when it's 0.
	if (diff>0) {
		hdr += sizeof(AllocHdr)+minCap;
		MEM_2_CP(hdr);
		copy.oneSz = 0;
		copy.items = diff-sizeof(AllocHdr);
		CP_2_MEM(hdr);
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
	if (oneSz==0) {
		r.okay = false;
		return r;
	}
	newCap = roundUp(getSz(oneSz,newItems),sizeof(AllocHdr));
	oldCap = roundUp(getSz(oneSz,oldItems),sizeof(AllocHdr));
	if (newCap<=oldCap) {
		// while newCap may be equal to oldCap,
		// so imagine this: oneSz=2, oldItems=1. oldCap will be 8.
		// now with newItems=1, newCap is also… 8?
		// so same aligned capacity, unequal item counts!
		// smth like that, idk.
		copy.items = newItems;
		CP_2_MEM(hdr);
		if (newCap<oldCap) {
			hdr = a+oldCap;
			MEM_2_CP(hdr);
			copy.oneSz = 0;
			copy.items = oldCap-newCap-sizeof(AllocHdr);
			CP_2_MEM(hdr);
		}
		r.okay = true;
		r.resp = a;
		return r;
	}
	hdr = a+oldCap;
	MEM_2_CP(hdr);
	size_t neighborCap = roundUp(getSz(copy.oneSz,copy.items),sizeof(AllocHdr));
	switch ((uint8_t)((copy.oneSz!=0)||(neighborCap<(newCap-oldCap)))) {
		case 1:
			gubResp newAlloc = alloc(oneSz, newItems);
			if (!(newAlloc.okay)) return newAlloc;
			void*o = newAlloc.resp;
			memcpy(o, a, oldCap);
			r.okay = true;
			r.resp = o;
			hdr = a-sizeof(AllocHdr);
			MEM_2_CP(hdr);
			size_t total = getSz(copy.oneSz,copy.items);
			copy.oneSz = 0;
			copy.items = total;
			CP_2_MEM(hdr);
			return r;
		default:
			memset(a+oldCap, 0, newCap-oldCap); // clear out memory (security reasons)
			hdr = a-sizeof(AllocHdr);
			MEM_2_CP(hdr);
			copy.items = newItems;
			CP_2_MEM(hdr);
			if (neighborCap!=0) {
				hdr += newCap+sizeof(AllocHdr);
				MEM_2_CP(hdr);
				copy.oneSz = 0;
				copy.items = neighborCap-(newCap-oldCap);
				CP_2_MEM(hdr);
			}
			r.okay = true;
			r.resp = a;
			return r;
	}
	__builtin_unreachable();
}

bool setupAlloc() {
	size_t memSz;
	// interruption from standard programme: memory map finding! yay!
	MBoot2Mem*mem = searchTag(6, &memSz);
	debugXXD(mem,memSz);
	while (!((mem->entSz==sizeof(MBoot2MemEnt))&&(mem->v==0))) {}
	MBoot2MemEnt*ptr = (MBoot2MemEnt*)((uint8_t*)(mem)+sizeof(MBoot2Mem));
	MBoot2MemEnt*end = (MBoot2MemEnt*)((uint8_t*)(mem)+memSz);
	uint32_t amnt = end-ptr;
	bool foundUsable = false;
	aSz = 0;
	AllocMeta curr;
	while (ptr<end) {
		bool usable = (((ptr->t==1)&&(ptr->len>=(sizeof(AllocHdr)+(sizeof(AllocMeta)*amnt))))&&(ptr->baseHi==0));
		if (usable) {
			size_t len = roundDown(ptr->len,sizeof(AllocHdr));
			uintptr_t base = ptr->baseLo;
			debugL("setupAlloc: found memory!");
			debugN(base);
			debugN(len);
			curr.base = (uintptr_t)base;
			curr.size = (size_t)len;
			void*seg = (void*)base;
			MEM_2_CP(seg);
			copy.oneSz = 0; // ensure free
			copy.items = len-sizeof(AllocHdr);
			CP_2_MEM(seg);
			extraMem.canUse = (!(foundUsable));
			extraMem.extra = curr;
			switch ((uint8_t)extraMem.canUse) {
				case 1:
					foundUsable = true;
					gubResp a = alloc(sizeof(AllocMeta), ++aSz);
					if (!(a.okay)) return false;
					aMeta = a.resp;
					break;
				default:
					gubResp newA = growAlloc(aMeta, ++aSz);
					if (!(newA.okay)) return false;
					aMeta = newA.resp;
					break;
			}
			memcpy(aMeta+aSz-1, &curr, sizeof(AllocMeta));
			debugL("alloc: meta");
			debugXXD(aMeta,sizeof(AllocMeta)*aSz);
		}
		ptr++;
	}
	#undef meta
	return foundUsable;
}

AllocUsage aUsage() {
	AllocUsage u = {0};
	size_t idx = 0;
	size_t max = aSz;
	void*hdr;
	void*hdrEnd;
	while (idx<max) {
		AllocMeta curr = aMeta[idx];
		hdr = (void*)curr.base;
		hdrEnd = (void*)(curr.base+curr.size);
		while (hdr<hdrEnd) {
			MEM_2_CP(hdr);
			size_t add = roundUp(getSz(copy.oneSz,copy.items),sizeof(AllocHdr));
			u.total += add+sizeof(AllocHdr);
			u.used += (copy.oneSz!=0)*add+sizeof(AllocHdr);
			hdr += add+sizeof(AllocHdr);
		}
		idx++;
	}
	return u;
}

void freeAlloc(void*a) {
	void*hdr = a-sizeof(AllocHdr);
	MEM_2_CP(hdr);
	copy.items = copy.oneSz*copy.items;
	copy.oneSz = 0;
	CP_2_MEM(hdr);
}
