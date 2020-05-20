#include "../../common.h"
#include "TESFile_CK.h"
#include "LogWindow.h"

int TESFile::hk_LoadTESInfo()
{
	int error = LoadTESInfo(this);

	if (error != 0)
		return error;

	const bool masterFile = (m_RecordFlags & FILE_RECORD_ESM) == FILE_RECORD_ESM;
	const bool activeFile = (m_RecordFlags & FILE_RECORD_ACTIVE) == FILE_RECORD_ACTIVE;

	// If it's an ESM being loaded as the active file, force it to act like a normal ESP
	if (masterFile && activeFile)
	{
		LogWindow::Log("Loading master file '%s' as a plugin\n", m_FileName);

		// Strip ESM flag, clear loaded ONAM data
		m_RecordFlags &= ~FILE_RECORD_ESM;
		((void(__fastcall *)(TESFile *))OFFSET(0x166CC60, 1530))(this);
	}
	
	// If loading ESP files as masters, flag everything except for the currently active plugin
	if (AllowMasterESP)
	{
		if (!masterFile && !activeFile && (m_RecordFlags & FILE_RECORD_CHECKED) == FILE_RECORD_CHECKED)
		{
			LogWindow::Log("Loading plugin file '%s' as a master\n", m_FileName);
			m_RecordFlags |= FILE_RECORD_ESM;
		}
	}

	return 0;
}

__int64 TESFile::hk_WriteTESInfo()
{
	bool resetEsmFlag = false;

	if ((m_RecordFlags & FILE_RECORD_ACTIVE) == FILE_RECORD_ACTIVE)
	{
		const char *extension = strrchr(m_FileName, '.');

		if (extension && !_stricmp(extension, ".esm"))
		{
			LogWindow::Log("Regenerating ONAM data for master file '%s'...\n", m_FileName);

			((void(__fastcall *)(TESFile *))OFFSET(0x166CCF0, 1530))(this);
			resetEsmFlag = true;
		}
	}

	__int64 form = WriteTESInfo(this);

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