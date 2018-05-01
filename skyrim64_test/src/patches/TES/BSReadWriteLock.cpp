#include "../../common.h"
#include "BSReadWriteLock.h"

BSReadWriteLock::BSReadWriteLock()
{
}

BSReadWriteLock::~BSReadWriteLock()
{
	Assert(m_Bits == 0 && m_WriteCount == 0);
}

void BSReadWriteLock::LockForRead()
{
	ProfileTimer("Read Lock Time");

	for (uint32_t count = 0; !TryLockForRead();)
	{
		if (++count > 1000)
			YieldProcessor();
	}
}

void BSReadWriteLock::UnlockRead()
{
	if (IsWritingThread())
		return;

	m_Bits.fetch_add(-READER, std::memory_order_release);
}

bool BSReadWriteLock::TryLockForRead()
{
	if (IsWritingThread())
		return true;

	// fetch_add is considerably (100%) faster than compare_exchange,
	// so here we are optimizing for the common (lock success) case.
	int16_t value = m_Bits.fetch_add(READER, std::memory_order_acquire);

	if (value & WRITER)
	{
		m_Bits.fetch_add(-READER, std::memory_order_release);
		return false;
	}

	return true;
}

void BSReadWriteLock::LockForWrite()
{
	ProfileTimer("Write Lock Time");

	for (uint32_t count = 0; !TryLockForWrite();)
	{
		if (++count > 1000)
			YieldProcessor();
	}
}

void BSReadWriteLock::UnlockWrite()
{
	if (--m_WriteCount > 0)
		return;

	m_ThreadId.store(0, std::memory_order_release);
	m_Bits.fetch_and(~WRITER, std::memory_order_release);
}

bool BSReadWriteLock::TryLockForWrite()
{
	if (IsWritingThread())
	{
		m_WriteCount++;
		return true;
	}

	int16_t expect = 0;
	if (m_Bits.compare_exchange_strong(expect, WRITER, std::memory_order_acq_rel))
	{
		m_WriteCount = 1;
		m_ThreadId.store(GetCurrentThreadId(), std::memory_order_release);
		return true;
	}

	return false;
}

void BSReadWriteLock::LockForReadAndWrite()
{
	// This is only called from BSAutoReadAndWriteLock (but it's always a write lock now)
}

bool BSReadWriteLock::IsWritingThread()
{
	return m_ThreadId == GetCurrentThreadId();
}

BSAutoReadAndWriteLock *BSAutoReadAndWriteLock::Initialize(BSReadWriteLock *Child)
{
	m_Lock = Child;
	m_Lock->LockForWrite();

	return this;
}

void BSAutoReadAndWriteLock::Deinitialize()
{
	m_Lock->UnlockWrite();
}

void PatchLocks()
{
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06DF0), &BSReadWriteLock::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06E10), &BSReadWriteLock::LockForRead);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC070D0), &BSReadWriteLock::UnlockRead);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06E90), &BSReadWriteLock::LockForWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC070E0), &BSReadWriteLock::UnlockWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07080), &BSReadWriteLock::TryLockForWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07110), &BSReadWriteLock::IsWritingThread);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07130), &BSAutoReadAndWriteLock::Initialize);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07180), &BSAutoReadAndWriteLock::Deinitialize);

	//Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xBE9010), &BSSpinLock::LockForWrite);	// EnterUpgradeableReaderLock -- check parent function
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06F90), &BSReadWriteLock::LockForReadAndWrite);		// UpgdateToWriteLock -- this is a no-op
	//Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xBE9270), &BSSpinLock::UnlockWrite);	// ExitUpgradeableReaderLock -- name might be wrong
}