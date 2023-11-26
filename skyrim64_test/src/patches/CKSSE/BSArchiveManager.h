#pragma once

#include "..\..\common.h"

class BSArchiveManager
{
public:
	static void Initialize();
	static bool IsAvailableForLoad(LPCSTR ArchiveName);
};