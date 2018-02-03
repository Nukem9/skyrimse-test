#pragma once

#include <atomic>
#include <stdint.h>

class BSReadWriteLock
{
private:
    std::atomic<int32_t> m_Bits;     // Must be globally visible
    std::atomic<int16_t> m_ThreadId; // We don't really care what other threads see
    volatile int8_t m_WriteCount;

    enum : int32_t
    {
        READER   = 4,
        WRITER   = 1
    };

public:
	DECLARE_CONSTRUCTOR_HOOK(BSReadWriteLock);

	BSReadWriteLock();
	~BSReadWriteLock();

    void LockForRead();
    void UnlockRead();
    bool TryLockForRead();

    void LockForWrite();
    void UnlockWrite();
    bool TryLockForWrite();

	void LockForReadAndWrite();

    bool IsWritingThread();
};
static_assert(sizeof(BSReadWriteLock) <= 0x8, "Lock must fit inside the original game structure");

class BSAutoReadAndWriteLock
{
private:
    BSReadWriteLock *m_Lock;

public:
	BSAutoReadAndWriteLock() = delete;

    // Constructor hook
	BSAutoReadAndWriteLock *Initialize(BSReadWriteLock *Child);

    // Destructor hook
    void Deinitialize();
};
static_assert(sizeof(BSAutoReadAndWriteLock) <= 0x8, "Lock must fit inside the original game structure");