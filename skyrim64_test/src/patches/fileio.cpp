#include "../../../tbb2018/concurrent_hash_map.h"
#include "../common.h"

struct MMapFileInfo
{
	HANDLE FileHandle;
	HANDLE MapHandle;
	void *MapBase;
	uint64_t FilePosition;
	uint64_t FileMaxPosition;

	bool IsMMap()
	{
		return MapHandle != nullptr;
	}

	uint64_t ReadMapped(void *Buffer, size_t Size)
	{
		if (FilePosition + Size > FileMaxPosition)
			Size = FileMaxPosition - FilePosition + 1;

		memcpy(Buffer, (void *)((uintptr_t)MapBase + FilePosition), Size);

		FilePosition += Size;

		LARGE_INTEGER pos;
		pos.QuadPart = FilePosition;
		Assert(SetFilePointerEx(FileHandle, pos, nullptr, FILE_BEGIN));

		return Size;
	}

	uint64_t Read(void *Buffer, size_t Size)
	{
		AssertDebug(Size < ULONG_MAX);

		if (IsMMap())
			return ReadMapped(Buffer, Size);

		DWORD bytesRead = 0;

		if (ReadFile(FileHandle, Buffer, (DWORD)Size, &bytesRead, nullptr))
			return bytesRead;

		return UINT64_MAX;
	}

	uint64_t Write(const void *Buffer, size_t Size)
	{
		AssertDebug(Size < ULONG_MAX);

		DWORD bytesWritten = 0;

		if (WriteFile(FileHandle, Buffer, (DWORD)Size, &bytesWritten, nullptr))
			return bytesWritten;

		return UINT64_MAX;
	}

	bool SetFilePointer(int64_t Offset, int64_t *NewPosition, uint32_t Method)
	{
		switch (Method)
		{
		case SEEK_SET: Method = FILE_BEGIN; break;
		case SEEK_CUR: Method = FILE_CURRENT; break;
		case SEEK_END: Method = FILE_END; break;

		default:
			Assert(false);
			return false;
		}

		LARGE_INTEGER move;
		LARGE_INTEGER position;
		move.QuadPart = Offset;

		if (SetFilePointerEx(FileHandle, move, &position, Method))
		{
			if (NewPosition)
				*NewPosition = position.QuadPart;

			FilePosition = position.QuadPart;
			return true;
		}

		return false;
	}

	bool Flush()
	{
		if (IsMMap())
			return FlushViewOfFile(MapBase, 0) != FALSE;

		return FlushFileBuffers(FileHandle) != FALSE;
	}
};

tbb::concurrent_hash_map<HANDLE, MMapFileInfo *> g_FileMap;

MMapFileInfo *GetFileMMap(HANDLE Input)
{
	MMapFileInfo *info = nullptr;

	AssertMsg(((uintptr_t)Input & 0b11) == 0, "Unexpected bits set");
	Assert(Input);

	// If this entry wasn't present already, create a new mapping
	tbb::concurrent_hash_map<HANDLE, MMapFileInfo *>::accessor accessor;

	if (!g_FileMap.find(accessor, Input))
	{
		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(Input, &fileSize))
			Assert(false);

		info = new MMapFileInfo;
		info->FileHandle = Input;
		info->FilePosition = 0;
		info->FileMaxPosition = fileSize.QuadPart - 1;

		if (fileSize.QuadPart <= 4096)
		{
			info->MapHandle = nullptr;
			info->MapBase = nullptr;
		}
		else
		{
			// Map the entire file into memory all at once
			info->MapHandle = CreateFileMapping(info->FileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
			info->MapBase = MapViewOfFile(info->MapHandle, FILE_MAP_READ, 0, 0, 0);

			if (!info->MapHandle || !info->MapBase)
				Assert(false);
		}

		g_FileMap.emplace(Input, info);
	}
	else
	{
		// Found, already initialized
		info = accessor->second;
	}

	return info;
}

#define GET_HANDLE_OVERRIDE(x) (((uintptr_t)(x) & 0b11) == 0b11)

MMapFileInfo *GetStdioFileMap(FILE *Input)
{
	if (!GET_HANDLE_OVERRIDE(Input))
		return nullptr;

	HANDLE temp = (HANDLE)((uintptr_t)Input & ~0b11);
	return GetFileMMap(temp);
}

FILE *RegisterFileHandle(HANDLE Input)
{
	AssertMsg(((uintptr_t)Input & 0b11) == 0, "Unexpected bits set");

	FILE *temp = (FILE *)((uintptr_t)Input | 0b11);
	GetStdioFileMap(temp);
	return temp;
}

BOOL WINAPI hk_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	auto info = GetFileMMap(hFile);
	uint64_t bytesRead = info->Read(lpBuffer, nNumberOfBytesToRead);

	if (bytesRead != UINT64_MAX)
	{
		*lpNumberOfBytesRead = (DWORD)bytesRead;
		return TRUE;
	}

	return FALSE;
}

BOOL WINAPI hk_ReadFileEx(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	auto info = GetFileMMap(hFile);

	if (info->IsMMap())
	{
		uint64_t bytesRead = info->ReadMapped(lpBuffer, nNumberOfBytesToRead);
		lpCompletionRoutine(0, (DWORD)bytesRead, lpOverlapped);
		return TRUE;
	}

	// Fallback
	return ReadFileEx(hFile, lpBuffer, nNumberOfBytesToRead, lpOverlapped, lpCompletionRoutine);
}

BOOL WINAPI hk_SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
	switch (dwMoveMethod)
	{
	case FILE_BEGIN: dwMoveMethod = SEEK_SET; break;
	case FILE_CURRENT: dwMoveMethod = SEEK_CUR; break;
	case FILE_END: dwMoveMethod = SEEK_END; break;
	}

	auto info = GetFileMMap(hFile);

	if (info->SetFilePointer(liDistanceToMove.QuadPart, &lpNewFilePointer->QuadPart, dwMoveMethod))
		return TRUE;

	return FALSE;
}

BOOL WINAPI hk_CloseHandle(HANDLE Input)
{
	tbb::concurrent_hash_map<HANDLE, MMapFileInfo *>::accessor accessor;

	if (g_FileMap.find(accessor, Input))
	{
		auto *info = accessor->second;

		if (info->IsMMap())
		{
			UnmapViewOfFile(info->MapBase);
			CloseHandle(info->MapHandle);
		}

		g_FileMap.erase(accessor);
		delete info;
	}

	return CloseHandle(Input);
}

decltype(&fopen_s) VC140_fopen_s;
decltype(&_wfopen_s) VC140_wfopen_s;
decltype(&fopen) VC140_fopen;
decltype(&fclose) VC140_fclose;
decltype(&fread) VC140_fread;
decltype(&fwrite) VC140_fwrite;
decltype(&fflush) VC140_fflush;
decltype(&fseek) VC140_fseek;
decltype(&ftell) VC140_ftell;
decltype(&rewind) VC140_rewind;

errno_t hk_fopen_s(FILE **File, const char *Filename, const char *Mode)
{
	if (strstr(Filename, "Plugins.txt"))
		return VC140_fopen_s(File, Filename, Mode);

	if (!Mode || strlen(Mode) <= 0)
		return EINVAL;

	DWORD accessMode = 0;
	DWORD createMode = 0;

	for (size_t i = 0; i < strlen(Mode); i++)
	{
		switch (toupper(Mode[i]))
		{
		case 'R':
			accessMode |= GENERIC_READ;
			createMode = OPEN_EXISTING;
			break;

		case 'W':
			accessMode |= GENERIC_READ | GENERIC_WRITE;
			createMode = CREATE_ALWAYS;
			break;

		case 'A':
			accessMode |= GENERIC_READ | GENERIC_WRITE;
			createMode = OPEN_ALWAYS;
			break;

		case 'B':
			// Always treated as binary
			break;

		case '+':
			accessMode |= GENERIC_WRITE;
			break;

		default:
			AssertMsg(false, "Invalid mode flag");
			return EINVAL;
		}
	}

	HANDLE fileHandle = CreateFileA(Filename, accessMode, FILE_SHARE_READ, nullptr, createMode, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (fileHandle == INVALID_HANDLE_VALUE)
		return EINVAL;

	*File = RegisterFileHandle(fileHandle);
	return 0;
}

errno_t hk_wfopen_s(FILE **File, const wchar_t *Filename, const wchar_t *Mode)
{
	if (!Mode || wcslen(Mode) <= 0)
		return EINVAL;

	DWORD accessMode = 0;
	DWORD createMode = 0;

	for (size_t i = 0; i < wcslen(Mode); i++)
	{
		switch (towupper(Mode[i]))
		{
		case L'R':
			accessMode |= GENERIC_READ;
			createMode = OPEN_EXISTING;
			break;

		case L'W':
			accessMode |= GENERIC_READ | GENERIC_WRITE;
			createMode = CREATE_ALWAYS;
			break;

		case L'A':
			accessMode |= GENERIC_READ | GENERIC_WRITE;
			createMode = OPEN_ALWAYS;
			break;

		case L'B':
			// Always treated as binary
			break;

		case L'+':
			accessMode |= GENERIC_WRITE;
			break;

		default:
			AssertMsg(false, "Invalid mode flag");
			return EINVAL;
		}
	}

	HANDLE fileHandle = CreateFileW(Filename, accessMode, FILE_SHARE_READ, nullptr, createMode, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (fileHandle == INVALID_HANDLE_VALUE)
		return EINVAL;

	*File = RegisterFileHandle(fileHandle);
	return 0;
}

FILE *hk_fopen(const char *Filename, const char *Mode)
{
	FILE *temp;
	errno_t err = hk_fopen_s(&temp, Filename, Mode);

	if (err != 0)
	{
		errno = err;
		return nullptr;
	}

	return temp;
}

int hk_fclose(FILE *stream)
{
	if (MMapFileInfo *info = GetStdioFileMap(stream))
	{
		hk_CloseHandle(info->FileHandle);
		return 0;
	}

	return VC140_fclose(stream);
}

size_t hk_fread(void *ptr, size_t size, size_t count, FILE *stream)
{
	if (size == 0 || count == 0)
		return 0;

	if (MMapFileInfo *info = GetStdioFileMap(stream))
	{
		uint64_t bytesRead = info->Read(ptr, size * count);

		// Returns the number of ELEMENTS read
		return bytesRead / size;
	}

	return VC140_fread(ptr, size, count, stream);
}

size_t hk_fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
{
	if (size == 0 || count == 0)
		return 0;

	if (MMapFileInfo *info = GetStdioFileMap(stream))
	{
		uint64_t bytesWritten = info->Write(ptr, size * count);

		// Returns the number of ELEMENTS written
		return bytesWritten / size;
	}

	return VC140_fwrite(ptr, size, count, stream);
}

int hk_fflush(FILE *stream)
{
	if (MMapFileInfo *info = GetStdioFileMap(stream))
	{
		if (!info->Flush())
			return EOF;

		return 0;
	}

	return VC140_fflush(stream);
}

int hk_fseek(FILE *stream, long offset, int origin)
{
	if (MMapFileInfo *info = GetStdioFileMap(stream))
	{
		if (info->SetFilePointer(offset, nullptr, origin))
			return 0;

		// Failure is anything but 0
		return 1;
	}

	return VC140_fseek(stream, offset, origin);
}

long hk_ftell(FILE *stream)
{
	if (MMapFileInfo *info = GetStdioFileMap(stream))
	{
		// Don't move the pointer - just get where it's currently at
		int64_t currentPos;

		if (!info->SetFilePointer(0, &currentPos, SEEK_CUR))
		{
			errno = EINVAL;
			return -1L;
		}

		AssertMsg(currentPos < LONG_MAX, "64bit -> 32bit truncation");
		return (long)currentPos;
	}

	return VC140_ftell(stream);
}

void hk_rewind(FILE *stream)
{
	if (MMapFileInfo *info = GetStdioFileMap(stream))
	{
		// Don't care if this fails or not
		info->SetFilePointer(0, nullptr, SEEK_SET);
		return;
	}

	VC140_rewind(stream);
}

void PatchFileIO()
{
	if (!g_IsCreationKit)
	{
		*(uint8_t **)&VC140_fopen_s = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "fopen_s", (PBYTE)hk_fopen_s);
		*(uint8_t **)&VC140_wfopen_s = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "_wfopen_s", (PBYTE)hk_wfopen_s);
		*(uint8_t **)&VC140_fopen = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "fopen", (PBYTE)hk_fopen);
		*(uint8_t **)&VC140_fclose = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "fclose", (PBYTE)hk_fclose);

		*(uint8_t **)&VC140_fread = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "fread", (PBYTE)hk_fread);
		*(uint8_t **)&VC140_fwrite = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "fwrite", (PBYTE)hk_fwrite);
		// needs fgets

		*(uint8_t **)&VC140_fflush = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "fflush", (PBYTE)hk_fflush);
		*(uint8_t **)&VC140_fseek = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "fseek", (PBYTE)hk_fseek);
		*(uint8_t **)&VC140_ftell = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "ftell", (PBYTE)hk_ftell);
		*(uint8_t **)&VC140_rewind = Detours::IATHook((PBYTE)g_ModuleBase, "API-MS-WIN-CRT-STDIO-L1-1-0.DLL", "rewind", (PBYTE)hk_rewind);
		// needs feof
	}

	Detours::IATHook((PBYTE)g_ModuleBase, "KERNEL32.dll", "CloseHandle", (PBYTE)hk_CloseHandle);
	Detours::IATHook((PBYTE)g_ModuleBase, "KERNEL32.dll", "ReadFile", (PBYTE)hk_ReadFile);
	Detours::IATHook((PBYTE)g_ModuleBase, "KERNEL32.dll", "ReadFileEx", (PBYTE)hk_ReadFileEx);
	Detours::IATHook((PBYTE)g_ModuleBase, "KERNEL32.dll", "SetFilePointerEx", (PBYTE)hk_SetFilePointerEx);
}