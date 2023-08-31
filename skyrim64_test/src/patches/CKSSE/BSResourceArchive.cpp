#include "..\..\common.h"
#include "BSResourceArchive.h"
#include "LogWindow.h"

void BSResourceArchive::GetFileSizeStr(unsigned int fileSize, BSString& fileSizeStr) {
	if (fileSize >= 0x40000000)
		fileSizeStr.Format("%.3f GByte", ((long double)fileSize) / 0x40000000);
	else if (fileSize >= 0x100000)
		fileSizeStr.Format("%3.3f MByte", ((long double)fileSize) / 0x100000);
	else if (fileSize >= 0x400)
		fileSizeStr.Format("%3.3f KByte", ((long double)fileSize) / 0x400);
	else
		fileSizeStr.Format("%d Byte", fileSize);
}

void* BSResourceArchive::hk_LoadArchive(void* Loose, const char* file_name, void* Unk1, int Unk2)
{
	AssertMsg(file_name, "There is no name of the load archive");

	BSString filePath = BSString::Utils::GetDataPath() + file_name, fileSizeStr;
	//AssertMsgVa(BSString::Utils::FileExists(filePath), "Can't found file %s", file_name);

	//if (!BSString::Utils::FileExists(filePath))
	//	_MESSAGE_FMT("Can't found file \"%s\".", file_name);
	if (BSString::Utils::FileExists(filePath))
	{
		unsigned int fileSize = 0;
		WIN32_FILE_ATTRIBUTE_DATA fileData;
		if (GetFileAttributesExA(*filePath, GetFileExInfoStandard, &fileData))
			fileSize = (uint64_t)fileData.nFileSizeLow | ((uint64_t)fileData.nFileSizeHigh << 32);

		GetFileSizeStr(fileSize, fileSizeStr);
		_MESSAGE_FMT("Load an archive file \"%s\" (%s)...", file_name, *fileSizeStr);
	}

	return OldLoadArchive(Loose, file_name, Unk1, Unk2);
}

void BSResourceArchive::LoadArchive(const char* file_name)
{
	if (BSString::Utils::FileExists(BSString::Utils::GetDataPath() + file_name))
		((void(__fastcall*)(const char* file_name, int Unk1, int Unk2))OFFSET(0x264C6B0, 1573))(file_name, 0, 0);
}