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

#pragma once

#include "../../common.h"

enum class BSFileOrigins : DWORD {
	kFileOrigin_Beginning = 0,
	kFileOrigin_Current = 1,
	kFileOrigin_Ending = 2
};

enum class BSFileModes : DWORD {
	kFileMode_ReadOnly = 0,
	kFileMode_WriteOnly = 1,
	kFileMode_AppendOnly = 2
};

BOOL FIXAPI FileExists(LPCSTR filename);
HANDLE FIXAPI FileCreate(LPCSTR filename, LPCSTR mode);
VOID FIXAPI FileClose(HANDLE handle);
INT64 FIXAPI FileSeek(HANDLE handle, INT64 distance, BSFileOrigins origin = BSFileOrigins::kFileOrigin_Current);
INT64 FIXAPI FileSeekEx(HANDLE handle, INT64 distance, BSFileOrigins origin = BSFileOrigins::kFileOrigin_Current);
LONG FIXAPI FileRead(HANDLE handle, LPVOID buffer, LONG size);
LONG FIXAPI FileWrite(HANDLE handle, LPVOID buffer, LONG size);
BOOL FIXAPI FileFlush(HANDLE handle);
INT64 FIXAPI FileSize(HANDLE handle);
INT64 FIXAPI FileSizeByName(LPCSTR filename);
INT64 FIXAPI FileGetPosition(HANDLE handle);
VOID FIXAPI FileSetPosition(HANDLE handle, INT64 distance);
BOOL FIXAPI FileEof(HANDLE handle);

class BSFile {
public:
	inline static BOOL(*ICreateInstance)(BSFile*, LPCSTR, BSFileModes, DWORD64, BOOL);
	BOOL hk_ICreateInstance(LPCSTR fileName, BSFileModes mode, DWORD64 bufferSize, BOOL isTextFile);
public:
	// standard function wrapper
	// Using standard C99 functions for I/O is unacceptable.
	// The reading takes place in very small buffer. If you believe procmon.exe ReadFile calls reach up to a million in 4kb - 8kb.
	// -- do not use these functions they are outdated technologically --
	//
	// I decided them here as I need to fix the BSFile, but I'm sure other TESFile or NiFile are affected as well.

	static FILE* hk_fopen(const char* filename, const char* mode);
	static errno_t hk_fopen_s(FILE** stream, const char* filename, const char* mode);
	static long hk_ftell(FILE* stream);
	static int hk_fseek(FILE* stream, long offset, int origin);
	static void hk_fclose(FILE* stream);
	static int hk_fflush(FILE* stream);
	static int hk_feof(FILE* stream);
	static int hk_fgetc(FILE* stream);
	static char* hk_fgets(char* str, int numChars, FILE* stream);
	static int hk_fputc(int character, FILE* stream);
	static int hk_fputs(char* str, FILE* stream);
	static size_t hk_fread(void* buffer, size_t size, size_t count, FILE* stream);
	static size_t hk_fwrite(void* buffer, size_t size, size_t count, FILE* stream);
};