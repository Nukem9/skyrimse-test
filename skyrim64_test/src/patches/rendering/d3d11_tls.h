#pragma once

#define BSGRAPHICS_BASE_OFFSET 0x304BEF0	// Offset from the EXE base
#define BSGRAPHICS_PATCH_SIZE  0x2594 //0x25A0		// Size of the variable structure (block)

#define TLS_INSTRUCTION_MEMORY_REGION_SIZE 400000
#define TLS_INSTRUCTION_BLOCK_SIZE 64

#define TLS_DEBUG_ENABLE 0
#define TLS_DEBUG_MEMORY_ACCESS 0

extern "C" unsigned int _tls_index;

void InitializeTLSHooks();
VOID WINAPI TLSPatcherCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved);
uintptr_t GetTlsOffset(void *Variable);

void PageGuard_Monitor(uintptr_t VirtualAddress, size_t Size);
LONG WINAPI PageGuard_Check(PEXCEPTION_POINTERS ExceptionInfo);