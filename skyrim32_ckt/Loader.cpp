#include "stb.h"
#include "resource.h"
#include "CreationKit.h"
#include "LipSynchAnim.h"
#include "Loader.h"

//
// Relocations aren't supported. To work around this, force this executable to be loaded at a base address of 0x10000. The CK.exe will be emulated 
// at a base address of 0x400000.
//
#pragma comment(linker, "/BASE:0x10000")
#pragma bss_seg(push)
#pragma bss_seg(".CKXCODE")
volatile char FORCE_LOAD_REQUIRED_DATA[0x1FC9000];
#pragma bss_seg(pop)

namespace Loader
{
	const static auto BAD_DLL = reinterpret_cast<HCUSTOMMODULE>(0xDEADBEEF);
	const static auto BAD_IMPORT = reinterpret_cast<FARPROC>(0xFEFEDEDE);

	bool Initialize()
	{
		if (!MapExecutable())
			return false;

		// Allocate arbitrary TLS chunk
		int tlsIndex = *(int *)0x1E97DC0;
		*(void **)(__readfsdword(0x2C) + 4 * tlsIndex) = VirtualAlloc(nullptr, 64 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		// Kill MemoryContextTracker ctor/dtor
		PatchMemory(0x948500, { 0xC2, 0x10, 0x00 });
		PatchMemory(0x948560, { 0xC3 });
		PatchMemory(0x9484E9, { 0x5E, 0xC3 });

		// Replace memory allocators with malloc and kill initializers
		PatchMemory(0x4010A0, { 0xC3 });
		DetourFunction(0x947D20, &CreationKit::MemoryManager_Alloc);
		DetourFunction(0x947320, &CreationKit::MemoryManager_Free);
		DetourFunction(0x4014D0, &CreationKit::ScrapHeap_Alloc);
		DetourFunction(0x931990, &CreationKit::ScrapHeap_Free);

		// Patch BSOStream::Write in LipSynchAnim::SaveToFile to use our own file handling
		PatchMemory(0x587810, { 0x90, 0x90, 0x90 });// Kill +0x4 pointer adjustment
		DetourFunction(0x587816, &LipSynchAnim::hk_call_00587816, true);
		DetourFunction(0x58781F, &LipSynchAnim::hk_call_00587816, true);

		// Add logging
		((int(*)(void *))(0x8BF320))(&CreationKit::FaceFXLogCallback);
		DetourFunction(0x40AFC0, &CreationKit::LogCallback);

		// Patch WinMain in order to only run CRT initialization
		PatchMemory(0x48E8B0, { 0xC2, 0x10, 0x00 });
		PatchMemory(0xE84A16, { 0xEB, 0x0E });
		((void(__cdecl *)())(0xE84A7B))();

		CoInitializeEx(nullptr, 0);				// COM components
		((void(*)())(0x934B90))();				// BSResource (filesystem)
		((void(*)())(0x469FE0))();				// LipSynchManager::Init()
		PatchMemory(0x46AA59, { 0x90, 0x90 });	// Required to force update FonixData.cdf path in TLS

		return true;
	}

	bool MapExecutable()
	{
		ForceReference();

		HMODULE module = GetModuleHandleA(nullptr);

		// Decompress the embedded exe and then initialize it
		HRSRC resource = FindResourceW(module, MAKEINTRESOURCE(IDR_CK_LIP_BINARY1), L"CK_LIP_BINARY");
		uint32_t resourceSize = SizeofResource(module, resource);

		if (!resource || resourceSize <= 0)
		{
			printf("Failed to find embedded CK resource\n");
			return false;
		}

		auto data = reinterpret_cast<unsigned char *>(LockResource(LoadResource(module, resource)));
		auto exeData = new unsigned char[stb_decompress_length(data)];

		if (stb_decompress(exeData, data, resourceSize) == 0)
		{
			printf("CK resource decompression failure\n");
			return false;
		}

		HMEMORYMODULE creationKitExe = MemoryLoadLibraryEx(exeData, stb_decompress_length(data), MmMemoryAlloc, MemoryDefaultFree, MmGetLibrary, MmGetLibraryProcAddr, MemoryDefaultFreeLibrary, nullptr);
		delete[] exeData;

		if (!creationKitExe)
		{
			printf("Failed to map CK executable into memory\n");
			return false;
		}

		return true;
	}

	void ForceReference()
	{
		FORCE_LOAD_REQUIRED_DATA[0] = 0;
		FORCE_LOAD_REQUIRED_DATA[sizeof(FORCE_LOAD_REQUIRED_DATA) - 1] = 0;
	}

	HCUSTOMMODULE MmGetLibrary(LPCSTR Name, void *Userdata)
	{
		static const char *moduleBlacklist[] =
		{
			"SSCE5432.DLL",
			"D3D9.DLL",
			"DSOUND.DLL",
			"STEAM_API.DLL",
			"X3DAUDIO1_7.DLL",
			"DBGHELP.DLL",
			"D3DX9_42.DLL",
			"XINPUT1_3.DLL",
		};

		// Check for blacklisted DLLs first
		for (const char *mod : moduleBlacklist)
		{
			if (!_stricmp(Name, mod))
				return BAD_DLL;
		}

		return reinterpret_cast<HCUSTOMMODULE>(LoadLibraryA(Name));
	}

	FARPROC MmGetLibraryProcAddr(HCUSTOMMODULE Module, LPCSTR Name, void *Userdata)
	{
		if (Module == BAD_DLL)
			return BAD_IMPORT;

		return MemoryDefaultGetProcAddress(Module, Name, Userdata);
	}

	LPVOID MmMemoryAlloc(LPVOID Address, SIZE_T Size, DWORD AllocationType, DWORD Protect, void *Userdata)
	{
		auto addr = reinterpret_cast<uintptr_t>(Address);

		if (addr >= 0x400000 && (addr + Size) <= 0x1FC9000)
		{
			auto minAddr = reinterpret_cast<uintptr_t>(&FORCE_LOAD_REQUIRED_DATA[0]);
			auto maxAddr = reinterpret_cast<uintptr_t>(&FORCE_LOAD_REQUIRED_DATA[sizeof(FORCE_LOAD_REQUIRED_DATA)]);

			// Sanity check
			if (addr < minAddr)
				__debugbreak();

			if ((addr + Size) > maxAddr)
				__debugbreak();

			DWORD old;
			VirtualProtect(Address, Size, Protect, &old);

			return Address;
		}

		return VirtualAlloc(Address, Size, AllocationType, Protect);
	}

	void PatchMemory(uintptr_t Address, const uint8_t *Data, size_t Size)
	{
		auto asPointer = reinterpret_cast<void *>(Address);
		DWORD protect = 0;

		VirtualProtect(asPointer, Size, PAGE_EXECUTE_READWRITE, &protect);

		for (uintptr_t i = Address; i < (Address + Size); i++)
			*reinterpret_cast<uint8_t *>(i) = *Data++;

		VirtualProtect(asPointer, Size, protect, &protect);
		FlushInstructionCache(GetCurrentProcess(), asPointer, Size);
	}

	void PatchMemory(uintptr_t Address, std::initializer_list<uint8_t> Data)
	{
		PatchMemory(Address, Data.begin(), Data.size());
	}

	void DetourFunction(uintptr_t Target, uintptr_t Destination, bool Call)
	{
		uint8_t data[5];

		data[0] = Call ? 0xE8 : 0xE9;
		*reinterpret_cast<int32_t *>(&data[1]) = static_cast<int32_t>(Destination - Target) - 5;

		PatchMemory(Target, data, sizeof(data));
	}
}