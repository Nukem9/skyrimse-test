#include "../../common.h"
#include "TESFile_CK.h"
#include "LogWindow.h"
#include "BSString.h"

#include <fstream>

int TESFile_CK::hk_LoadTESInfo()
{
	int error = LoadTESInfo(this);

	if (error != 0)
		return error;

	const bool masterFile = (m_RecordFlags & FILE_RECORD_ESM) == FILE_RECORD_ESM;
	const bool activeFile = (m_RecordFlags & FILE_RECORD_ACTIVE) == FILE_RECORD_ACTIVE;

	// If it's an ESM being loaded as the active file, force it to act like a normal ESP
	if (AllowSaveESM)
	{
		if (masterFile && activeFile)
		{
			LogWindow::Log("Loading master file '%s' as a plugin\n", m_FileName);

			// Strip ESM flag, clear loaded ONAM data
			m_RecordFlags &= ~FILE_RECORD_ESM;
			((void(__fastcall*)(TESFile_CK*))OFFSET(0x166CC60, 1530))(this);
		}
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

__int64 TESFile_CK::hk_WriteTESInfo()
{
	bool resetEsmFlag = false;

	if (AllowSaveESM)
	{
		if ((m_RecordFlags & FILE_RECORD_ACTIVE) == FILE_RECORD_ACTIVE)
		{
			const char *extension = strrchr(m_FileName, '.');

			if (extension && !_stricmp(extension, ".esm"))
			{
				LogWindow::Log("Regenerating ONAM data for master file '%s'...\n", m_FileName);

				((void(__fastcall*)(TESFile_CK*))OFFSET(0x166CCF0, 1530))(this);
				resetEsmFlag = true;
			}
		}
	}

	auto form = WriteTESInfo(this);

	if (resetEsmFlag)
		m_RecordFlags &= ~FILE_RECORD_ESM;

	return form;
}

bool TESFile_CK::IsActiveFileBlacklist()
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

	/*auto type = TESFile_CK::GetTypeFile((BSString::Utils::GetRelativeDataPath() + m_FileName).Get());
	if ((type & TESFile_CK::FILE_RECORD_ESL) == TESFile_CK::FILE_RECORD_ESL) {
		MessageBoxA(GetForegroundWindow(), 
			"Light master files cannot to be open normally.\n"
			"The probability of getting CTD is very high.\n", 
			"Warning", MB_ICONWARNING);
	}*/

	return false;
}

bool TESFile_CK::ReadFirstChunk(const char* fileName, TESChunk_CK& chunk) {
	std::ifstream ifs(fileName, std::ios::binary);
	if (!ifs.good())
		return false;
	
	ifs.read(reinterpret_cast<char*>(&chunk), sizeof(TESChunk_CK));
	ifs.close();

	return true;
}

uint32_t TESFile_CK::GetTypeFile(const char* fileName) {
	TESChunk_CK chunk;
	if (ReadFirstChunk(fileName, chunk))
		return chunk.flags;
	return 0;
}

uint32_t TESFile_CK::GetIndexLoader() const {
	return ((uint32_t(__fastcall*)(const TESFile_CK*))OFFSET(0x15C1630, 16438))(this);
}