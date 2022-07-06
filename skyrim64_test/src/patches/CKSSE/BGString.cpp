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

#include "BGString.h"

namespace usse {
	namespace api {
		BGSStringCache StringCache;
		BGSConvertorString ConvertorString;

		LPCSTR BGSConvertorString::Utf8ToWinCP(LPCSTR src)
		{
			// Ansi verification is necessary because there are a lot of strings, especially short and system strings. 
			// The debug file without this option was more than 70 mb, compared to 2604 kb.
			// Translation of fallout4.esm has become significantly faster.

			if ((src == pre) || !IsValid(src) || !XUtil::Conversion::IsUtf8Valid(src))
				return src;

			std::string wincp_str = XUtil::Conversion::Utf8ToAnsi(src);

			// utf-8 takes up more memory than ansi, so I can use active memory
			pre = src;
			strcpy(const_cast<LPSTR>(pre), wincp_str.c_str());

			return pre;
		}

		LPCSTR BGSConvertorString::WinCPToUtf8(LPCSTR src)
		{
			// Not all strings are translated during loading and remain in Utf-8. 
			// They are loaded after opening the dialog. As an example "Description".

			if (!IsValid(src) || XUtil::Conversion::IsUtf8Valid(src))
				return src;

			// in the Creation Kit code, the request to return a string occurs twice in a row.
			if ((pre == src) && (StringCache.Size() > 0))
				return StringCache.Last();

			// convert
			std::string utf8_str = XUtil::Conversion::AnsiToUtf8(src);

			// Unicode initially takes up more memory than ansi. 
			// Therefore, a heap is created that will store memory for the duration of saving.
			// Lists work quickly on adding items.
			StringCache.Push(utf8_str);

			// pointer to the memory of the contents of the last line, it is of course unique
			pre = src;
			return StringCache.Last();
		}

		LPCSTR BGSConvertorString::Convert(LPCSTR s) {
			// Back to utf-8 (temporarily)
			if (mode == BGSConvertorString::MODE_UTF8)
				return WinCPToUtf8(s);
			// All APIs are implemented as ANSI. In this regard, utf-8 must be converted to the specified ANSI.
			else
				return Utf8ToWinCP(s);
		}
	}
}