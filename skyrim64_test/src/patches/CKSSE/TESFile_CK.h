//////////////////////////////////////////
/*
* Copyright (c) 2020 Nukem9 <email:Nukem@outlook.com>
* Copyright (c) 2022 Perchik71 <email:perchik71@outlook.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this
* software and associated documentation files (the "Software"), to deal in the Software
* without restriction, including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or
* substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
* PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
* FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
* OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
//////////////////////////////////////////

#pragma once

#include <stdint.h>

struct TESChunk_CK {
	char type[4];
	uint32_t size;
	uint32_t flags;
	uint32_t identifier;
	uint16_t timestamp;
	uint16_t vercontrol;
	uint16_t internalver;
	uint16_t unk;
};

class TESFile_CK
{
private:
	TESFile_CK();
	~TESFile_CK();

public:
	enum : uint32_t
	{
		FILE_RECORD_ESM = 0x1,			// Master plugin
		FILE_RECORD_CHECKED = 0x4,		// Pending load/loaded
		FILE_RECORD_ACTIVE = 0x8,		// Save target
		FILE_RECORD_LOCALIZED = 0x80,	// Strings removed
		FILE_RECORD_ESL = 0x200,		// Small file
	};

	char _pad0[0x58];
	char m_FileName[MAX_PATH];
	char m_FilePath[MAX_PATH];
	char _pad1[0x8];
	uint32_t m_bufsize;
	char _pad2[0x1CC];
	uint32_t m_RecordFlags;

	inline static int (* LoadTESInfo)(TESFile_CK *);
	inline static __int64 (* WriteTESInfo)(TESFile_CK *);
	inline static bool AllowSaveESM;
	inline static bool AllowMasterESP;

	int hk_LoadTESInfo();
	__int64 hk_WriteTESInfo();
	bool IsActiveFileBlacklist();

	uint32_t GetIndexLoader() const;
public:
	static bool ReadFirstChunk(const char* fileName, TESChunk_CK& chunk);
	static uint32_t GetTypeFile(const char* fileName);
};
static_assert_offset(TESFile_CK, m_FileName, 0x58);
static_assert_offset(TESFile_CK, m_FilePath, 0x15C);
static_assert_offset(TESFile_CK, m_RecordFlags, 0x438);