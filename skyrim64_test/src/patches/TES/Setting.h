#pragma once

#include <stdio.h>
#include "BSTList.h"

union SETTING_VALUE
{
	const char *str;
	int i;
	unsigned int u;
	float f;
	bool b;
	char c;
	unsigned char h;

	struct
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	} rgba;
};

class Setting
{
public:
	enum SETTING_TYPE
	{
		ST_BINARY = 0x0,
		ST_CHAR = 0x1,
		ST_UCHAR = 0x2,
		ST_INT = 0x3,
		ST_UINT = 0x4,
		ST_FLOAT = 0x5,
		ST_STRING = 0x6,
		ST_RGB = 0x7,
		ST_RGBA = 0x8,
		ST_NONE = 0x9,
	};

	static SETTING_TYPE TypeFromPrefix(const char *Prefix)
	{
		switch (Prefix[0])
		{
		case 'a': return ST_RGBA;
		case 'b': return ST_BINARY;
		case 'c': return ST_CHAR;
		case 'f': return ST_FLOAT;
		case 'h': return ST_UCHAR;
		case 'i': return ST_INT;
		case 'r': return ST_RGB;
		case 'S': return ST_STRING;
		case 's': return ST_STRING;
		case 'u': return ST_UINT;
		}

		return ST_NONE;
	}

	void GetAsString(char *Buffer, size_t BufferLen)
	{
		switch (TypeFromPrefix(pKey))
		{
		case ST_BINARY:
			strncpy_s(Buffer, BufferLen, uValue.b ? "true" : "false", _TRUNCATE);
			break;

		case ST_CHAR:
			_snprintf_s(Buffer, BufferLen, _TRUNCATE, "%d", (int)uValue.c);
			break;

		case ST_UCHAR:
			_snprintf_s(Buffer, BufferLen, _TRUNCATE, "%u", (uint32_t)uValue.h);
			break;

		case ST_INT:
			_snprintf_s(Buffer, BufferLen, _TRUNCATE, "%d", uValue.i);
			break;

		case ST_UINT:
			_snprintf_s(Buffer, BufferLen, _TRUNCATE, "%u", uValue.u);
			break;

		case ST_FLOAT:
			_snprintf_s(Buffer, BufferLen, _TRUNCATE, "%.4f", uValue.f);
			break;

		case ST_STRING:
			strncpy_s(Buffer, BufferLen, uValue.str, _TRUNCATE);
			break;

		case ST_RGB:
			_snprintf_s(Buffer, BufferLen, _TRUNCATE, "%hhu %hhu %hhu", uValue.rgba.r, uValue.rgba.g, uValue.rgba.b);
			break;

		case ST_RGBA:
			_snprintf_s(Buffer, BufferLen, _TRUNCATE, "%hhu %hhu %hhu %hhu", uValue.rgba.r, uValue.rgba.g, uValue.rgba.b, uValue.rgba.a);
			break;

		case ST_NONE:
			strcpy_s(Buffer, BufferLen, "");
			break;
		}
	}

	bool SetFromString(const char *Input)
	{
		SETTING_VALUE value;

		switch (TypeFromPrefix(pKey))
		{
		case ST_BINARY:
			if (!_stricmp(Input, "true") || !_stricmp(Input, "1"))
				value.b = true;
			else if (!_stricmp(Input, "false") || !_stricmp(Input, "0"))
				value.b = false;
			else
				return false;
			break;

		case ST_CHAR:
			value.c = (char)atoi(Input);
			break;

		case ST_UCHAR:
			value.h = (unsigned char)atoi(Input);
			break;

		case ST_INT:
			value.i = (unsigned char)atoi(Input);
			break;

		case ST_UINT:
			value.u = (unsigned int)strtoul(Input, nullptr, 10);
			break;

		case ST_FLOAT:
			value.f = (float)atof(Input);
			break;

		case ST_STRING:
			// I don't know how to handle these as far as the engine is concerned
			return false;
			break;

		case ST_RGB:
			if (sscanf_s(Input, "%hhu %hhu %hhu", &value.rgba.r, &value.rgba.g, &value.rgba.b) != 3)
				return false;
			break;

		case ST_RGBA:
			if (sscanf_s(Input, "%hhu %hhu %hhu %hhu", &value.rgba.r, &value.rgba.g, &value.rgba.b, &value.rgba.a) != 4)
				return false;
			break;

		case ST_NONE:
			return false;
		}

		uValue = value;
		return true;
	}

	virtual ~Setting();
	virtual bool IsPrefSetting();

	SETTING_VALUE uValue;
	const char *pKey;
};
static_assert_offset(Setting, uValue, 0x8);
static_assert_offset(Setting, pKey, 0x10);

template<typename T>
struct SettingT : public Setting
{
};

template<typename T>
class SettingCollection
{
	char pSettingFile[260];
	void *pHandle;
};

template<typename T>
class SettingCollectionList : public SettingCollection<T>
{
public:
	virtual ~SettingCollectionList();
	virtual void AddSetting(Setting *S);
	virtual void RemoveSetting(Setting *S);
	virtual bool WriteSetting(Setting *S) = 0;
	virtual bool ReadSetting(Setting *S) = 0;
	virtual bool Open();
	virtual bool Close();
	virtual bool ReadSettingsFromProfile();
	virtual bool WriteSettings();
	virtual bool ReadSettings();

	BSSimpleList<T *> SettingsA;
};
static_assert_offset(SettingCollectionList<Setting>, SettingsA, 0x118);

class INISettingCollection : public SettingCollectionList<Setting>
{
};

AutoPtr(INISettingCollection *, INISettingCollectionSingleton, 0x3043758);