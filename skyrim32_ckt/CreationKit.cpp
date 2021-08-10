#include <stdio.h>
#include "CreationKit.h"

namespace CreationKit
{
	void SetFaceFXDataPath(const char *Path)
	{
		static char tempPath[1024];
		static bool init = [&]()
		{
			// Manually patch Fonix.cdf lookup path - alternate global variable
			auto fonixPathPtr = reinterpret_cast<uintptr_t>(&tempPath);
			Loader::PatchMemory(0x469CA8, reinterpret_cast<uint8_t *>(&fonixPathPtr), sizeof(fonixPathPtr));

			return true;
		}();

		// "C:\Directory\My\FonixData.cdf"
		strcpy_s(tempPath, Path);
	}

	void SetFaceFXLanguage(const char *Language)
	{
		static char tempLanguage[128];
		static bool init = [&]()
		{
			// Manually patch language - alternate global variable
			auto languagePtr = reinterpret_cast<uintptr_t>(&tempLanguage);
			Loader::PatchMemory(0x11B0AEC, reinterpret_cast<uint8_t *>(&languagePtr), sizeof(languagePtr));

			return true;
		}();

		// "USEnglish"
		strcpy_s(tempLanguage, Language);
	}

	void FaceFXLogCallback(const char *Text, int Type)
	{
		printf("[FaceFX %02d]: %s\n", Type, Text);
	}

	void LogCallback(int Type, const char *Format, ...)
	{
		char buffer[2048];
		va_list va;

		va_start(va, Format);
		_vsnprintf_s(buffer, _TRUNCATE, Format, va);
		va_end(va);

		printf("[CKIT32 %02d]: %s\n", Type, buffer);
	}

	void *__fastcall MemoryManager_Alloc(void *Thisptr, void *_EDX, uint32_t Size, uint32_t Alignment, bool Aligned)
	{
		if (Size <= 0)
			Size = 1;

		void *data = malloc(Size);
		return memset(data, 0, Size);
	}

	void __fastcall MemoryManager_Free(void *Thisptr, void *_EDX, void *Ptr, bool Aligned)
	{
		if (!Ptr)
			return;

		free(Ptr);
	}

	void *ScrapHeap_Alloc(uint32_t Size)
	{
		return MemoryManager_Alloc(nullptr, nullptr, Size, 0, false);
	}

	void ScrapHeap_Free(void *Ptr)
	{
		MemoryManager_Free(nullptr, nullptr, Ptr, false);
	}
}