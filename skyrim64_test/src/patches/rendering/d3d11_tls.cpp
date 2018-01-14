#include "common.h"

#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:p_tls_callback1")
#pragma const_seg(push)
#pragma const_seg(".CRT$XLAAA")
extern "C" const PIMAGE_TLS_CALLBACK p_tls_callback1 = TLSPatcherCallback;
#pragma const_seg(pop)

// Defined in CRT headers somewhere. Used purely for sanity checking.
extern "C" unsigned int _tls_index;

#define GET_TLS_BLOCK(Index) (__readgsqword(0x58) + (Index) * sizeof(void *))

unsigned int g_TlsIndex;

INIT_ONCE TLSPatcherInit = INIT_ONCE_STATIC_INIT;
SRWLOCK g_TLSDataLock;

uintptr_t g_MainTLSBlock;
uint32_t g_MainThreadId;

std::unordered_map<uint32_t, uintptr_t> g_ThreadTLSMaps;

uintptr_t g_PageGuardBase;
size_t g_PageGuardSize;
uint32_t g_PageGuardTls = TLS_OUT_OF_INDEXES;

uint32_t delayedThreadPatchIndex;
uint32_t delayedTlsThreadIds[256];
uintptr_t delayedTlsBlocks[256];

uintptr_t AllocateGuardedBlock()
{
	uintptr_t memory = (uintptr_t)VirtualAlloc(nullptr, 0x4000 + 4096 + 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!memory)
		__debugbreak();

	// Prevent bad writes to the first or last pages
	DWORD old;
	VirtualProtect((LPVOID)memory, 4096, PAGE_NOACCESS, &old);
	VirtualProtect((LPVOID)(memory + 0x4000 + 4096), 4096, PAGE_NOACCESS, &old);

	return memory + 4096;
}

void InitializeTLSMain()
{
	InitOnceExecuteOnce(&TLSPatcherInit, [](PINIT_ONCE, PVOID, PVOID *)
	{
		InitializeSRWLock(&g_TLSDataLock);
		return TRUE;
	}, nullptr, nullptr);

	InitializeTLSDll();

	// WARNING: THIS FUNCTION NEEDS TO EXECUTE ON THE MAIN THREAD!!
	uintptr_t *currentTlsBlock = (uintptr_t *)GET_TLS_BLOCK(g_TlsIndex);

	void *block = (void *)AllocateGuardedBlock();
	uint32_t threadId = GetCurrentThreadId();

	if (!block)
		__debugbreak();

	AcquireSRWLockExclusive(&g_TLSDataLock);
	{
		InterlockedExchange(&g_MainThreadId, threadId);
		InterlockedExchangePointer((volatile PVOID *)&g_MainTLSBlock, block);

		g_ThreadTLSMaps.insert_or_assign(g_MainThreadId, *currentTlsBlock);
		*currentTlsBlock = g_MainTLSBlock;

		// Delayed-patch thread indexes
		for (uint32_t i = 0; i < delayedThreadPatchIndex; i++)
		{
			uintptr_t *tlsBlock = (uintptr_t *)(delayedTlsBlocks[i] + g_TlsIndex * sizeof(void *));

			g_ThreadTLSMaps.insert_or_assign(delayedTlsThreadIds[i], *tlsBlock);
			*tlsBlock = g_MainTLSBlock;
		}
	}
	ReleaseSRWLockExclusive(&g_TLSDataLock);
}

void InitializeTLSDll()
{
	void *(*GetStaticTlsData)();
	size_t(*GetStaticTlsDataSize)();
	uint32_t(*GetStaticTlsSlot)();

	AcquireSRWLockExclusive(&g_TLSDataLock);
	{
		if (g_TlsIndex)
			__debugbreak();

		HMODULE lib = LoadLibraryA("skyrim64_tls_mt.dll");
		*(FARPROC *)&GetStaticTlsData = GetProcAddress(lib, "GetStaticTlsData");
		*(FARPROC *)&GetStaticTlsDataSize = GetProcAddress(lib, "GetStaticTlsDataSize");
		*(FARPROC *)&GetStaticTlsSlot = GetProcAddress(lib, "GetStaticTlsSlot");

		if (!GetStaticTlsData || !GetStaticTlsDataSize || !GetStaticTlsSlot)
			__debugbreak();

		if (GetStaticTlsDataSize() <= 0)
			__debugbreak();

		if (GetStaticTlsSlot() == 0 || GetStaticTlsSlot() == _tls_index)
			__debugbreak();

		// g_TlsIndex is the IMPLICIT index for skyrim64_tls_mt.dll. More info: http://www.nynaeve.net/?p=185
		g_TlsIndex = GetStaticTlsSlot();
	}
	ReleaseSRWLockExclusive(&g_TLSDataLock);
}

VOID WINAPI TLSPatcherCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved)
{
	InitOnceExecuteOnce(&TLSPatcherInit, [](PINIT_ONCE, PVOID, PVOID *)
	{
		InitializeSRWLock(&g_TLSDataLock);
		return TRUE;
	}, nullptr, nullptr);

	if (Reason != DLL_THREAD_ATTACH && Reason != DLL_THREAD_DETACH)
		return;

	// Every operation here involves a write
	AcquireSRWLockExclusive(&g_TLSDataLock);

	// WARNING: TLS callbacks can be executed before the CRT itself is! This means
	// anything under std::* can't be used until then. Plain old data must be used.
	if (!g_MainTLSBlock)
	{
		// Still need to track new threads before WinMain()
		delayedTlsThreadIds[delayedThreadPatchIndex] = GetCurrentThreadId();
		delayedTlsBlocks[delayedThreadPatchIndex] = __readgsqword(0x58);
		delayedThreadPatchIndex++;

		if (delayedThreadPatchIndex >= ARRAYSIZE(delayedTlsBlocks))
			__debugbreak();
	}
	else
	{
		// &Teb->ThreadLocalStoragePointer[_tls_index];
		uintptr_t *currentTlsBlock = (uintptr_t *)GET_TLS_BLOCK(g_TlsIndex);
		auto tlsMapping = g_ThreadTLSMaps.find(GetCurrentThreadId());

		if (Reason == DLL_THREAD_ATTACH)
		{
			OutputDebugStringA("TLS Callback: Thread attach\n");

			if (tlsMapping != g_ThreadTLSMaps.end())
				__debugbreak();

			// New thread, redirect TLS[_tls_index] to main thread's TLS[_tls_index]
			g_ThreadTLSMaps.insert_or_assign(GetCurrentThreadId(), *currentTlsBlock);
			*currentTlsBlock = g_MainTLSBlock;
		}
		else if (Reason == DLL_THREAD_DETACH)
		{
			OutputDebugStringA("TLS Callback: Thread detach\n");

			if (tlsMapping == g_ThreadTLSMaps.end())
				__debugbreak();

			// Restore the original thread's TLS block. It's freed in NTDLL on thread exit,
			// causing a heap corruption otherwise.
			*currentTlsBlock = tlsMapping->second;
			g_ThreadTLSMaps.erase(tlsMapping);
		}
	}

	ReleaseSRWLockExclusive(&g_TLSDataLock);
}

void *HACK_GetThreadedGlobals()
{
	return (void *)*(uintptr_t *)GET_TLS_BLOCK(g_TlsIndex);
}

void *HACK_GetMainGlobals()
{
	return (void *)g_MainTLSBlock;
}

uintptr_t GetMyTls()
{
	AcquireSRWLockShared(&g_TLSDataLock);
	uintptr_t map = g_ThreadTLSMaps[GetCurrentThreadId()];
	ReleaseSRWLockShared(&g_TLSDataLock);

	return map;
}

uintptr_t GetMainTls()
{
	return g_MainTLSBlock;
}

void TLS_RevertThread()
{
	*(uintptr_t *)GET_TLS_BLOCK(g_TlsIndex) = GetMyTls();
}

void TLS_RestoreThread()
{
	*(uintptr_t *)GET_TLS_BLOCK(g_TlsIndex) = GetMainTls();
}

void PageGuard_Monitor(uintptr_t VirtualAddress, size_t Size)
{
	g_PageGuardBase = VirtualAddress;
	g_PageGuardSize = Size;

	if (g_PageGuardTls == TLS_OUT_OF_INDEXES)
	{
		g_PageGuardTls = TlsAlloc();
		AddVectoredExceptionHandler(TRUE, PageGuard_Check);
	}

	MEMORY_BASIC_INFORMATION memInfo;
	VirtualQuery((PVOID)VirtualAddress, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));
	VirtualProtect((PVOID)VirtualAddress, Size, memInfo.Protect | PAGE_GUARD, &memInfo.Protect);
}

LONG WINAPI PageGuard_Check(PEXCEPTION_POINTERS ExceptionInfo)
{
	if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
	{
		// Store the offset that is trying to be accessed
		TlsSetValue(g_PageGuardTls, (LPVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);

		ExceptionInfo->ContextRecord->EFlags |= 0x100;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		uintptr_t eip = (uintptr_t)ExceptionInfo->ExceptionRecord->ExceptionAddress;
		uintptr_t offset = (uintptr_t)TlsGetValue(g_PageGuardTls);

		// Is the offset in our range?
		if (offset >= g_PageGuardBase && offset < (g_PageGuardBase + g_PageGuardSize))
		{
			char buf[1024];
			sprintf_s(buf, "ADDR HIT: 0x%llX FROM 0x%llX\n", offset, eip);
			OutputDebugStringA(buf);
		}

		MEMORY_BASIC_INFORMATION memInfo;
		VirtualQuery((PVOID)g_PageGuardBase, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));
		VirtualProtect(memInfo.BaseAddress, memInfo.RegionSize, memInfo.Protect | PAGE_GUARD, &memInfo.Protect);
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}