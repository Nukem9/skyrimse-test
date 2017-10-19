#include "../../stdafx.h"

BSReadWriteLock *BSReadWriteLock::Initialize()
{
	m_Bits = 0;
	m_ThreadId = 0;
	m_WriteCount = 0;
	return this;
}

void BSReadWriteLock::AcquireRead()
{
	ProfileTimer("Read Lock Time");

	for (uint32_t count = 0; !TryAcquireRead();)
	{
		if (++count > 1000)
			YieldProcessor();
	}
}

void BSReadWriteLock::ReleaseRead()
{
	if (IsWriteOwner())
		return;

	m_Bits.fetch_add(-READER, std::memory_order_release);
}

bool BSReadWriteLock::TryAcquireRead()
{
	if (IsWriteOwner())
		return true;

	// fetch_add is considerably (100%) faster than compare_exchange,
	// so here we are optimizing for the common (lock success) case.
	int32_t value = m_Bits.fetch_add(READER, std::memory_order_acquire);

	if (value & (WRITER | UPGRADED))
	{
		m_Bits.fetch_add(-READER, std::memory_order_release);
		return false;
	}

	return true;
}

void BSReadWriteLock::AcquireWrite()
{
	ProfileTimer("Write Lock Time");

	for (uint32_t count = 0; !TryAcquireWrite();)
	{
		if (++count > 1000)
			YieldProcessor();
	}
}

void BSReadWriteLock::ReleaseWrite()
{
	static_assert(READER > WRITER + UPGRADED, "Wrong bits!");

	if (--m_WriteCount > 0)
		return;

	m_ThreadId.store(0, std::memory_order_release);
	m_Bits.fetch_and(~(WRITER | UPGRADED), std::memory_order_release);
}

bool BSReadWriteLock::TryAcquireWrite()
{
	if (IsWriteOwner())
	{
		m_WriteCount++;
		return true;
	}

	int32_t expect = 0;
	if (m_Bits.compare_exchange_strong(expect, WRITER, std::memory_order_acq_rel))
	{
		m_WriteCount = 1;
		m_ThreadId.store((uint16_t)GetCurrentThreadId(), std::memory_order_release);
		return true;
	}

	return false;
}

bool BSReadWriteLock::IsWriteOwner()
{
	return m_ThreadId == (uint16_t)GetCurrentThreadId();
}

void BSReadWriteLock::UpgradeRead()
{
}

BSScopedRWLock *BSScopedRWLock::Initialize(BSReadWriteLock *Child)
{
	m_ChildLock = Child;
	m_ChildLock->AcquireWrite();

	return this;
}

void BSScopedRWLock::Deinitialize()
{
	m_ChildLock->ReleaseWrite();
}

void PatchLocks()
{
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05910), &BSReadWriteLock::Initialize);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05930), &BSReadWriteLock::AcquireRead);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05BF0), &BSReadWriteLock::ReleaseRead);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC059B0), &BSReadWriteLock::AcquireWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05C00), &BSReadWriteLock::ReleaseWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05BA0), &BSReadWriteLock::TryAcquireWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05C30), &BSReadWriteLock::IsWriteOwner);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05C50), &BSScopedRWLock::Initialize);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05CA0), &BSScopedRWLock::Deinitialize);

	//Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xBE9010), &BSSpinLock::AcquireWrite);	// EnterUpgradeableReaderLock -- check parent function
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC05AB0), &BSReadWriteLock::UpgradeRead);		// UpgdateToWriteLock -- this is a no-op
	//Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xBE9270), &BSSpinLock::ReleaseWrite);	// ExitUpgradeableReaderLock -- name might be wrong

	// C05B30 - Enter any lock mode, returns true if entered write, false if read (no sleep time!), but it's unused by game.
}