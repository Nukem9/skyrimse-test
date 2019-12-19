#include "../../common.h"
#include "TESFile_CK.h"
#include "LogWindow.h"

int TESFile::hk_LoadPluginHeader()
{
	int error = LoadPluginHeader(this);

	if (error != 0)
		return error;

	// If the file is an ESM being loaded as an active file, pretend it's a normal ESP
	if ((m_RecordFlags & (FILE_RECORD_ESM | FILE_RECORD_ACTIVE)) == (FILE_RECORD_ESM | FILE_RECORD_ACTIVE))
	{
		EditorUI_Log("Loading master file '%s' as a plugin...\n", m_FileName);

		// Strip ESM flag, clear loaded ONAM data
		m_RecordFlags &= ~FILE_RECORD_ESM;
		((void(__fastcall *)(TESFile *))OFFSET(0x166CC60, 1530))(this);
	}

	return error;
}

__int64 TESFile::hk_WritePluginHeader()
{
	bool resetEsmFlag = false;

	if ((m_RecordFlags & FILE_RECORD_ACTIVE) == FILE_RECORD_ACTIVE)
	{
		const char *extension = strrchr(m_FileName, '.');

		if (extension && !_stricmp(extension, ".esm"))
		{
			EditorUI_Log("Regenerating ONAM data for master file '%s'...\n", m_FileName);

			((void(__fastcall *)(TESFile *))OFFSET(0x166CCF0, 1530))(this);
			resetEsmFlag = true;
		}
	}

	__int64 form = WritePluginHeader(this);

	if (resetEsmFlag)
		m_RecordFlags &= ~FILE_RECORD_ESM;

	return form;
}

bool TESFile::IsActiveFileBlacklist()
{
	if ((m_RecordFlags & FILE_RECORD_ESM) == FILE_RECORD_ESM)
	{
		if (!_stricmp(m_FileName, "Skyrim.esm") ||
			!_stricmp(m_FileName, "Update.esm") ||
			!_stricmp(m_FileName, "Dawnguard.esm") ||
			!_stricmp(m_FileName, "HearthFires.esm") ||
			!_stricmp(m_FileName, "Dragonborn.esm"))
		{
			MessageBoxA(GetForegroundWindow(), "Base game master files cannot be set as the active file.", "Warning", MB_ICONWARNING);
			return true;
		}
	}

	return false;
}