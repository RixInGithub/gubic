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

void*alloc(size_t oneSz, size_t items) {
	size_t minCap = roundUp(oneSz*items,sizeof(AllocHdr));
	if (minCap==0) return NULL; // absolutely not!
	size_t cntCap;
	void*cnt = firstMem;
	while (true) { // TODO: use multiple memory segments?
		MEM_2_CP(cnt);
		cntCap = roundUp(copy.oneSz*copy.items,sizeof(AllocHdr));
		if ((!(copy.taken))&&(cntCap>=minCap)) break; // we found free mem!
		cnt += sizeof(AllocHdr)+cntCap;
	}
	void*o = cnt+sizeof(AllocMeta);
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
	return cnt+sizeof(AllocMeta);
}

void*growAlloc(void*a, size_t newItems) {
	size_t oneSz;
	size_t oldItems;
	size_t oldCap;
	size_t newCap;
	void*hdr = a-sizeof(AllocHdr);
	MEM_2_CP(hdr);
	oneSz = copy.oneSz;
	oldItems = copy.items;
	// no copy back is required since i did not modify anything.
	if (oldItems==newItems) return a;
	newCap = roundUp(newItems*oneSz,sizeof(AllocMeta));
	oldCap = roundUp(oldItems*oneSz,sizeof(AllocMeta));
	if (oldCap==newCap) return a; // is this even required? prob not, since you can't really change oneSz of an allocation.
	debugL("realloc: todo: implement fully!");
	hdr = a+oldCap;
	MEM_2_CP(hdr);
	switch ((char)copy.taken) {
		case 1:
			debugL("realloc: need to allocate bigger version and then move old allocated data!");
			return NULL;
		default:
			return a;
	}
	__builtin_unreachable();
}

bool setupAlloc() {
	debugN(sizeof(GreatPtr));
	return false;
	size_t memSz;
	MBoot2Mem*mem = searchTag(6, &memSz);
	debugXXD(mem,memSz);
	while (!((mem->entSz==sizeof(MBoot2MemEnt))&&(mem->v==0))) {}
	MBoot2MemEnt*ptr = (MBoot2MemEnt*)((uint8_t*)(mem)+sizeof(MBoot2Mem));
	MBoot2MemEnt*end = (MBoot2MemEnt*)((uint8_t*)(mem)+memSz);
	uint32_t amnt = end-ptr;
	bool foundUsable = false;
	AllocMeta*meta;
	while (ptr<end) {
		bool usable = (((ptr->t==1)&&(ptr->len>=(sizeof(AllocHdr)+(sizeof(AllocMeta)*amnt))))&&(ptr->baseHi==0));
		if (usable) {
			uint32_t base = ptr->baseLo;
			debugL("alloc: found memory!");
			debugN(base);
			debugN(ptr->len);
			void*seg = (void*)ptr->baseLo;
			MEM_2_CP(seg);
			copy.taken = false; // ensure taken = false
			copy.oneSz = 1;
			copy.items = ptr->len-sizeof(AllocHdr);
			CP_2_MEM(seg);
			if (!(foundUsable)) {
				foundUsable = true;
				firstMem = seg;
				meta = alloc(sizeof(AllocMeta), 1);
				meta->base = (uintptr_t)base;
				meta->size = (size_t)ptr->len; // truncate
				debugL("alloc: meta");
				debugXXD(meta,sizeof(AllocMeta));
			}
		}
		ptr++;
	}
	return foundUsable;
}
