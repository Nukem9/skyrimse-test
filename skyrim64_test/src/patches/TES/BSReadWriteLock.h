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
    BSReadWriteLock()
    {
        Initialize();
    }

    // Constructor hook
    BSReadWriteLock *Initialize();

    void AcquireRead();
    void ReleaseRead();
    bool TryAcquireRead();

    void AcquireWrite();
    void ReleaseWrite();
    bool TryAcquireWrite();

    bool IsWriteOwner();
    void UpgradeRead();
};
static_assert(sizeof(BSReadWriteLock) <= 0x8, "Lock must fit inside the original game structure");

class BSScopedRWLock
{
private:
    BSReadWriteLock *m_ChildLock;

public:
    BSScopedRWLock() = delete;

    // Constructor hook
    BSScopedRWLock *Initialize(BSReadWriteLock *Child);

    // Destructor hook
    void Deinitialize();
};
static_assert(sizeof(BSScopedRWLock) <= 0x8, "Lock must fit inside the original game structure");

void PatchLocks();
