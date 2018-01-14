#pragma once

#define BSGRAPHICS_TLS_BASE_OFFSET	0x0			// Offset into TLS data where this struct is stored
#define BSGRAPHICS_BASE_OFFSET		0x304BEF0	// Offset from the EXE base
#define BSGRAPHICS_PATCH_SIZE		0x2594		//0x25A0		// Size of the variable structure (block)

#define TLS_INSTRUCTION_MEMORY_REGION_SIZE (300 * 1024)
#define TLS_INSTRUCTION_BLOCK_SIZE 64

#define TLS_DEBUG_ENABLE 0
#define TLS_DEBUG_MEMORY_ACCESS 0

extern unsigned int g_TlsIndex;

void *HACK_GetThreadedGlobals();
void *HACK_GetMainGlobals();

void InitializeTLSMain();
void InitializeTLSDll();
VOID WINAPI TLSPatcherCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved);

void PageGuard_Monitor(uintptr_t VirtualAddress, size_t Size);
LONG WINAPI PageGuard_Check(PEXCEPTION_POINTERS ExceptionInfo);