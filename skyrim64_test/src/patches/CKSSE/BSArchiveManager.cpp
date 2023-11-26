#include "BSArchiveManager.h"
#include "BSString.h"
#include "..\..\xutil.h"

static std::vector<BSString*> g_arrayArchivesAvailable;

void BSArchiveManager::Initialize()
{
	auto pathData = BSString::Utils::GetDataPath();

	WIN32_FIND_DATA	FileFindData;
	HANDLE hFindFile = FindFirstFileExA(*(pathData + "*.bsa"), FindExInfoStandard, &FileFindData,
		FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
	if (hFindFile != INVALID_HANDLE_VALUE) {
		do {
			g_arrayArchivesAvailable.push_back(new BSString(FileFindData.cFileName));
		} while (FindNextFile(hFindFile, &FileFindData));
	}

	auto func = [](const std::string& svalue) {
		BSString strName;

		if (svalue.length() > 0) {
			LPSTR s_c = new CHAR[svalue.length() + 1];
			strcpy(s_c, svalue.c_str());

			LPSTR stoken = strtok(s_c, ",");
			if (stoken) {
				do {
					auto index = g_arrayArchivesAvailable.begin();
					auto fname = XUtil::trim(stoken);

					for (; index != g_arrayArchivesAvailable.end(); index++)
					{
						if (!_stricmp(fname.c_str(), (*index)->c_str()))
							break;
					}

					if (index != g_arrayArchivesAvailable.end()) {
						delete *index;
						g_arrayArchivesAvailable.erase(index);
					}

					stoken = strtok(NULL, ",");
				} while (stoken);
			}

			delete[] s_c;
		}
	};

	static const char* SC_NONE = "<NONE>";

	auto s = g_INI_ck_User_conf.Get("Archive", "sResourceArchiveList", SC_NONE);
	func((s == SC_NONE) ? g_INI_ck_conf.Get("Archive", "sResourceArchiveList", "") : s);
	s = g_INI_ck_User_conf.Get("Archive", "sResourceArchiveList2", SC_NONE);
	func((s == SC_NONE) ? g_INI_ck_conf.Get("Archive", "sResourceArchiveList2", "") : s);
	s = g_INI_ck_User_conf.Get("Archive", "sResourceArchiveMemoryCacheList", SC_NONE);
	func((s == SC_NONE) ? g_INI_ck_conf.Get("Archive", "sResourceArchiveMemoryCacheList", "") : s);
	s = g_INI_ck_User_conf.Get("Archive", "sResourceStartUpArchiveList", SC_NONE);
	func((s == SC_NONE) ? g_INI_ck_conf.Get("Archive", "sResourceStartUpArchiveList", "") : s);
	s = g_INI_ck_User_conf.Get("Archive", "sResourceIndexFileList", SC_NONE);
	func((s == SC_NONE) ? g_INI_ck_conf.Get("Archive", "sResourceIndexFileList", "") : s);
}

bool BSArchiveManager::IsAvailableForLoad(LPCSTR ArchiveName)
{
	auto index = g_arrayArchivesAvailable.begin();
	auto fname = XUtil::trim(ArchiveName);

	for (; index != g_arrayArchivesAvailable.end(); index++)
	{
		if (!_stricmp(fname.c_str(), (*index)->c_str()))
			break;
	}

	return index != g_arrayArchivesAvailable.end();
}
