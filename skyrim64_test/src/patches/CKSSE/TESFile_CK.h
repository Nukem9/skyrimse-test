#pragma once

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
	char _pad1[0x1D8];
	uint32_t m_RecordFlags;

	inline static int (* LoadTESInfo)(TESFile_CK *);
	inline static __int64 (* WriteTESInfo)(TESFile_CK *);
	inline static bool AllowSaveESM;
	inline static bool AllowMasterESP;

	int hk_LoadTESInfo();
	__int64 hk_WriteTESInfo();
	bool IsActiveFileBlacklist();
};
static_assert_offset(TESFile_CK, m_FileName, 0x58);
static_assert_offset(TESFile_CK, m_FilePath, 0x15C);
static_assert_offset(TESFile_CK, m_RecordFlags, 0x438);