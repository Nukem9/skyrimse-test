#pragma once

class bhkThreadMemorySource
{
public:
	char _pad0[0x8];
	CRITICAL_SECTION m_CritSec;

public:
	DECLARE_CONSTRUCTOR_HOOK(bhkThreadMemorySource);

	bhkThreadMemorySource();
	virtual ~bhkThreadMemorySource();
	virtual void *blockAlloc(int numBytes);
	virtual void blockFree(void *p, int numBytes);
	virtual void *bufAlloc(int& reqNumBytesInOut);
	virtual void bufFree(void *p, int numBytes);
	virtual void *bufRealloc(void *pold, int oldNumBytes, int& reqNumBytesInOut);
	virtual void blockAllocBatch(void **ptrsOut, int numPtrs, int blockSize);
	virtual void blockFreeBatch(void **ptrsIn, int numPtrs, int blockSize);
	virtual void getMemoryStatistics(class MemoryStatistics& u);
	virtual int getAllocatedSize(const void *obj, int nbytes);
	virtual void resetPeakMemoryStatistics();
};
static_assert_offset(bhkThreadMemorySource, m_CritSec, 0x10);