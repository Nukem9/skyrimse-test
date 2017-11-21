#include "common.h"

#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:p_tls_callback1")
#pragma const_seg(push)
#pragma const_seg(".CRT$XLAAA")
EXTERN_C const PIMAGE_TLS_CALLBACK p_tls_callback1 = TLSPatcherCallback;
#pragma const_seg(pop)

uintptr_t g_MainTLSBlock;
std::unordered_map<uint32_t, uintptr_t> g_ThreadTLSMaps;

uintptr_t g_PageGuardBase;
size_t g_PageGuardSize;
uint32_t g_PageGuardTls = TLS_OUT_OF_INDEXES;

void InitializeTLSHooks()
{
	// WARNING: THIS FUNCTION NEEDS TO EXECUTE ON THE MAIN THREAD!!
	g_MainTLSBlock = *(uintptr_t *)(__readgsqword(0x58) + _tls_index * sizeof(void *));

	if (!g_MainTLSBlock)
		__debugbreak();

	g_ThreadTLSMaps.insert_or_assign(GetCurrentThreadId(), g_MainTLSBlock);
}

VOID WINAPI TLSPatcherCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved)
{
	if (Reason == DLL_THREAD_ATTACH)
	{
		OutputDebugStringA("TLS Callback: Thread attach\n");

		// New thread, redirect TLS[_tls_index] to main thread's TLS[_tls_index]
		uintptr_t *currentTlsBlock = (uintptr_t *)(__readgsqword(0x58) + _tls_index * sizeof(void *));

		g_ThreadTLSMaps.insert_or_assign(GetCurrentThreadId(), *currentTlsBlock);
		*currentTlsBlock = g_MainTLSBlock;

		char buf[1024];
		sprintf_s(buf, "TLS Base: 0x%llX\n", *currentTlsBlock);
		OutputDebugStringA(buf);
	}
	else if (Reason == DLL_THREAD_DETACH)
	{
		OutputDebugStringA("TLS Callback: Thread detach\n");

		auto found = g_ThreadTLSMaps.find(GetCurrentThreadId());

		if (found == g_ThreadTLSMaps.end())
			__debugbreak();

		// We need to restore the original thread's memory block here. It's freed in NTDLL on thread exit,
		// causing a heap corruption otherwise.
		*(uintptr_t *)(__readgsqword(0x58) + _tls_index * sizeof(void *)) = (uintptr_t)found->second;
		g_ThreadTLSMaps.erase(found);
	}
}

uintptr_t GetTlsOffset(void *Variable)
{
	uintptr_t tlsBase = *(uintptr_t *)(__readgsqword(0x58) + _tls_index * sizeof(void *));
	return (uintptr_t)Variable - tlsBase;
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

	DWORD old;
	VirtualProtect((PVOID)VirtualAddress, Size, memInfo.Protect | PAGE_GUARD, &old);
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

		DWORD old;
		VirtualProtect(memInfo.BaseAddress, memInfo.RegionSize, memInfo.Protect | PAGE_GUARD, &old);
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}