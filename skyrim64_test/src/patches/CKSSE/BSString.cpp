//////////////////////////////////////////
/*
* Fallout 4 Script Extender (F4SE)
* by Ian Patterson, Stephen Abel, and Brendan Borthwick (ianpatt, behippo, and purplelunchbox)
*
* Contact the F4SE Team
*
* Entire Team
* Send email to team [at] f4se [dot] silverlock [dot] org
*
* Ian (ianpatt)
* Send email to ianpatt+f4se [at] gmail [dot] com
*
* Stephen (behippo)
* Send email to gamer [at] silverlock [dot] org
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
* PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
* FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
* OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
//////////////////////////////////////////

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

#include "..\TES\MemoryManager.h"
#include "BSString.h"

#define WDELIMITERS " \t\n\r\f\v"

#define BSSTRING_DEBUG 0

BSString::BSString(VOID) : 
	m_data(NULL), m_dataLen(0), m_bufLen(0) 
{}

BSString::BSString(LPCSTR string, WORD size) : BSString() {
	Set(string ? string : "", size);
}

BSString::BSString(const std::string& string, WORD size) : BSString() {
	Set(string.empty() ? "" : string.c_str(), size);
}

BSString::BSString(const BSString& string, WORD size) : BSString() {
	Set(string, size);
}

BSString::BSString(const BSString& string) : BSString() {
	Set(string, string.m_bufLen);
}

BSString::~BSString(VOID) {
	Clear();
}

BOOL BSString::Reserved(WORD size) {
	WORD newLen = size;
	WORD newSize = newLen + 1;
	auto newData = (LPSTR)MemoryManager::Allocate(NULL, newSize, 0, false);
	if (!newData)
			return FALSE;

	if (m_bufLen) {
		strncpy(newData, m_data, newLen);
		MemoryManager::Deallocate(NULL, (LPVOID)m_data, false);
		newData[newLen] = 0;
	}
	else
		newData[0] = 0;

	m_data = newData;
	m_bufLen = newSize;
	m_dataLen = newLen;

	return TRUE;
}

BOOL BSString::Set(LPCSTR string, WORD size) {
	pad0C = 0;

	WORD wNeedLen, wNeedBuf;

	if (!size)
		wNeedLen = (string) ? strlen(string) : 0;
	else
		wNeedLen = size;

	wNeedBuf = wNeedLen + 1;

	if (wNeedBuf > m_bufLen) {
		if (!wNeedLen)
			goto __dealloc;
		else {
			LPSTR newData = (LPSTR)MemoryManager::Allocate(NULL, wNeedBuf, 0, false);
			memcpy(newData, string, wNeedLen);
			newData[wNeedLen] = 0;

			if (m_data)
				MemoryManager::Deallocate(NULL, (LPVOID)m_data, false);

			WORD wRealBuf = (WORD)MemoryManager::Size(NULL, newData);

			m_data = newData;
			m_bufLen = wRealBuf > wNeedBuf ? wRealBuf : wNeedBuf;
			m_dataLen = wNeedLen;
		}
	}
	else {
		if (!string) {
			__dealloc:
			if (m_data) {
				MemoryManager::Deallocate(NULL, (LPVOID)m_data, false);

				m_data = NULL;
				m_bufLen = 0;
				m_dataLen = 0;
			}
		}
		else {
			memcpy(m_data, string, wNeedLen);
			m_dataLen = wNeedLen;
			m_data[wNeedLen] = 0;
		}
	}

	return TRUE;
}

BOOL BSString::Set(const BSString& string, WORD size) {
#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: string.m_data %s", __FUNCTION__, string.m_data);
#endif

	return Set(string.m_data ? *string : "", size);
}

VOID BSString::Clear(VOID) {
	if (m_data) {
		MemoryManager::Deallocate(NULL, (LPVOID)m_data, false);
		m_data = NULL;
		m_dataLen = 0;
		m_bufLen = 0;
	}
}

BSString BSString::Reverse(VOID) const {
	if (!m_dataLen)
		return "";

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: m_data %s m_bufLen %d m_dataLen %d", __FUNCTION__, m_data, m_bufLen, m_dataLen);
#endif

	auto s = BSString(m_data);

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: s.m_data %s", __FUNCTION__, s.m_data);
#endif

	return strrev(*s);
}

BSString& BSString::Format(LPCSTR format, ...) {
	Clear();

	va_list va;
	va_start(va, format);
	auto size = _vsnprintf(NULL, 0, format, va);
	if (size) {
		m_bufLen = size + 1;
		m_dataLen = size;
		m_data = (LPSTR)MemoryManager::Allocate(NULL, m_bufLen, 0, false);
		if (!m_data) {
			m_bufLen = 0;
			m_dataLen = 0;
		}
		else {
			vsprintf(m_data, format, va);
			m_data[size] = 0;
		}
	}
	va_end(va);

	return *this;
}

BSString& BSString::Format(LPCSTR format, va_list ap) {
	auto size = _vsnprintf(NULL, 0, format, ap);
	if (size) {
		m_bufLen = size + 1;
		m_dataLen = size;
		m_data = (LPSTR)MemoryManager::Allocate(NULL, m_bufLen, 0, false);
		if (!m_data) {
			m_bufLen = 0;
			m_dataLen = 0;
		}
		else {
			vsprintf(m_data, format, ap);
			m_data[size] = 0;
		}
	}

	return *this;
}

INT BSString::Compare(LPCSTR string, BOOL ignoreCase) const {
	if (ignoreCase)
		return _stricmp(m_data, string);
	else
		return strcmp(m_data, string);
}

BSString& BSString::Append(LPCSTR str) {
	if (str) {
		DWORD dwLen = strlen(str);
		if (dwLen) {
			if (m_dataLen) {
				if (Reserved(m_dataLen + dwLen + 1))
					strcat(m_data, str);
			}
			else
				Set(str);
		}
	}

	return *this;
}

BSString& BSString::Append(const BSString& string) {
	return Append(string.m_dataLen ? *string : "");
}

BSString& BSString::Append(CHAR ch) {
	CHAR Buf[2] = { ch, 0 };
	return Append(Buf);
}

BSString& BSString::AppendFormat(LPCSTR format, ...) {
	BSString fmt;
	va_list va;
	va_start(va, format);
	fmt.Format(format, va);
	va_end(va);

	return fmt.IsEmpty() ? *this : Append(fmt);
}

BSString& BSString::AppendFormat(LPCSTR format, va_list ap) {
	BSString fmt;
	fmt.Format(format, ap);
	return fmt.IsEmpty() ? *this : Append(fmt);
}

BSString& BSString::Copy(WORD start, WORD len) {
	return Assign(m_data, start, len);
}

BSString& BSString::Assign(LPCSTR str, WORD start, WORD len) {
	if (str) {
		if (!len)
			len = strlen(str);

		Set(*BSString(str + start), len);
	}

	return *this;
}

BSString& BSString::AssignUnsafe(LPCSTR str, WORD start, WORD len) {
	if (str) {
		if (!len)
			len = strlen(str);

		Set(str + start, len);
	}

	return *this;
}

BSString& BSString::Erase(WORD start, WORD len) {
	if ((!start && !len) || (start >= len))
		return *this;

	if (!start) {
		if (len >= m_dataLen)
			Clear();
		else
			return Assign(m_data, len);
	}
	else {
		if (len >= m_dataLen)
			return Assign(m_data, 0, start);
		else
			return Assign(m_data, 0, start).Append(m_data + (start + len));
	}

	return *this;
}

WORD BSString::FindLastOf(CHAR ch, WORD offset) const {
	if (!m_data)
		return srNone;

	LPSTR Ret = strrchr(m_data + offset, ch);

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: ch %c m_data %s Ret %d", __FUNCTION__, ch, m_data, Ret);
#endif

	return Ret ? (WORD)(Ret - m_data) : srNone;
}

WORD BSString::FindFirstOf(CHAR ch, WORD offset) const {
	if (IsEmpty())
		return srNone;

	LPSTR Ret = strchr(m_data + offset, ch);

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: ch %c m_data %s Ret %d", __FUNCTION__, ch, m_data, Ret);
#endif

	return Ret ? (WORD)(Ret - m_data) : srNone;
}

WORD BSString::FindLastOf(LPCSTR chs, WORD offset) const {
	WORD Ret = Reverse().FindFirstOf(chs, offset);

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: chs %s m_data %s Ret %d", __FUNCTION__, chs, m_data, Ret);
#endif

	return Ret == srNone ? srNone : m_dataLen - Ret;
}

WORD BSString::FindFirstOf(LPCSTR chs, WORD offset) const {
	if (IsEmpty() || !chs)
		return srNone;

	LPSTR Ret = strpbrk(m_data + offset, chs);

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: chs %s m_data %s Ret %d", __FUNCTION__, chs, m_data, Ret);
#endif

	return Ret ? Ret - m_data : srNone;
}

BSString BSString::UpperCase(VOID) const {
	if (IsEmpty())
		return "";

	auto s = BSString(m_data);
	UpperCase(*s);
	return s;
}

BSString BSString::LowerCase(VOID) const {
	if (IsEmpty())
		return "";

	auto s = BSString(m_data);
	LowerCase(*s);
	return s;
}

void BSString::UpperCase(LPCSTR str) {
	if (!str)
		return;

	strupr(const_cast<LPSTR>(str));
}

void BSString::LowerCase(LPCSTR str) {
	if (!str)
		return;

	strlwr(const_cast<LPSTR>(str));
}

WORD BSString::Find(LPCSTR substr, EFlags flags) const {
	if (IsEmpty() || !substr)
		return srNone;

	LPSTR Ret = NULL;
	if (flags == sfInsensitiveCase)
		Ret = strstr(*LowerCase(), *(BSString(substr).LowerCase()));
	else
		LPSTR Ret = strstr(m_data, substr);

	return Ret ? Ret - m_data : srNone;
}

BSString BSString::Trim(VOID) const {
	if (IsEmpty())
		return "";

	std::string s(m_data);
	s.erase(s.find_last_not_of(WDELIMITERS) + 1);
	s.erase(0, s.find_first_not_of(WDELIMITERS));

	return s.c_str();
}

BSString BSString::FormatString(LPCSTR format, ...) {
	BSString fmt;
	va_list va;

	va_start(va, format);
	fmt.Format(format, va);
	va_end(va);

	return fmt;
}

BSString BSString::Utils::ExtractFilePath(const BSString& fname) {
	WORD it = fname.FindLastOf("/\\");

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: it %d fname %s", __FUNCTION__, it, *fname);
#endif

	if (it != srNone)
		return BSString(*fname, it);

	return fname;
}

BSString BSString::Utils::ExtractFileName(const BSString& fname) {
	WORD it = fname.FindLastOf("/\\");

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: it %d fname %s", __FUNCTION__, it, *fname);
#endif

	if (it != srNone)
		return BSString(*fname + (it + 1));
	
	return fname;
}

BSString BSString::Utils::ExtractFileExt(const BSString& fname) {
	WORD it = fname.FindLastOf('.');

#if BSSTRING_DEBUG
	_MESSAGE_FMT("%s: it %d fname %s", __FUNCTION__, it, *fname);
#endif

	if (it != srNone)
		return BSString(*fname + it);
	
	return "";
}

BSString BSString::Utils::GetCurrentPath(VOID) {
	constexpr auto SIZE = 1024;
	CHAR szTemp[SIZE];
	
	if (::GetCurrentDirectoryA(SIZE, szTemp))
		return szTemp;

	return "";
}

BSString BSString::Utils::GetFileNameModule(const BSString& mname) {
	constexpr auto SIZE = 1024;
	CHAR szTemp[SIZE];

	if (::GetModuleFileNameA(GetModuleHandleA(mname.IsEmpty() ? NULL : *mname), szTemp, MAX_PATH))
		return szTemp;

	return "";
}

BSString BSString::Utils::GetApplicationPath(VOID) {
	return ExtractFilePath(GetFileNameModule(""));
}

BSString BSString::Utils::ChangeFileExt(const BSString& fname, const BSString& ext) {
	WORD it = fname.FindLastOf('.');

	if (it != srNone)
		return BSString(*fname, it - 1).Append(ext);

	return BSString(fname).Append(ext);
}

BSString BSString::Utils::GetRelativeDataPath(VOID) {
	return "Data\\";
}

BSString BSString::Utils::GetDataPath(VOID) {
	return GetApplicationPath() + "Data\\";
}

BSString BSString::Utils::GetFixesPath(VOID) {
	return GetApplicationPath() + "Data\\F4CKFixes\\";
}

BOOL BSString::Utils::FileExists(const BSString& fname) {
	auto Ret = GetFileAttributesA(*fname);
	return (Ret != INVALID_FILE_ATTRIBUTES) && ((Ret & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY);
}

BOOL BSString::Utils::DirectoryExists(const BSString& fname) {
	auto Ret = GetFileAttributesA(*fname);
	return (Ret != INVALID_FILE_ATTRIBUTES) && ((Ret & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
}

BSString BSString::Transforms::BoolToStr(BOOL value, BOOL inText) {
	if (inText)
		return (value) ? "True" : "False";
	else
		return (value) ? "1" : "0";
}

BSString BSString::Transforms::IntToStr(INT64 value) {
	CHAR szBuffer[128];
	return _i64toa(value, szBuffer, 10);
}

BSString BSString::Transforms::UIntToStr(UINT64 value) {
	CHAR szBuffer[128];
	return _ui64toa(value, szBuffer, 10);
}

BSString BSString::Transforms::FloatToStr(DOUBLE value) {
	return std::to_string(value);
}

BOOL BSString::Transforms::StrToBool(const BSString& value) {
	if (*value && ((value == "1") || (value == "True")))
		return TRUE;

	return FALSE;
}

INT64 BSString::Transforms::StrToInt(const BSString& value) {
	LPSTR lpEnd = NULL;
	return strtoll(*value, &lpEnd, 10);
}

UINT64 BSString::Transforms::StrToUInt(const BSString& value) {
	LPSTR lpEnd = NULL;
	return strtoull(*value, &lpEnd, 10);
}

DOUBLE BSString::Transforms::StrToFloat(const BSString& value) {
	LPSTR lpEnd = NULL;
	return strtod(*value, &lpEnd);
}

INT64 BSString::Transforms::HexToInt(const BSString& value) {
	LPSTR lpEnd = NULL;
	return strtoll(*value, &lpEnd, 16);
}

UINT64 BSString::Transforms::HexToUInt(const BSString& value) {
	LPSTR lpEnd = NULL;
	return strtoull(*value, &lpEnd, 16);
}

BSString BSString::Transforms::IntToHex(INT64 value) {
	std::stringstream stream;
	stream << "0x"
		<< std::setfill('0') << std::setw(sizeof(INT64) * 2)
		<< std::hex << value;
	return stream.str();
}

BSString BSString::Transforms::UIntToHex(UINT64 value) {
	std::stringstream stream;
	stream << "0x"
		<< std::setfill('0') << std::setw(sizeof(UINT64) * 2)
		<< std::hex << value;
	return stream.str();
}

BSString BSString::Converts::Utf8ToAnsi(const BSString& str) {
	if (XUtil::Conversion::IsUtf8Valid(*str))
		return XUtil::Conversion::Utf8ToAnsi(*str);
	else
		return str;
}

BSString BSString::Converts::AnsiToUtf8(const BSString& str) {
	if (!XUtil::Conversion::IsUtf8Valid(*str))
		return XUtil::Conversion::AnsiToUtf8(*str);
	else
		return str;
}

BSString BSString::Converts::WideToAnsi(LPWSTR str) {
	return XUtil::Conversion::WideToAnsi(str);
}