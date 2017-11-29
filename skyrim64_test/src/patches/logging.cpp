#include "../common.h"

#define STARTS_WITH(buf, str) (strstr(buf, str) == (buf))

void LogVa(const char *Format, va_list va)
{
	char buffer[2048];
	vsnprintf_s(buffer, _TRUNCATE, Format, va);

	ui::log::Add("%s\n", buffer);
}

__int64 LogFunc3(__int64 a1, const char *Format, ...)
{
	va_list va;
	va_start(va, Format);

	LogVa(Format, va);
	return 0;
}

void LogFunc2(const char *Format, ...)
{
	if (STARTS_WITH(Format, " %s asking for random") ||
		STARTS_WITH(Format, " %s got a quest back") ||
		STARTS_WITH(Format, " %s failed to get"))
		return;

	va_list va;
	va_start(va, Format);
	LogVa(Format, va);
	va_end(va);
}

int LogFunc1(const char *Format, ...)
{
	va_list va;
	va_start(va, Format);
	LogVa(Format, va);
	va_end(va);

	return 0;
}

int hk_sprintf_s(char *DstBuf, size_t SizeInBytes, const char *Format, ...)
{
	va_list va;
	va_start(va, Format);
	int len = vsprintf_s(DstBuf, SizeInBytes, Format, va);
	va_end(va);

	if (strlen(Format) <= 20 ||
		STARTS_WITH(DstBuf, "data\\") ||
		STARTS_WITH(DstBuf, "Data\\") ||
		STARTS_WITH(DstBuf, "mesh\\") ||
		STARTS_WITH(DstBuf, "Meshes\\") ||
		STARTS_WITH(DstBuf, "Textures\\") ||
		STARTS_WITH(DstBuf, "Interface\\") ||
		STARTS_WITH(DstBuf, "SHADERSFX") ||
		STARTS_WITH(Format, "%s (%08X)[%d]/%s") ||
		STARTS_WITH(Format, "alias %s on"))
		return len;

	ui::log::Add("%s\n", DstBuf);
	return len;
}

void PatchLogging()
{
	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x1CDFE0), (PBYTE)&LogFunc1);
	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x1660D0), (PBYTE)&LogFunc2);
	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x578F70), (PBYTE)&LogFunc2);
	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x179C40), (PBYTE)&LogFunc3);
	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0x142550), (PBYTE)&hk_sprintf_s);
}