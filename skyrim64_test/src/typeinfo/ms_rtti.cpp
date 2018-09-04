#include "../common.h"
#include "ms_rtti.h"

extern "C"
{
	typedef void* (*malloc_func_t)(size_t);
	typedef void(*free_func_t)(void*);
	char *__unDNameEx(char *outputString, const char *name, int maxStringLength, malloc_func_t pAlloc, free_func_t pFree, char *(__fastcall *pGetParameter)(int), unsigned int disableFlags);
}

namespace MSRTTI
{
	using namespace detail;

	uintptr_t g_RdataStart;	// .rdata
	uintptr_t g_RdataEnd;
	uintptr_t g_DataStart;	// .data
	uintptr_t g_DataEnd;
	uintptr_t g_CodeStart;	// .text
	uintptr_t g_CodeEnd;

	std::vector<Info> Tables;

	void Initialize()
	{
		//g_RdataStart = ::g_ModuleBase + 0x15231F0;
		//g_RdataEnd = ::g_ModuleBase + 0x1DD4448;

		//g_DataStart = ::g_ModuleBase + 0x1DD5000;
		//g_DataEnd = g_DataStart + 0x16E8000;

		g_RdataStart = ::g_ModuleBase + 0x3021000;
		g_RdataEnd = ::g_ModuleBase + 0x38EAFFF;

		g_DataStart = ::g_ModuleBase + 0x38EB000;
		g_DataEnd = ::g_ModuleBase + 0x5A13FFF;

		g_CodeStart = ::g_CodeBase;
		g_CodeEnd = g_CodeStart + ::g_CodeSize;

		for (uintptr_t i = g_RdataStart; i < (g_RdataEnd - sizeof(uintptr_t) - sizeof(uintptr_t)); i++)
		{
			// Skip all non-2-aligned addresses. Not sure if this is OK or it skips tables.
			if (i % 2 != 0)
				continue;

			//
			// This might be a valid RTTI entry, so check if:
			// - The COL points to somewhere in .rdata
			// - The COL has a valid signature
			// - The first virtual function points to .text
			//
			uintptr_t addr = *(uintptr_t *)i;
			uintptr_t vfuncAddr = *(uintptr_t *)(i + sizeof(uintptr_t));

			if (!IsWithinRDATA(addr) || !IsWithinCODE(vfuncAddr))
				continue;

			auto locator = reinterpret_cast<CompleteObjectLocator *>(addr);

			if (!IsValidCOL(locator))
				continue;

			Info info;
			info.VTableAddress = i + sizeof(uintptr_t);
			info.VTableOffset = locator->Offset;
			info.VFunctionCount = 0;
			info.RawName = locator->TypeDescriptor.Get()->name;
			info.Locator = locator;

			// Demangle
			info.Name = __unDNameEx(nullptr, info.RawName + 1, 0, malloc, free, nullptr, 0x2800);

			// Determine number of virtual functions
			for (uintptr_t j = info.VTableAddress; j < (g_RdataEnd - sizeof(uintptr_t)); j += sizeof(uintptr_t))
			{
				if (!IsWithinCODE(*(uintptr_t *)j))
					break;

				info.VFunctionCount++;
			}

			// Done
			Tables.push_back(info);
		}
	}

	void Dump(FILE *File)
	{
		for (const Info& info : Tables)
			fprintf(File, "`%s`: VTable [0x%p, 0x%p offset, %lld functions] `%s`\n", info.Name, info.VTableAddress - g_ModuleBase, info.VTableOffset, info.VFunctionCount, info.RawName);
	}

	const Info *Find(const char *Name, bool Exact)
	{
		auto results = FindAll(Name, Exact);
		AssertMsgDebug(results.size() == 1, "Had no results or had more than 1 result");

		return results.at(0);
	}

	std::vector<const Info *> FindAll(const char *Name, bool Exact)
	{
		// Multiple classes can have identical names but different vtable displacements,
		// so return all that match
		std::vector<const Info *> results;

		for (const Info& info : Tables)
		{
			if (Exact)
			{
				if (!strcmp(info.Name, Name))
					results.push_back(&info);
			}
			else
			{
				if (strcasestr(info.Name, Name))
					results.push_back(&info);
			}
		}

		return results;
	}

	namespace detail
	{
		bool IsWithinRDATA(uintptr_t Address)
		{
			return (Address >= g_RdataStart && Address <= g_RdataEnd) ||
				(Address >= g_DataStart && Address <= g_DataEnd);
		}

		bool IsWithinCODE(uintptr_t Address)
		{
			return Address >= g_CodeStart && Address <= g_CodeEnd;
		}

		bool IsValidCOL(CompleteObjectLocator *Locator)
		{
			return Locator->Signature == CompleteObjectLocator::COL_Signature64 && IsWithinRDATA(Locator->TypeDescriptor.Address());
		}

		const char *strcasestr(const char *String, const char *Substring)
		{
			const char *a, *b;

			for (; *String; *String++)
			{
				a = String;
				b = Substring;

				while (toupper(*a++) == toupper(*b++))
					if (!*b)
						return String;
			}

			return nullptr;
		}
	}
}