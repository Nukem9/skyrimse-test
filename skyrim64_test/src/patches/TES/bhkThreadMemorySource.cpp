#include "../../common.h"
#include "MemoryManager.h"
#include "bhkThreadMemorySource.h"

bhkThreadMemorySource::bhkThreadMemorySource()
{
	InitializeCriticalSection(&m_CritSec);
}

bhkThreadMemorySource::~bhkThreadMemorySource()
{
	DeleteCriticalSection(&m_CritSec);
}

void *bhkThreadMemorySource::blockAlloc(int numBytes)
{
	return MemoryManager::Allocate(nullptr, numBytes, 16, true);
}

void bhkThreadMemorySource::blockFree(void *p, int numBytes)
{
	MemoryManager::Deallocate(nullptr, p, true);
}

void *bhkThreadMemorySource::bufAlloc(int& reqNumBytesInOut)
{
	return blockAlloc(reqNumBytesInOut);
}

void bhkThreadMemorySource::bufFree(void *p, int numBytes)
{
	return blockFree(p, numBytes);
}

void *bhkThreadMemorySource::bufRealloc(void *pold, int oldNumBytes, int& reqNumBytesInOut)
{
	void *p = blockAlloc(reqNumBytesInOut);
	memcpy(p, pold, oldNumBytes);
	blockFree(pold, oldNumBytes);

	return p;
}

void bhkThreadMemorySource::blockAllocBatch(void **ptrsOut, int numPtrs, int blockSize)
{
	for (int i = 0; i < numPtrs; i++)
		ptrsOut[i] = blockAlloc(blockSize);
}

void bhkThreadMemorySource::blockFreeBatch(void **ptrsIn, int numPtrs, int blockSize)
{
	for (int i = 0; i < numPtrs; i++)
		blockFree(ptrsIn[i], blockSize);
}

void bhkThreadMemorySource::getMemoryStatistics(class MemoryStatistics& u)
{
	// Nothing
}

int bhkThreadMemorySource::getAllocatedSize(const void *obj, int nbytes)
{
	Assert(false);
	return 0;
}

void bhkThreadMemorySource::resetPeakMemoryStatistics()
{
	// Nothing
}