#include "../../xbyak/xbyak.h"
#include "../common.h"

uintptr_t g_CodeRegion;

#define CODE_BLOCK(x) struct __z##x : public Xbyak::CodeGenerator { __z##x() : CodeGenerator(32, (void *)g_CodeRegion) {
#define CODE_BLOCK_END(x) }} x; g_CodeRegion += 32;

ID3D11DeviceContext2 *lastUpdate = (ID3D11DeviceContext2 *)0xDEADBEEFCAFEDEAD;
std::unordered_map<uint32_t, ID3D11DeviceContext2 **> ContextAddrs;

thread_local ID3D11DeviceContext2 *TLS_DeviceContext;

extern "C" unsigned int _tls_index;

void SetThisThreadContext(ID3D11DeviceContext2 *ctx)
{
	TLS_DeviceContext = ctx;
}

void AddThreadTls()
{
	ContextAddrs.insert_or_assign(GetCurrentThreadId(), &TLS_DeviceContext);
	TLS_DeviceContext = lastUpdate;
}

VOID WINAPI tls_callback1(PVOID DllHandle, DWORD Reason, PVOID Reserved)
{
	if (Reason == DLL_THREAD_ATTACH)
	{
		OutputDebugStringA("TLS Callback\n");

		AddThreadTls();
	}
	else if (Reason == DLL_THREAD_DETACH)
	{
		ContextAddrs.erase(GetCurrentThreadId());
	}
}

void UpdateContextTest(ID3D11DeviceContext2 *ctx)
{
	// Update TLS_DeviceContext across all threads
	lastUpdate = ctx;

	for (auto& pair : ContextAddrs)
		*pair.second = lastUpdate;
}

uintptr_t GetTlsOffset(void *Variable)
{
	uintptr_t tlsBase = *(uintptr_t *)(__readgsqword(0x58) + _tls_index * sizeof(void *));
	return (uintptr_t)Variable - tlsBase;
}

#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:p_tls_callback1")
#pragma const_seg(push)
#pragma const_seg(".CRT$XLAAA")
EXTERN_C const PIMAGE_TLS_CALLBACK p_tls_callback1 = tls_callback1;
#pragma const_seg(pop)

// Patch out the default game ID3D11DeviceContext (ImmediateContext) to use a TLS pointer instead
void PatchContexts()
{
	uintptr_t contextAddress = g_ModuleBase + 0x304DEA0;

	// Generate the TLS replacement blocks
#define TLS_CONTEXT_READ_BLOCK(reg) \
	CODE_BLOCK(TLSReplacement_READ_##reg) \
		mov(reg, 0x58); \
		putSeg(gs); \
		mov(reg, ptr [reg]); \
		mov(reg, ptr [reg + _tls_index * sizeof(void *)]); \
		mov(reg, ptr [reg + GetTlsOffset(&TLS_DeviceContext)]); \
		ret(); \
	CODE_BLOCK_END(TLSReplacement_READ_##reg)

	TLS_CONTEXT_READ_BLOCK(rax);
	TLS_CONTEXT_READ_BLOCK(rcx);
	TLS_CONTEXT_READ_BLOCK(rdx);

	CODE_BLOCK(TLSReplacement_WRITE_rax)
		push(rcx);
		push(rax);
		mov(rcx, rax);
		mov(rax, (size_t)&UpdateContextTest);
		call(rax);
		pop(rax);
		pop(rcx);

		//push(rax);
		//push(rax);
		//mov(rax, 0x58);
		//putSeg(gs);
		//mov(rax, ptr [rax]);
		//mov(rax, ptr [rax + _tls_index * sizeof(void *)]);
		//pop(ptr [rcx + GetTlsOffset(&TLS_DeviceContext)]);
		//pop(rax);
		ret();
	CODE_BLOCK_END(TLSReplacement_WRITE_rax)

	// 48 8B 0D 39 DC 2D 02      mov rcx, cs:d3dcontext
	// 48 89 05 61 CB 2D 02      mov cs:d3dcontext, rax
	uintptr_t codeStart = g_CodeBase;
	uintptr_t codeEnd = codeStart + g_CodeSize;

	for (uintptr_t i = codeStart; i < codeEnd; i++)
	{
		// mov rcx, cs:d3dcontext
		if (*(BYTE *)(i + 0) == 0x48 && *(BYTE *)(i + 1) == 0x8B && *(BYTE *)(i + 2) == 0x0D)
		{
			uintptr_t pointsTo = i + *(uint32_t *)(i + 3) + 7;

			if (pointsTo != contextAddress)
				continue;

			BYTE data[7];
			data[0] = 0xE8;
			*(uint32_t *)&data[1] = (uintptr_t)TLSReplacement_READ_rcx.getCode() - i - 5;
			data[5] = 0x90;
			data[6] = 0x90;

			PatchMemory(i, data, sizeof(data));
		}

		// mov rax, cs:d3dcontext
		if (*(BYTE *)(i + 0) == 0x48 && *(BYTE *)(i + 1) == 0x8B && *(BYTE *)(i + 2) == 0x05)
		{
			uintptr_t pointsTo = i + *(uint32_t *)(i + 3) + 7;

			if (pointsTo != contextAddress)
				continue;

			BYTE data[7];
			data[0] = 0xE8;
			*(uint32_t *)&data[1] = (uintptr_t)TLSReplacement_READ_rax.getCode() - i - 5;
			data[5] = 0x90;
			data[6] = 0x90;

			PatchMemory(i, data, sizeof(data));
		}

		// mov rdx, cs:d3dcontext
		if (*(BYTE *)(i + 0) == 0x48 && *(BYTE *)(i + 1) == 0x8B && *(BYTE *)(i + 2) == 0x15)
		{
			uintptr_t pointsTo = i + *(uint32_t *)(i + 3) + 7;

			if (pointsTo != contextAddress)
				continue;

			BYTE data[7];
			data[0] = 0xE8;
			*(uint32_t *)&data[1] = (uintptr_t)TLSReplacement_READ_rdx.getCode() - i - 5;
			data[5] = 0x90;
			data[6] = 0x90;

			PatchMemory(i, data, sizeof(data));
		}

		// mov cs:d3dcontext, rax
		if (*(BYTE *)(i + 0) == 0x48 && *(BYTE *)(i + 1) == 0x89 && *(BYTE *)(i + 2) == 0x05)
		{
			uintptr_t pointsTo = i + *(uint32_t *)(i + 3) + 7;

			if (pointsTo != contextAddress)
				continue;

			BYTE data[7];
			data[0] = 0xE8;
			*(uint32_t *)&data[1] = (uintptr_t)TLSReplacement_WRITE_rax.getCode() - i - 5;
			data[5] = 0x90;
			data[6] = 0x90;

			PatchMemory(i, data, sizeof(data));
		}
	}
}

void PatchReferences()
{
	// Find a region within 2GB (subtract some for tolerance)
	uintptr_t maxDelta = (1ull * 1024 * 1024 * 1024) - 4096;

	uintptr_t start = g_ModuleBase - maxDelta;
	uintptr_t end = g_ModuleBase + maxDelta;

	while (start < end)
	{
		MEMORY_BASIC_INFORMATION memInfo;
		if (VirtualQuery((LPVOID)start, &memInfo, sizeof(memInfo)) == 0)
			break;

		if (memInfo.State == MEM_FREE && memInfo.RegionSize >= 16384)
		{
			g_CodeRegion = (uintptr_t)VirtualAlloc(memInfo.BaseAddress, 16384, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			if (g_CodeRegion)
				break;
		}

		start = (uintptr_t)memInfo.BaseAddress + 4096 + 1;
	}

	if (!g_CodeRegion)
		__debugbreak();

	PatchContexts();
}