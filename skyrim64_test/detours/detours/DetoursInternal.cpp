#include "stdafx.h"

namespace Detours::Internal
{
	uint32_t GlobalOptions;

	void SetGlobalOptions(uint32_t Options)
	{
		LockExchange32((volatile LONG *)&GlobalOptions, Options & OPT_MASK);
	}

	uint32_t GetGlobalOptions()
	{
		return GlobalOptions;
	}

	uint8_t *AlignAddress(uint64_t Address, uint8_t Align)
	{
		if (Address % Align != 0)
			Address += Align - Address % 8;

		return (uint8_t *)Address;
	}

	bool AtomicCopy4X8(uint8_t *Target, uint8_t *Memory, sizeptr_t Length)
	{
		// Buffer to hold temporary opcodes
		char buffer[8];

		// Only 4/8byte sizes supported
		if(Length > sizeof(buffer))
			return false;

		DWORD dwOld = 0;
		if (!VirtualProtect(Target, Length, PAGE_EXECUTE_READWRITE, &dwOld))
			return false;

		if(Length <= 4)
		{
			// Copy the old data
			memcpy(&buffer, Target, 4);

			// Rewrite it with the new data
			memcpy(&buffer, Memory, Length);

			// Write all 4 bytes at once
			LockExchange32((volatile LONG *)Target, *(LONG *)&buffer);
		}
		else if(Length <= 8)
		{
			// Copy the old data
			memcpy(&buffer, Target, 8);

			// Rewrite it with the new data
			memcpy(&buffer, Memory, Length);

			// Write all 8 bytes at once
			LockExchange64((volatile LONGLONG *)Target, *(LONGLONG *)&buffer);
		}

		// Ignore if this fails, the memory was copied either way
		VirtualProtect(Target, Length, dwOld, &dwOld);

		return true;
	}

	bool WriteMemory(uint8_t *Target, uint8_t *Memory, sizeptr_t Length)
	{
		DWORD dwOld = 0;
		if (!VirtualProtect(Target, Length, PAGE_EXECUTE_READWRITE, &dwOld))
			return false;

		memcpy(Target, Memory, Length);

		// Ignore if this fails, the memory was copied either way
		VirtualProtect(Target, Length, dwOld, &dwOld);

		return true;
	}

	bool FlushCache(uint8_t *Target, sizeptr_t Length)
	{
		return FlushInstructionCache(GetCurrentProcess(), Target, Length) != FALSE;
	}

	uint8_t *IATHook(uint8_t *Module, const char *ImportModule, const char *API, uint8_t *Detour)
	{
		// Validate DOS Header
		ULONG_PTR moduleBase		= (ULONG_PTR)Module;
		PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)moduleBase;

		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return nullptr;

		// Validate PE Header and (64-bit|32-bit) module type
		PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(moduleBase + dosHeader->e_lfanew);

		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
			return nullptr;

		if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
			return nullptr;

		// Get the load configuration section which holds the security cookie address
		auto dataDirectory	= ntHeaders->OptionalHeader.DataDirectory;
		DWORD sectionRVA	= dataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		DWORD sectionSize	= dataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

		if (sectionRVA == 0 || sectionSize == 0)
			return nullptr;

		// https://jpassing.com/2008/01/06/using-import-address-table-hooking-for-testing/
		// https://llvm.org/svn/llvm-project/compiler-rt/trunk/lib/interception/interception_win.cc
		//
		// Iterate over each import descriptor
		auto importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(moduleBase + sectionRVA);

		for (size_t i = 0; importDescriptor[i].Name != 0; i++)
		{
			PSTR dllName = (PSTR)(moduleBase + importDescriptor[i].Name);

			// Is this the specific module the user wants?
			if (!_stricmp(dllName, ImportModule))
			{
				if (!importDescriptor[i].FirstThunk)
					return false;

				auto nameTable		= (PIMAGE_THUNK_DATA)(moduleBase + importDescriptor[i].OriginalFirstThunk);
				auto importTable	= (PIMAGE_THUNK_DATA)(moduleBase + importDescriptor[i].FirstThunk);

				for (; nameTable->u1.Ordinal != 0; ++nameTable, ++importTable)
				{
					if (!IMAGE_SNAP_BY_ORDINAL(nameTable->u1.Ordinal))
					{
						auto importName = (PIMAGE_IMPORT_BY_NAME)(moduleBase + nameTable->u1.ForwarderString);
						auto funcName	= (const char *)&importName->Name[0];

						// If this is the function name we want, hook it
						if (!strcmp(funcName, API))
						{
							uint8_t *original	= (uint8_t *)importTable->u1.AddressOfData;
							uint8_t *newPointer = Detour;

							// Swap the pointer atomically
							if (!AtomicCopy4X8((uint8_t *)&importTable->u1.AddressOfData, (uint8_t *)&newPointer, sizeof(importTable->u1.AddressOfData)))
								return nullptr;

							// Done
							return original;
						}
					}
				}
			}
		}

		// API or module name wasn't found
		return nullptr;
	}
}