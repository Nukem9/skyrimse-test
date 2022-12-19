//////////////////////////////////////////
/*
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

#include "BSFile_CK.h"
#include "LogWindow.h"
#include "../../xutil.h"

#include <io.h> 

#pragma warning(disable : 6011)
#pragma warning(disable : 6031)

/// 

struct WRAP_FILE {
	DWORD cbSize;
	DWORD dwSign;
	HANDLE hFile;

	static FILE* CreateInstance(HANDLE handle);
	static BOOL IsWrapInstance(FILE* handle);
	static WRAP_FILE* GetInstance(FILE* handle);
};

FILE* WRAP_FILE::CreateInstance(HANDLE handle) {
	if (handle == INVALID_HANDLE_VALUE)
		return NULL;

	WRAP_FILE* wrapHandle = new WRAP_FILE;
	if (wrapHandle) {
		wrapHandle->cbSize = sizeof(WRAP_FILE);
		wrapHandle->dwSign = 0xDEADDEAD;
		wrapHandle->hFile = handle;
	}

	//_MESSAGE_FMT("CreateInstance: %p %d", wrapHandle, wrapHandle->cbSize);

	return (FILE*)(((CHAR*)wrapHandle) + 8);
}

BOOL WRAP_FILE::IsWrapInstance(FILE* handle) {
	if (!handle)
		return FALSE;

	auto wrapHandle = (WRAP_FILE*)(((CHAR*)handle) - 8);
	//_MESSAGE_FMT("IsWrapInstance: %p sign %X %d", wrapHandle, wrapHandle->dwSign, wrapHandle->cbSize);

	return (wrapHandle->cbSize == sizeof(WRAP_FILE)) &&
		(wrapHandle->dwSign == 0xDEADDEAD);
}

WRAP_FILE* WRAP_FILE::GetInstance(FILE* handle) {
	if (IsWrapInstance(handle))
		return (WRAP_FILE*)(((CHAR*)handle) - 8);

	return NULL;
}

/// 

BOOL FIXAPI FileExists(LPCSTR filename) {
	return access(filename, 0) != -1;
}

HANDLE FIXAPI FileCreate(LPCSTR filename, LPCSTR mode) {
	if (!mode || !filename)
		return INVALID_HANDLE_VALUE;

	auto lmode = strlen(mode);
	if (mode[lmode - 1] == 't')
		// text mode
		return INVALID_HANDLE_VALUE;

	_MESSAGE_FMT("Open binary file: '%s' mode '%s'", filename, mode);

	BOOL bAppend = FALSE;
	DWORD dwShareMode = 0;
	DWORD dwDesiredAccess = 0;
	DWORD dwCreationDisposition = 0;

	if (mode[1] != '+') {
		if (mode[0] == 'r') {
			dwCreationDisposition = OPEN_EXISTING;
			dwShareMode = FILE_SHARE_READ;
			dwDesiredAccess = GENERIC_READ;
		}
		else if (mode[0] == 'w') {
			dwCreationDisposition = OPEN_ALWAYS;
			dwDesiredAccess = GENERIC_WRITE;
		}
		else if (mode[0] == 'a') {
			dwCreationDisposition = OPEN_ALWAYS;
			dwDesiredAccess = GENERIC_WRITE;
			bAppend = TRUE;
		}
	}
	else {
		dwCreationDisposition = CREATE_ALWAYS;
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		bAppend = mode[0] == 'a';
	}

	HANDLE hFile = CreateFile(filename, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (hFile != INVALID_HANDLE_VALUE) {
		if (bAppend) {
			LONG newSizeHigh = 0;
			SetFilePointer(hFile, 0, &newSizeHigh, FILE_END);
		}

		return hFile;
	}

	return INVALID_HANDLE_VALUE;
}

VOID FIXAPI FileClose(HANDLE handle) {
	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle(handle);
}

INT64 FIXAPI FileSeek(HANDLE handle, INT64 distance, BSFileOrigins origin) {
	LARGE_INTEGER li;
	li.QuadPart = distance;

	if (((li.LowPart = SetFilePointer(handle, li.LowPart, &li.HighPart, (DWORD)origin)) == INVALID_SET_FILE_POINTER) && 
		(GetLastError() != NO_ERROR))
		return -1;

	return li.QuadPart;
}

INT64 FIXAPI FileSeekEx(HANDLE handle, INT64 distance, BSFileOrigins origin) {
	LARGE_INTEGER li, liRes;
	li.QuadPart = distance;

	if (!SetFilePointerEx(handle, li, &liRes, (DWORD)origin) && (GetLastError() != NO_ERROR))
		return -1;

	return liRes.QuadPart;
}

LONG FIXAPI FileRead(HANDLE handle, LPVOID buffer, LONG size) {
	DWORD dwResult = 0;
	if (ReadFile(handle, buffer, size, &dwResult, NULL))
		return !dwResult ? EOF : (LONG)dwResult;

	return EOF;
}

LONG FIXAPI FileWrite(HANDLE handle, LPVOID buffer, LONG size) {
	DWORD dwResult = 0;
	if (WriteFile(handle, buffer, size, &dwResult, NULL))
		return (LONG)dwResult;

	return EOF;
}

BOOL FIXAPI FileFlush(HANDLE handle) {
	return FlushFileBuffers(handle);
}

INT64 FIXAPI FileSize(HANDLE handle) {
	auto SafePos = FileSeekEx(handle, 0);
	auto SizeRet = FileSeekEx(handle, 0, BSFileOrigins::kFileOrigin_Ending);
	FileSeekEx(handle, SafePos, BSFileOrigins::kFileOrigin_Beginning);
	return SizeRet;
}

INT64 FIXAPI FileSizeByName(LPCSTR filename) {
	WIN32_FILE_ATTRIBUTE_DATA info;

	if (GetFileAttributesExA(filename, GetFileExInfoStandard, &info))
		return (int64_t)info.nFileSizeLow | ((int64_t)info.nFileSizeHigh << 32);

	return 0;
}

INT64 FIXAPI FileGetPosition(HANDLE handle) {
	return FileSeekEx(handle, 0);
}

VOID FIXAPI FileSetPosition(HANDLE handle, INT64 distance) {
	FileSeekEx(handle, distance, BSFileOrigins::kFileOrigin_Beginning);
}

BOOL FIXAPI FileEof(HANDLE handle) {
	char szBuf[1];
	if (FileRead(handle, szBuf, 1) != EOF) {
		FileSeek(handle, -1);
		return FALSE;
	}

	return TRUE;
}

/// 

BOOL BSFile::hk_ICreateInstance(LPCSTR fileName, BSFileModes mode, DWORD64 bufferSize, BOOL isTextFile) {
	if (mode == BSFileModes::kFileMode_ReadOnly && bufferSize < 0x40000)
		bufferSize = 0x40000;

	return ICreateInstance(this, fileName, mode, bufferSize, isTextFile);
}

/// 

FILE* BSFile::hk_fopen(const char* filename, const char* mode) {
	auto handle = FileCreate(filename, mode);
	if (handle != INVALID_HANDLE_VALUE)
		return WRAP_FILE::CreateInstance(handle);
	else
		return fopen(filename, mode);
}

errno_t BSFile::hk_fopen_s(FILE** stream, const char* filename, const char* mode) {
	if (!stream || !mode || !filename)
		return EINVAL;

	*stream = hk_fopen(filename, mode);
	if (!(*stream))
		return EINVAL;

	return 0;
}

long BSFile::hk_ftell(FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);

	//_MESSAGE_FMT("hk_ftell: %p - %p", stream, wrapHandle);

	if (!wrapHandle)
		return ftell(stream);

	return (long)FileGetPosition(wrapHandle->hFile);
}

int BSFile::hk_fseek(FILE* stream, long offset, int origin) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return fseek(stream, offset, origin);

	BSFileOrigins Origin = BSFileOrigins::kFileOrigin_Beginning;
	if (origin == SEEK_END)
		Origin = BSFileOrigins::kFileOrigin_Ending;
	else if (origin == SEEK_CUR)
		Origin = BSFileOrigins::kFileOrigin_Current;

	if (FileSeekEx(wrapHandle->hFile, offset, Origin) == -1)
		return EINVAL;

	return 0;
}

void BSFile::hk_fclose(FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle) {
		fclose(stream);
		return;
	}

	FileClose(wrapHandle->hFile);
	delete wrapHandle;
}

int BSFile::hk_fflush(FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return fflush(stream);

	return FileFlush(wrapHandle->hFile) ? 0 : EOF;
}

int BSFile::hk_feof(FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return feof(stream);

	return FileEof(wrapHandle->hFile);
}

int BSFile::hk_fgetc(FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return fgetc(stream);

	char ch[1];
	if (FileRead(wrapHandle->hFile, ch, 1) == EOF)
		// end of file
		return EOF;

	return ch[0];
}

char* BSFile::hk_fgets(char* str, int numChars, FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return fgets(str, numChars, stream);

	if (!str || numChars <= 0)
		return NULL;

	if (numChars == 1) {
		str[0] = NULL;
		return str;
	}

	LONG lRes = 0;
	ZeroMemory(str, numChars);
	if ((lRes = FileRead(wrapHandle->hFile, str, numChars - 1)) != EOF) {
		int iLen = 0;
		for (; iLen < lRes - 1; iLen++) {
			if (str[iLen] == '\n')
				break;
		}

		str[iLen + 1] = NULL;
		FileSeek(wrapHandle->hFile, -(lRes - (iLen + 1)));

		return str;
	}

	return NULL;
}

int BSFile::hk_fputc(int character, FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return fputc(character, stream);

	DWORD dwResult = 0;
	char ch[1] = { (char)character };
	if (WriteFile(wrapHandle->hFile, ch, 1, &dwResult, NULL))
		return character;

	return EOF;
}

int BSFile::hk_fputs(char* str, FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return fputs(str, stream);

	if (!str)
		return EOF;

	DWORD dwResult = 0;
	DWORD dwWriteBytes = strlen(str);

	if (!dwWriteBytes)
		return 0;

	if (WriteFile(wrapHandle->hFile, str, dwWriteBytes, &dwResult, NULL))
		return (int)dwResult;

	return EOF;
}

size_t BSFile::hk_fread(void* buffer, size_t size, size_t count, FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return fread(buffer, size, count, stream);

	return FileRead(wrapHandle->hFile, buffer, size * count);
}

size_t BSFile::hk_fwrite(void* buffer, size_t size, size_t count, FILE* stream) {
	auto wrapHandle = WRAP_FILE::GetInstance(stream);
	if (!wrapHandle)
		return fwrite(buffer, size, count, stream);

	return FileWrite(wrapHandle->hFile, buffer, size * count);
}