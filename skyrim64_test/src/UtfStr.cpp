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

#include "Common.h"
#include "UtfStr.h"
#include "xutil.h"

#define __deprecated_utf8

#ifndef __deprecated_utf8
#define UNICODE_INVALID 63
#endif // __deprecated_utf8

namespace XUtil
{
	namespace Conversion
	{
#if SKYRIMSE_LAZ_UNICODE_PLUGIN
		typedef bool(*laz_unicode_plugin_is_utf8_t)(const char* str);
		typedef int(*laz_unicode_plugin_wincp_to_utf8_t)(const char* src, char* dst);
		typedef int(*laz_unicode_plugin_utf8_to_wincp_t)(const char* src, char* dst, const bool test_invalid);
		typedef int(*laz_unicode_plugin_utf16_to_utf8_t)(const wchar_t* src, char* dst);
		typedef int(*laz_unicode_plugin_utf8_to_utf16_t)(const char* src, wchar_t* dst, const bool test_invalid);

		laz_unicode_plugin_is_utf8_t laz_unicode_plugin_is_utf8 = nullptr;
		laz_unicode_plugin_wincp_to_utf8_t laz_unicode_plugin_wincp_to_utf8 = nullptr;
		laz_unicode_plugin_utf8_to_wincp_t laz_unicode_plugin_utf8_to_wincp = nullptr;
		laz_unicode_plugin_utf16_to_utf8_t laz_unicode_plugin_utf16_to_utf8 = nullptr;
		laz_unicode_plugin_utf8_to_utf16_t laz_unicode_plugin_utf8_to_utf16 = nullptr;
#endif // !SKYRIMSE_LAZ_UNICODE_PLUGIN

#if SKYRIMSE_LAZ_UNICODE_PLUGIN
		bool UTILAPI LazUnicodePluginInit(void)
		{
			HMODULE hModule = LoadLibraryW(L"CreationKitUnicodePlugin.dll");
			if (hModule)
			{
				laz_unicode_plugin_is_utf8 = (laz_unicode_plugin_is_utf8_t)GetProcAddress(hModule, (LPCSTR)5);
				laz_unicode_plugin_wincp_to_utf8 = (laz_unicode_plugin_wincp_to_utf8_t)GetProcAddress(hModule, (LPCSTR)2);
				laz_unicode_plugin_utf8_to_wincp = (laz_unicode_plugin_utf8_to_wincp_t)GetProcAddress(hModule, (LPCSTR)1);
				laz_unicode_plugin_utf16_to_utf8 = (laz_unicode_plugin_utf16_to_utf8_t)GetProcAddress(hModule, (LPCSTR)4);
				laz_unicode_plugin_utf8_to_utf16 = (laz_unicode_plugin_utf8_to_utf16_t)GetProcAddress(hModule, (LPCSTR)3);

				return laz_unicode_plugin_is_utf8 && laz_unicode_plugin_wincp_to_utf8 && laz_unicode_plugin_utf8_to_wincp &&
					laz_unicode_plugin_utf16_to_utf8 && laz_unicode_plugin_utf8_to_utf16;
			}

			return false;
		}
#endif // !SKYRIMSE_LAZ_UNICODE_PLUGIN

		bool UTILAPI IsUtf8Valid(const utf8string &str)
		{
#if SKYRIMSE_LAZ_UNICODE_PLUGIN
			if (laz_unicode_plugin_is_utf8)
				return laz_unicode_plugin_is_utf8(str.c_str());
			else
			{
				if (!MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0))
					return GetLastError() == ERROR_NO_UNICODE_TRANSLATION;
				return true;
			}
#else
			if (!MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0))
				return GetLastError() == ERROR_NO_UNICODE_TRANSLATION;
			return true;
#endif // !SKYRIMSE_LAZ_UNICODE_PLUGIN
		}

		utf8string UTILAPI Utf16ToUtf8(const utf16string &str)
		{
			utf8string utf8str;

			if (str.empty())
				return utf8str;

#if SKYRIMSE_LAZ_UNICODE_PLUGIN
			if (laz_unicode_plugin_utf16_to_utf8)
			{
				int l = laz_unicode_plugin_utf16_to_utf8(str.c_str(), NULL);
				AssertMsgVa(l > 0, "laz_unicode_plugin_utf16_to_utf8()");
				// this will null-terminate
				utf8str.resize(l);
				laz_unicode_plugin_utf16_to_utf8(str.c_str(), &utf8str[0]);
			}
			else
			{
				int utf8_cnt = WideCharToMultiByte(CP_UTF8, MB_PRECOMPOSED, str.c_str(), -1, NULL, 0, NULL, NULL);
				//AssertMsgVa(utf8_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("WideCharToMultiByte()").c_str());
				// this will null-terminate
				utf8str.resize(utf8_cnt);
				WideCharToMultiByte(CP_UTF8, MB_PRECOMPOSED, str.c_str(), -1, &utf8str[0], utf8_cnt, NULL, NULL);
			}
#else
			int utf8_cnt = WideCharToMultiByte(CP_UTF8, MB_PRECOMPOSED, str.c_str(), -1, NULL, 0, NULL, NULL);
			AssertMsgVa(utf8_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("WideCharToMultiByte()").c_str());
			// this will null-terminate
			utf8str.resize(utf8_cnt);
			WideCharToMultiByte(CP_UTF8, MB_PRECOMPOSED, str.c_str(), -1, &utf8str[0], utf8_cnt, NULL, NULL);
#endif // !SKYRIMSE_LAZ_UNICODE_PLUGIN
			return utf8str;
		}

#ifndef __deprecated_utf8
		int sub_utf8_to_utf16(wchar_t* dest, size_t max_dest_chars, const char* source, size_t source_bytes)
		{
			if (source == nullptr)
				return 0;
			
			int result = -1;
			size_t input_utf8 = 0;
			size_t output_utf16 = 0;
			char cByte, cTemp;
			size_t char_len = 0;
			size_t uChar;

			if (source_bytes >= 3)
			{
				if (source[0] == 0xEF && source[0] == 0xBB && source[0] == 0xBF)
					input_utf8 = 3;
			}

			if (dest)
			{
				while ((output_utf16 < max_dest_chars) && (input_utf8 < source_bytes))
				{
					cByte = source[input_utf8];
					if (!(cByte & 0x80))
					{
						// One character US-ASCII, convert it to unicode
						
						dest[output_utf16] = (wchar_t)cByte;
						output_utf16++;
						input_utf8++;
					}
					else
					{
						cTemp = cByte;
						char_len = 0;

						while (cTemp & 0x80)
						{
							cTemp = (cTemp << 1) & 0xFE;
							char_len++;
						}

						// Test for the "CharLen" conforms UTF-8 string
						// This means the 10xxxxxx pattern.

						if ((input_utf8 + char_len - 1) > source_bytes)
							// Insuficient chars in string to decode
							// UTF-8 array. Fallback to single char.
							char_len = 1;
						
						for (size_t look = 1; look < char_len; look++)
						{
							if ((((source[input_utf8 + look]) & 0x80) != 0x80) || ((source[input_utf8 + look]) & 0x40))
							{
								// Invalid UTF-8 sequence, fallback.
							    char_len = look;
								break;
							}
						}
						
						uChar = 0xFFFF;
						switch (char_len)
						{
							case 1:
							{
								// Not valid UTF-8 sequence
								uChar = UNICODE_INVALID+1;
								break;
							}
							case 2:
							{
								// Two bytes UTF, convert it
								uChar = (source[input_utf8] & 0x1F) << 6;
								uChar |= source[input_utf8 + 1] & 0x3F;
								if (uChar < 0x80)
									// Not valid UTF-8 sequence
									uChar = UNICODE_INVALID+2;
								break;
							}
							case 3:
							{
								// Three bytes, convert it to unicode
								uChar = (source[input_utf8] & 0x0F) << 12;
								uChar |= (source[input_utf8 + 1] & 0x3F) << 6;
								uChar |= source[input_utf8 + 2] & 0x3F;
								if (uChar < 0x800 || uChar >= 0xFFFE || (uChar >= 0xD800 && (uChar <= 0xDFFF)))
									// Not valid UTF-8 sequence
									uChar = UNICODE_INVALID+3;
								break;
							}
							case 4:
							{
								// Four bytes, convert it to two unicode characters
								uChar = (source[input_utf8] & 0x07) << 18;
								uChar |= (source[input_utf8 + 1] & 0x3F) << 12;
								uChar |= (source[input_utf8 + 2] & 0x3F) << 6;
								uChar |= source[input_utf8 + 3] & 0x3F;
								if (uChar < 0x10000 || uChar > 0x10FFFF)
									// Not valid UTF-8 sequence
									uChar = UNICODE_INVALID+4;
								else
								{
									// only store pair if room
									uChar -= 0x10000;
									if (output_utf16 < max_dest_chars - 1)
									{
										dest[output_utf16] = (wchar_t)((uChar >> 10) + 0xD800);
										output_utf16++;
										uChar = (uChar & 0x3FF) + 0xDC00;
									}
									else
									{
										input_utf8 = input_utf8 + char_len;
										// don't store anything
										char_len = 0;
									}
								}

								break;
							}
							default:
								// Invalid UTF8 to unicode conversion,
								// mask it as invalid UNICODE too.
								uChar = UNICODE_INVALID+5;
								break;
						}

						if (char_len)
						{
							dest[output_utf16] = (wchar_t)uChar;
							output_utf16++;
						}

						input_utf8 += char_len;
					}
				}  // while ((output_utf16 < max_dest_chars) && (input_utf8 < source_bytes))

				result = (int)(output_utf16 + 1);
			}
			// calc length
			else
			{
				while (input_utf8 < source_bytes)
				{
					cByte = source[input_utf8];
					if (!(cByte & 0x80))
					{
						// One character US-ASCII, convert it to unicode
						output_utf16++;
						input_utf8++;
					}
					else
					{
						cTemp = cByte;
						char_len = 0;

						while (cTemp & 0x80)
						{
							cTemp = (cTemp << 1) & 0xFE;
							char_len++;
						}

						// Test for the "CharLen" conforms UTF-8 string
						// This means the 10xxxxxx pattern.

						if ((input_utf8 + char_len - 1) > source_bytes)
							// Insuficient chars in string to decode
							// UTF-8 array. Fallback to single char.
							char_len = 1;
						
						for (size_t look = 1; look < char_len; look++)
						{
							if ((((source[input_utf8 + look]) & 0x80) != 0x80) || ((source[input_utf8 + look]) & 0x40))
							{
								// Invalid UTF-8 sequence, fallback.
							    char_len = look;
								break;
							}
						}

						uChar = 0xFFFF;
						switch (char_len)
						{
							case 1:
							{
								// Not valid UTF-8 sequence
								uChar = UNICODE_INVALID;
								break;
							}
							case 2:
							{
								// Two bytes UTF, convert it
								uChar = (source[input_utf8] & 0x1F) << 6;
								uChar |= source[input_utf8 + 1] & 0x3F;
								if (uChar < 0x80)
									// Not valid UTF-8 sequence
									uChar = UNICODE_INVALID;
								break;
							}
							case 3:
							{
								// Three bytes, convert it to unicode
								uChar = (source[input_utf8] & 0x0F) << 12;
								uChar |= (source[input_utf8 + 1] & 0x3F) << 6;
								uChar |= source[input_utf8 + 2] & 0x3F;
								if (uChar < 0x800 || uChar >= 0xFFFE || (uChar >= 0xD800 && (uChar <= 0xDFFF)))
									// Not valid UTF-8 sequence
									uChar = UNICODE_INVALID;
								break;
							}
							case 4:
							{
								// Four bytes, convert it to two unicode characters
								uChar = (source[input_utf8] & 0x07) << 18;
								uChar |= (source[input_utf8 + 1] & 0x3F) << 12;
								uChar |= (source[input_utf8 + 2] & 0x3F) << 6;
								uChar |= source[input_utf8 + 3] & 0x3F;
								if (uChar < 0x10000 || uChar > 0x10FFFF)
									// Not valid UTF-8 sequence
									uChar = UNICODE_INVALID;
								else
									// extra character character
									output_utf16++;
								break;
							}
							default:
								// Invalid UTF8 to unicode conversion,
								// mask it as invalid UNICODE too.
								uChar = UNICODE_INVALID;
								break;
						}

						if (char_len)
						{
							dest[output_utf16] = (wchar_t)uChar;
							output_utf16++;
						}

						input_utf8 += char_len;
					}
				}  // while (input_utf8 < source_bytes)

				result = (int)(output_utf16 + 1);
			}

			return result;
		}
#endif // __deprecated_utf8

		utf16string UTILAPI Utf8ToUtf16(const utf8string &str)
		{
			utf16string utf16str;

			if (str.empty())
				return utf16str;

#ifndef __deprecated_utf8
			utf16str.resize(str.length());
			int iRes = sub_utf8_to_utf16(&utf16str[0], utf16str.length(), &str[0], str.length());
			AssertMsgVa(iRes > 0, "%s", "sub_utf8_to_utf16()");
			// this will null-terminate
			utf16str.resize(iRes);
#else
#if SKYRIMSE_LAZ_UNICODE_PLUGIN
			if (laz_unicode_plugin_utf8_to_utf16)
			{
				utf16str.resize(str.length());
				int l = laz_unicode_plugin_utf8_to_utf16(str.c_str(), &utf16str[0], false);
				AssertMsgVa(l > 0, "laz_unicode_plugin_utf8_to_utf16()");
				// this will null-terminate
				utf16str.resize(l);
			}
			else
			{
				int utf16_cnt = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str.c_str(), -1, NULL, 0);
				//AssertMsgVa(utf16_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("MultiByteToWideChar()").c_str());
				// this will null-terminate
				utf16str.resize(utf16_cnt);
				MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str.c_str(), -1, &utf16str[0], utf16_cnt);
			}
#else
			int utf16_cnt = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str.c_str(), -1, NULL, 0);
			AssertMsgVa(utf16_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("MultiByteToWideChar()").c_str());
			// this will null-terminate
			utf16str.resize(utf16_cnt);
			MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str.c_str(), -1, &utf16str[0], utf16_cnt);
#endif // !SKYRIMSE_LAZ_UNICODE_PLUGIN
#endif // !__deprecated_utf8

			return utf16str;
		}

		std::string UTILAPI Utf8ToAnsi(const utf8string &str)
		{
			std::string ansistr;

			if (str.empty())
				return ansistr;

#if SKYRIMSE_LAZ_UNICODE_PLUGIN
			if (laz_unicode_plugin_utf8_to_wincp)
			{
				ansistr.resize(str.length());
				int l = laz_unicode_plugin_utf8_to_wincp(str.c_str(), &ansistr[0], false);
				AssertMsgVa(l > 0, "laz_unicode_plugin_utf8_to_wincp()");
				// this will null-terminate
				ansistr.resize(l);
			}
			else
			{
				utf16string utf16str = Utf8ToUtf16(str);
				int ansi_cnt = WideCharToMultiByte(CP_ACP, 0, utf16str.c_str(), -1, NULL, 0, NULL, NULL);

				//AssertMsgVa(ansi_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("WideCharToMultiByte()").c_str());
				// this will null-terminate
				ansistr.resize(ansi_cnt);
				WideCharToMultiByte(CP_ACP, 0, utf16str.c_str(), -1, &ansistr[0], ansi_cnt, NULL, NULL);
			}
#else
			utf16string utf16str = utf8_to_utf16(str);
			int ansi_cnt = WideCharToMultiByte(CP_ACP, 0, utf16str.c_str(), -1, NULL, 0, NULL, NULL);

			AssertMsgVa(ansi_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("WideCharToMultiByte()").c_str());
			// this will null-terminate
			ansistr.resize(ansi_cnt);
			WideCharToMultiByte(CP_ACP, 0, utf16str.c_str(), -1, &ansistr[0], ansi_cnt, NULL, NULL);
#endif // SKYRIMSE_LAZ_UNICODE_PLUGIN
			return ansistr;
		}

		utf8string UTILAPI AnsiToUtf8(const std::string &str)
		{
			if (str.empty())
				return utf8string();

			
#if SKYRIMSE_LAZ_UNICODE_PLUGIN
			if (laz_unicode_plugin_wincp_to_utf8)
			{
				utf8string utf8str;
				int l = laz_unicode_plugin_wincp_to_utf8(str.c_str(), NULL);
				AssertMsgVa(l > 0, "laz_unicode_plugin_utf16_to_utf8()");
				// this will null-terminate
				utf8str.resize(l);
				laz_unicode_plugin_wincp_to_utf8(str.c_str(), &utf8str[0]);
				return utf8str;
			}
			else
			{
				utf16string utf16str;
				int utf16_cnt = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
				//AssertMsgVa(utf16_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("MultiByteToWideChar()").c_str());
				// this will null-terminate
				utf16str.resize(utf16_cnt);
				MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &utf16str[0], utf16_cnt);

				return Utf16ToUtf8(utf16str);
			}
#else
			utf16string utf16str;
			int utf16_cnt = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
			AssertMsgVa(utf16_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("MultiByteToWideChar()").c_str());
			// this will null-terminate
			utf16str.resize(utf16_cnt);
			MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &utf16str[0], utf16_cnt);

			return utf16_to_utf8(utf16str);
#endif // !SKYRIMSE_LAZ_UNICODE_PLUGIN
		}

		std::string UTILAPI Utf16ToAnsi(const utf16string &str)
		{
			if (str.empty())
				return std::string();

			return Utf8ToAnsi(Utf16ToUtf8(str));
		}

		utf16string UTILAPI AnsiToUtf16(const std::string &str)
		{
			if (str.empty())
				return utf16string();

			return Utf8ToUtf16(AnsiToUtf8(str));
		}

		std::string UTILAPI WideToAnsi(const std::wstring &str)
		{
			std::string ansistr;

			if (str.empty())
				return ansistr;

			int ansi_cnt = WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
			//AssertMsgVa(ansi_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("WideCharToMultiByte()").c_str());
			// this will null-terminate
			ansistr.resize(ansi_cnt);
			WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, &ansistr[0], ansi_cnt, NULL, NULL);

			return ansistr;
		}

		std::wstring UTILAPI AnsiToWide(const std::string &str)
		{
			std::wstring widestr;
			
			if (str.empty())
				return widestr;

			int wide_cnt = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), (int)str.length(), NULL, 0);
			//AssertMsgVa(wide_cnt > 0, "%s", XUtil::Str::GetLastErrorToStr("MultiByteToWideChar()").c_str());
			// this will null-terminate
			widestr.resize(wide_cnt);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), (int)str.length(), &widestr[0], wide_cnt);

			return widestr;
		}
	}
}