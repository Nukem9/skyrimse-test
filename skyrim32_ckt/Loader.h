#pragma once

#include <windows.h>
#include <stdint.h>
#include <initializer_list>
#include <type_traits>
#include "MemoryModule.h"

namespace Loader
{
	bool Initialize();
	bool MapExecutable();
	void ForceReference();

	HCUSTOMMODULE MmGetLibrary(LPCSTR Name, void *Userdata);
	FARPROC MmGetLibraryProcAddr(HCUSTOMMODULE Module, LPCSTR Name, void *Userdata);
	LPVOID MmMemoryAlloc(LPVOID Address, SIZE_T Size, DWORD AllocationType, DWORD Protect, void *Userdata);

	void PatchMemory(uintptr_t Address, const uint8_t *Data, size_t Size);
	void PatchMemory(uintptr_t Address, std::initializer_list<uint8_t> Data);
	void DetourFunction(uintptr_t Target, uintptr_t Destination, bool Call = false);

	template<typename T>
	void DetourFunction(uintptr_t Target, T Destination, bool Call = false)
	{
		static_assert(std::is_member_function_pointer_v<T> || (std::is_pointer_v<T> && std::is_function_v<typename std::remove_pointer<T>::type>));

		DetourFunction(Target, *reinterpret_cast<uintptr_t *>(&Destination), Call);
	}

	void ForceReference();
}