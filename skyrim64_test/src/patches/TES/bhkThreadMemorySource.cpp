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
	return MemoryManager::Alloc(nullptr, numBytes, 0, false);
}

void bhkThreadMemorySource::blockFree(void *p, int numBytes)
{
	MemoryManager::Free(nullptr, p, false);
}

void *bhkThreadMemorySource::bufAlloc(int& reqNumBytesInOut)
{
	return MemoryManager::Alloc(nullptr, reqNumBytesInOut, 0, false);
}

void bhkThreadMemorySource::bufFree(void *p, int numBytes)
{
	MemoryManager::Free(nullptr, p, false);
}

void *bhkThreadMemorySource::bufRealloc(void *pold, int oldNumBytes, int& reqNumBytesInOut)
{
	void *p = MemoryManager::Alloc(nullptr, reqNumBytesInOut, 0, false);
	memcpy(p, pold, oldNumBytes);
	MemoryManager::Free(nullptr, pold, false);

	return p;
}

void bhkThreadMemorySource::blockAllocBatch(void **ptrsOut, int numPtrs, int blockSize)
{
	for (int i = 0; i < numPtrs; i++)
		ptrsOut[i] = MemoryManager::Alloc(nullptr, blockSize, 0, false);
}

void bhkThreadMemorySource::blockFreeBatch(void **ptrsIn, int numPtrs, int blockSize)
{
	for (int i = 0; i < numPtrs; i++)
		MemoryManager::Free(nullptr, ptrsIn[i], false);
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