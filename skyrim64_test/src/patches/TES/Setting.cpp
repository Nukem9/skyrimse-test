#include "../../common.h"
#include "Setting.h"

Setting::SETTING_TYPE Setting::TypeFromPrefix(const char *Prefix)
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

void Setting::GetAsString(char *Buffer, size_t BufferLen)
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

bool Setting::SetFromString(const char *Input)
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
		value.i = atoi(Input);
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

Setting *INISettingCollection::FindSetting(const char *Key)
{
	for (auto *s = SettingsA.QNext(); s; s = s->QNext())
	{
		if (!_stricmp(Key, s->QItem()->pKey))
			return s->QItem();
	}

	return nullptr;
}

void INISettingCollection::DumpSettingIDAScript(FILE *File)
{
	for (auto *s = SettingsA.QNext(); s; s = s->QNext())
	{
		void *addr = (void *)((uintptr_t)&s->QItem()->uValue - g_ModuleBase + 0x140000000);

		switch (Setting::TypeFromPrefix(s->QItem()->pKey))
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

		while (strchr(temp, ':'))
			*strchr(temp, ':') = '_';

		while (strchr(temp, ' '))
			*strchr(temp, ' ') = '_';

		fprintf(File, "set_name(0x%p, \"%s\");\n", addr, temp);
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
