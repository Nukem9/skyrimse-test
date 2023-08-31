#pragma once

#include "BSString.h"

class BSResourceArchive
{
public:
	typedef void*(*eventLoadArchive_t)(void* Loose, const char* file_name, void* Unk1, int Unk2);
	inline static eventLoadArchive_t OldLoadArchive;

	static void GetFileSizeStr(unsigned int fileSize, BSString& fileSizeStr);
	static void* hk_LoadArchive(void* Loose, const char* file_name, void* Unk1, int Unk2 = 1);
	static void LoadArchive(const char* file_name);
};