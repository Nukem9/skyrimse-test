#include "../common.h"

#define STARTS_WITH(buf, str) (strstr(buf, str) == (buf))

void LogVa(const char *Format, va_list va)
{
	char buffer[2048];
	vsnprintf_s(buffer, _TRUNCATE, Format, va);

	ui::log::Add("%s\n", buffer);
}

bool LogBNet(const char *String)
{
	ui::log::Add("%s\n", String);
	return true;
}

void NavmeshBuilderLog(__int64 a1, const char *Format, ...)
{
	if (!ui::opt::LogNavmeshProcessing)
		return;

	va_list va;
	va_start(va, Format);

	char buffer[2048];
	vsnprintf_s(buffer, _TRUNCATE, Format, va);

	ui::log::Add("[NAV] %s\n", buffer);
	va_end(va);
}

void LogFunc4(__int64 a1, const char *Format, ...)
{
	va_list va;
	va_start(va, Format);
	LogVa(Format, va);
	va_end(va);
}

__int64 LogFunc3(__int64 a1, const char *Format, ...)
{
	va_list va;
	va_start(va, Format);
	LogVa(Format, va);
	va_end(va);

	return 0;
}

void LogFunc2(const char *Format, ...)
{
	bool isQuest =
		STARTS_WITH(Format, " %s is now starting a") ||
		STARTS_WITH(Format, " %s is trying to start a") ||
		STARTS_WITH(Format, " %s  walkaway topic") ||
		STARTS_WITH(Format, " %s advancing the script") ||
		STARTS_WITH(Format, " %s asking for random") ||
		STARTS_WITH(Format, " %s got a quest back") ||
		STARTS_WITH(Format, " %s did not get a quest") ||
		STARTS_WITH(Format, " %s failed to get") ||
		STARTS_WITH(Format, " %s is not being allowed to advance");

	if (!ui::opt::LogQuestSceneActions && isQuest)
		return;

	va_list va;
	va_start(va, Format);

	char buffer[2048];
	vsnprintf_s(buffer, _TRUNCATE, Format, va);

	ui::log::Add(isQuest ? "[QST]%s\n" : "%s\n", buffer);
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
		STARTS_WITH(Format, "Queued ") ||
		STARTS_WITH(Format, "%s (%08X)[%d]/%s") ||
		STARTS_WITH(Format, "alias %s on") ||
		STARTS_WITH(Format, "%d/%d/%02d") ||
		STARTS_WITH(Format, "%u.%u.%I64i.%u.%"))
		return len;

	ui::log::Add("%s\n", DstBuf);
	return len;
}

void PatchLogging()
{
	Detours::X64::DetourFunction(g_ModuleBase + 0x1CDFE0, (uintptr_t)&LogFunc1);
	Detours::X64::DetourFunction(g_ModuleBase + 0x1660D0, (uintptr_t)&LogFunc2);
	Detours::X64::DetourFunction(g_ModuleBase + 0x578F70, (uintptr_t)&LogFunc2);
	Detours::X64::DetourFunction(g_ModuleBase + 0x179C40, (uintptr_t)&LogFunc3);
	Detours::X64::DetourFunction(g_ModuleBase + 0x5844F0, (uintptr_t)&LogFunc4);
	Detours::X64::DetourFunction(g_ModuleBase + 0x10EF900, (uintptr_t)&NavmeshBuilderLog);
	Detours::X64::DetourFunction(g_ModuleBase + 0x116C9E0, (uintptr_t)&LogBNet);
	Detours::X64::DetourFunction(g_ModuleBase + 0x142550, (uintptr_t)&hk_sprintf_s);
}