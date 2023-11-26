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

#include "..\..\common.h"

#ifndef LPSTR_TEXTCALLBACKA
#include <CommCtrl.h>
#endif // LPSTR_TEXTCALLBACKA

namespace usse {
	namespace api {
		class BGSStringCache {
		private:
			std::list<std::string> cache;
		public:
			BGSStringCache(VOID) = default;
			~BGSStringCache(VOID) = default;
		public:
			inline DWORD Size(VOID) const { return cache.size(); }
			inline VOID Clear(VOID) { cache.clear(); }
			inline VOID Push(const std::string& s) { cache.push_back(s); }
			inline LPCSTR Last(VOID) const { return &cache.back()[0]; }
		};

		extern BGSStringCache StringCache;

		class BGSConvertorString {
		public:
			enum {
				MODE_ANSI = 0,
				MODE_UTF8
			};
		private:
			LPCSTR pre;
			BYTE mode;
		public:
			BGSConvertorString(VOID) = default;
			~BGSConvertorString(VOID) = default;
		private:
			inline BOOL IsValid(LPCSTR s) const {
				return ((s != NULL) && (s != LPSTR_TEXTCALLBACKA) && (strlen(s) > 0));
			}

			LPCSTR Utf8ToWinCP(LPCSTR src);
			LPCSTR WinCPToUtf8(LPCSTR src);
		public:
			inline BYTE GetMode(VOID) const { return mode; }
			inline VOID SetMode(BYTE m) {
				mode = m;
				StringCache.Clear();
			}

			LPCSTR Convert(LPCSTR s);
		public:
			PROPERTY(GetMode, SetMode) BYTE Mode;
		};

		extern BGSConvertorString ConvertorString;

		typedef LPSTR(*lpfnGetStr_t)(LPCSTR);
		static lpfnGetStr_t BGSLocalizedString_OldGetStrProc;

		class BGSLocalizedString {
		public:
			static LPCSTR GetStr(LPCSTR str) {
				return ConvertorString.Convert(BGSLocalizedString_OldGetStrProc(str));
			}
		};
	}
}