#pragma once

class TESFile
{
private:
	TESFile();
	~TESFile();

public:
	enum : uint32_t
	{
		FILE_RECORD_ESM = 0x1,
		FILE_RECORD_ACTIVE = 0x8,
		FILE_RECORD_LOCALIZED = 0x80,
		FILE_RECORD_ESL = 0x200,
	};

	char _pad0[0x58];
	char m_FileName[MAX_PATH];
	char m_FilePath[MAX_PATH];
	char _pad1[0x1D8];
	uint32_t m_RecordFlags;

	inline static int (* LoadTESInfo)(TESFile *);
	inline static __int64 (* WriteTESInfo)(TESFile *);

	int hk_LoadTESInfo();
	__int64 hk_WriteTESInfo();
	bool IsActiveFileBlacklist();
};
static_assert_offset(TESFile, m_FileName, 0x58);
static_assert_offset(TESFile, m_FilePath, 0x15C);
static_assert_offset(TESFile, m_RecordFlags, 0x438);