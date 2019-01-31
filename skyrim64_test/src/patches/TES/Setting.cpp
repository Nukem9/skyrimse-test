#include "../../common.h"
#include "Setting.h"

#if 0
#define INI_ALLOW_MULTILINE 0
#define INI_USE_STACK 0
#define INI_MAX_LINE 4096
#include "../INIReader.h"
#endif

DefineIniSetting(sLanguage, General);

Setting::SETTING_TYPE Setting::DataType(const char *Prefix)
{
	switch (Prefix[0])
	{
	case 'b': return ST_BINARY;
	case 'c': return ST_CHAR;
	case 'h': return ST_UCHAR;
	case 'i': return ST_INT;
	case 'u': return ST_UINT;
	case 'f': return ST_FLOAT;
	case 'S': return ST_STRING;
	case 's': return ST_STRING;
	case 'r': return ST_RGB;
	case 'a': return ST_RGBA;
	}

	return ST_NONE;
}

void Setting::GetAsString(char *Buffer, size_t BufferLen)
{
	switch (DataType(pKey))
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
		strncpy_s(Buffer, BufferLen, uValue.str ? uValue.str : "", _TRUNCATE);
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

bool Setting::SetFromString(const char *Input)
{
	SETTING_VALUE value;

	switch (DataType(pKey))
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
		value.i = atoi(Input);
		break;

	case ST_UINT:
		value.u = (unsigned int)strtoul(Input, nullptr, 10);
		break;

	case ST_FLOAT:
		value.f = (float)atof(Input);
		break;

	case ST_STRING:
		(*this) = Input;
		return true;

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

bool INISettingCollection::hk_ReadSetting(Setting *S)
{
	if (!S->pKey)
		return false;

	bool result = false;
	bool openHandle = pHandle != nullptr;

	if (!openHandle)
		Open(true);

	AssertMsg(false, "Code is experimental, may cause subtle errors");

#if 0
	const INIReader *reader = (const INIReader *)pHandle;

	if (reader)
	{
		char mainKey[MAX_KEY_LENGTH];
		MainKey(S, mainKey);

		char subKey[MAX_SUBKEY_LENGTH];
		SubKey(S, subKey);

		switch (Setting::DataType(S->pKey))
		{
		case Setting::ST_BINARY:
			S->uValue.b = reader->GetBoolean(mainKey, subKey, S->uValue.b) != false;
			break;

		case Setting::ST_CHAR:
			S->uValue.c = (char)reader->GetInteger(mainKey, subKey, S->uValue.c);
			break;

		case Setting::ST_UCHAR:
			S->uValue.h = (unsigned char)reader->GetInteger(mainKey, subKey, S->uValue.h);
			break;

		case Setting::ST_INT:
		case Setting::ST_UINT:
			S->uValue.u = reader->GetInteger(mainKey, subKey, S->uValue.u);
			break;

		case Setting::ST_FLOAT:
			S->uValue.f = (float)reader->GetReal(mainKey, subKey, S->uValue.f);
			break;

		case Setting::ST_STRING:
		{
			if (!strcmp(mainKey, "LANGUAGE"))
			{
				// Remap the main key to language-specific sections
				strcpy_s(mainKey, MAX_KEY_LENGTH, reader->Get("General", "sLanguage", sLanguage->uValue.str).c_str());
			}

			(*S) = reader->Get(mainKey, subKey, S->uValue.str).c_str();
		}
		break;

		case Setting::ST_RGB:
		{
			std::string& value = reader->Get(mainKey, subKey, "");

			if (value.length() > 0)
			{
				uint32_t rgb[3];
				result = sscanf_s(value.c_str(), "%u,%u,%u", &rgb[0], &rgb[1], &rgb[2]) == 3;
				S->uValue.rgba.r = rgb[0];
				S->uValue.rgba.g = rgb[1];
				S->uValue.rgba.b = rgb[2];
				S->uValue.rgba.a = 255;
			}
		}
		break;

		case Setting::ST_RGBA:
		{
			std::string& value = reader->Get(mainKey, subKey, "");

			if (value.length() > 0)
			{
				uint32_t rgba[4];
				result = sscanf_s(value.c_str(), "%u,%u,%u,%u", &rgba[0], &rgba[1], &rgba[2], &rgba[3]) == 4;
				S->uValue.rgba.r = rgba[0];
				S->uValue.rgba.g = rgba[1];
				S->uValue.rgba.b = rgba[2];
				S->uValue.rgba.a = rgba[3];
			}
		}
		break;

		case Setting::ST_NONE:
			AssertMsg(false, "Trying to get an INI value for an invalid key");
			break;
		}
	}
#endif

	if (!openHandle)
		Close();

	return result;
}

bool INISettingCollection::hk_Open(bool OpenDuringRead)
{
#if 0
	INIReader *reader = new INIReader(pSettingFile);

	if (reader->ParseError() != 0)
	{
		delete reader;
		reader = nullptr;
	}

	pHandle = reader;
#endif

	//
	// Cut down the number of GetPrivateProfileX calls by an order of magnitude. Normally the game checks
	// an INI for every ESP/ESM, which then loops over every single INI variable.
	//
	if (GetFileAttributes(pSettingFile) != INVALID_FILE_ATTRIBUTES)
		pHandle = this;
	else
		pHandle = nullptr;

	return true;
}

bool INISettingCollection::hk_Close()
{
#if 0
	if (pHandle)
		delete static_cast<INIReader *>(pHandle);
#endif

	pHandle = nullptr;
	return true;
}

Setting *INISettingCollection::FindSetting(const char *Key) const
{
	for (auto *s = SettingsA.QNext(); s; s = s->QNext())
	{
		if (!_stricmp(Key, s->QItem()->pKey))
			return s->QItem();
	}

	return nullptr;
}

void INISettingCollection::DumpSettingIDAScript(FILE *File) const
{
	for (auto *s = SettingsA.QNext(); s; s = s->QNext())
	{
		void *addr = (void *)((uintptr_t)&s->QItem()->uValue - g_ModuleBase + 0x140000000);

		switch (Setting::DataType(s->QItem()->pKey))
		{
		case Setting::ST_BINARY: fprintf(File, "create_byte(0x%p);\n", addr); break;
		case Setting::ST_CHAR: fprintf(File, "create_byte(0x%p);\n", addr); break;
		case Setting::ST_UCHAR: fprintf(File, "create_byte(0x%p);\n", addr); break;
		case Setting::ST_INT: fprintf(File, "create_dword(0x%p);\n", addr); break;
		case Setting::ST_UINT: fprintf(File, "create_dword(0x%p);\n", addr); break;
		case Setting::ST_FLOAT: fprintf(File, "create_float(0x%p);\n", addr); break;
		case Setting::ST_STRING: fprintf(File, "create_qword(0x%p); op_plain_offset(0x%p, 0, 0);\n", addr, addr); break;
		case Setting::ST_RGB: fprintf(File, "create_byte(0x%p); make_array(0x%p, 0x3);\n", addr, addr); break;
		case Setting::ST_RGBA: fprintf(File, "create_byte(0x%p); make_array(0x%p, 0x4);\n", addr, addr); break;
		case Setting::ST_NONE: fprintf(File, "// UNIMPLEMENTED CASE!\n"); break;
		}

		// Replace 'setting:category' with 'setting_category'
		char temp[1024];
		strcpy_s(temp, s->QItem()->pKey);

		for (int i = 0; i < ARRAYSIZE(temp); i++)
		{
			if (temp[i] == ':' || temp[i] == ' ')
				temp[i] = '_';
		}

		fprintf(File, "set_name(0x%p, \"%s\");\n", addr, temp);
	}
}

void INISettingCollection::MainKey(const Setting *S, char *Buffer) const
{
	if (Buffer)
	{
		const char *ptr = "MAIN";

		if (S->pKey)
		{
			const char *v = strchr(S->pKey, ':');

			if (v)
				ptr = v + 1;
		}

		strcpy_s(Buffer, MAX_KEY_LENGTH, ptr);
	}
}

void INISettingCollection::SubKey(const Setting *S, char *Buffer) const
{
	if (Buffer)
	{
		Buffer[0] = '\0';

		if (S->pKey)
		{
			const char *ptr = strchr(S->pKey, ':');
			size_t len;

			if (ptr)
				len = (uintptr_t)ptr - (uintptr_t)S->pKey;
			else
				len = strlen(S->pKey);

			strncpy_s(Buffer, len + 1, S->pKey, len);
			Buffer[len] = '\0';
		}
	}
}

SettingResolverHack::SettingResolverHack(const char *Key) : m_Key(Key), m_Ptr(nullptr)
{
}

Setting *SettingResolverHack::operator->()
{
	if (!m_Ptr)
	{
		// Sometimes it's stored in SkyrimPrefs.ini or sometimes Skyrim.ini
		m_Ptr = INIPrefSettingCollectionSingleton->FindSetting(m_Key);

		if (!m_Ptr)
			m_Ptr = INISettingCollectionSingleton->FindSetting(m_Key);

		AssertMsgVa(m_Ptr, "Setting '%s' wasn't found!", m_Key);
	}

	return m_Ptr;
}
