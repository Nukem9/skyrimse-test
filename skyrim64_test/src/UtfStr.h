//////////////////////////////////////////
/*
* Copyright (c) 2020-2021 Perchik71 <email:perchik71@outlook.com>
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

#include <atomic>
#include <string>

#define UTILAPI WINAPI

namespace XUtil
{
	namespace Conversion
	{
		typedef std::basic_string<char> utf8string;
		typedef std::basic_string<wchar_t> utf16string;

#if SKYRIMSE_LAZ_UNICODE_PLUGIN
		bool UTILAPI LazUnicodePluginInit(void);
#endif // !SKYRIMSE_LAZ_UNICODE_PLUGIN

		bool UTILAPI IsUtf8Valid(const utf8string &str);
		utf8string UTILAPI Utf16ToUtf8(const utf16string &str);
		utf16string UTILAPI Utf8ToUtf16(const utf8string &str);
		std::string UTILAPI Utf8ToAnsi(const utf8string &str);
		utf8string UTILAPI AnsiToUtf8(const std::string &str);
		std::string UTILAPI Utf16ToAnsi(const utf16string &str);
		utf16string UTILAPI AnsiToUtf16(const std::string &str);
		std::string UTILAPI WideToAnsi(const std::wstring &str);
	    std::wstring UTILAPI AnsiToWide(const std::string &str);
	}
}
