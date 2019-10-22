#include "../../common.h"
#include <regex>
#include <tbb/concurrent_vector.h>
#include <Richedit.h>
#include "EditorUI.h"
#include "LogWindow.h"

HWND g_LogHwnd;
HANDLE g_LogPipeReader;
HANDLE g_LogPipeWriter;
FILE *g_LogFile;

tbb::concurrent_vector<const char *> g_LogPendingMessages;
std::set<uint64_t> g_LogMessageBlacklist;

bool EditorUI_CreateLogWindow()
{
	if (!LoadLibraryA("MSFTEDIT.dll"))
		return false;

	// Build warning blacklist stored in the ini file
	EditorUI_GenerateWarningBlacklist();

	// File output
	const std::string logPath = g_INI.Get("CreationKit_Log", "OutputFile", "none");

	if (strcmp(logPath.c_str(), "none") != 0)
	{
		if (fopen_s(&g_LogFile, logPath.c_str(), "w") != 0)
			g_LogFile = nullptr;

		AssertMsgVa(g_LogFile, "Unable to open the log file '%s' for writing. To disable, set the 'OutputFile' INI option to 'none'.", logPath.c_str());

		atexit([]()
		{
			if (g_LogFile)
				fclose(g_LogFile);
		});
	}

	// Window output
	HINSTANCE instance = (HINSTANCE)GetModuleHandle(nullptr);

	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hInstance = instance;
	wc.hIcon = LoadIcon(instance, MAKEINTRESOURCE(0x13E));
	wc.hIconSm = wc.hIcon;
	wc.lpfnWndProc = EditorUI_LogWndProc;
	wc.lpszClassName = TEXT("RTEDITLOG");
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

	if (!RegisterClassEx(&wc))
		return false;

	g_LogHwnd = CreateWindowEx(0, TEXT("RTEDITLOG"), TEXT("Log"), WS_OVERLAPPEDWINDOW, 64, 64, 1024, 480, nullptr, nullptr, instance, nullptr);

	if (!g_LogHwnd)
		return false;

	// Poll every 100ms for new lines
	SetTimer(g_LogHwnd, UI_LOG_CMD_ADDTEXT, 100, nullptr);
	ShowWindow(g_LogHwnd, SW_SHOW);
	UpdateWindow(g_LogHwnd);

	return true;
}

bool EditorUI_CreateStdoutListener()
{
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = nullptr;

	if (!CreatePipe(&g_LogPipeReader, &g_LogPipeWriter, &saAttr, 0))
		return false;

	// Ensure the read handle to the pipe for STDOUT is not inherited
	if (!SetHandleInformation(g_LogPipeReader, HANDLE_FLAG_INHERIT, 0))
		return false;

	std::thread pipeReader([]()
	{
		char logBuffer[16384];
		memset(logBuffer, 0, sizeof(logBuffer));

		while (true)
		{
			char buffer[4096];
			memset(buffer, 0, sizeof(buffer));

			DWORD bytesRead;
			bool succeeded = ReadFile(g_LogPipeReader, buffer, ARRAYSIZE(buffer) - 1, &bytesRead, nullptr) != 0;

			// Bail if there's nothing left or the process exited
			if (!succeeded || bytesRead <= 0)
				break;

			strcat_s(logBuffer, buffer);

			// Flush on every newline and skip empty/whitespace strings
			while (char *end = strchr(logBuffer, '\n'))
			{
				*end = '\0';
				size_t len = (size_t)(end - logBuffer);

				while (strchr(logBuffer, '\r'))
					*strchr(logBuffer, '\r') = ' ';

				if (len > 0 && (len > 1 || logBuffer[0] != ' '))
					EditorUI_Warning(6, "%s", logBuffer);

				strcpy_s(logBuffer, end + 1);
			}
		}

		CloseHandle(g_LogPipeReader);
		CloseHandle(g_LogPipeWriter);
	});

	pipeReader.detach();
	return true;
}

void EditorUI_GenerateWarningBlacklist()
{
	if (!g_INI.GetBoolean("CreationKit", "WarningBlacklist", false))
		return;

	// Keep reading entries until an empty one is hit
	for (uint32_t i = 0;; i++)
	{
		std::string& message = g_INI.Get("CreationKit_Warnings", "W" + std::to_string(i), "");

		if (message.length() <= 0)
			break;

		// Un-escape newline and carriage return characters
		for (size_t i; (i = message.find("\\n")) != std::string::npos;)
			message.replace(i, 2, "\n");

		for (size_t i; (i = message.find("\\r")) != std::string::npos;)
			message.replace(i, 2, "\r");

		g_LogMessageBlacklist.emplace(XUtil::MurmurHash64A(message.c_str(), message.length()));
	}
}

HWND EditorUI_GetLogWindow()
{
	return g_LogHwnd;
}

HANDLE EditorUI_GetStdoutListenerPipe()
{
	return g_LogPipeWriter;
}

LRESULT CALLBACK EditorUI_LogWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND richEditHwnd;
	static bool autoScroll;

	switch (Message)
	{
	case WM_CREATE:
	{
		const CREATESTRUCT *info = (CREATESTRUCT *)lParam;

		// Create the rich edit control (https://docs.microsoft.com/en-us/windows/desktop/Controls/rich-edit-controls)
		uint32_t style = WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_LEFT | ES_NOHIDESEL | ES_AUTOVSCROLL | ES_READONLY;

		richEditHwnd = CreateWindowExW(0, MSFTEDIT_CLASS, L"", style, 0, 0, info->cx, info->cy, Hwnd, nullptr, info->hInstance, nullptr);
		autoScroll = true;

		if (!richEditHwnd)
			return -1;

		// Set default position
		int winW = g_INI.GetInteger("CreationKit_Log", "Width", info->cx);
		int winH = g_INI.GetInteger("CreationKit_Log", "Height", info->cy);

		MoveWindow(Hwnd, info->x, info->y, winW, winH, FALSE);

		// Set a better font
		CHARFORMAT2A format;
		memset(&format, 0, sizeof(format));

		// Convert Twips to points (1 point = 20 Twips)
		int pointSize = g_INI.GetInteger("CreationKit_Log", "FontSize", 10) * 20;

		format.cbSize = sizeof(format);
		format.dwMask = CFM_FACE | CFM_SIZE | CFM_WEIGHT;
		format.yHeight = pointSize;
		strcpy_s(format.szFaceName, g_INI.Get("CreationKit_Log", "Font", "Consolas").c_str());
		format.wWeight = (WORD)g_INI.GetInteger("CreationKit_Log", "FontWeight", FW_NORMAL);
		SendMessageA(richEditHwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);

		// Subscribe to EN_MSGFILTER and EN_SELCHANGE
		SendMessageA(richEditHwnd, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_SELCHANGE);
	}
	return 0;

	case WM_DESTROY:
		DestroyWindow(richEditHwnd);
		return 0;

	case WM_SIZE:
	{
		int w = LOWORD(lParam);
		int h = HIWORD(lParam);
		MoveWindow(richEditHwnd, 0, 0, w, h, TRUE);
	}
	break;

	case WM_ACTIVATE:
	{
		if (wParam != WA_INACTIVE)
			SetFocus(richEditHwnd);
	}
	return 0;

	case WM_CLOSE:
		ShowWindow(Hwnd, SW_HIDE);
		return 0;

	case WM_NOTIFY:
	{
		static uint64_t lastDoubleClickTime;
		LPNMHDR notification = (LPNMHDR)lParam;

		if (notification->code == EN_MSGFILTER)
		{
			auto msgFilter = (MSGFILTER *)notification;

			if (msgFilter->msg == WM_LBUTTONDBLCLK)
				lastDoubleClickTime = GetTickCount64();
		}
		else if (notification->code == EN_SELCHANGE)
		{
			auto selChange = (SELCHANGE *)lParam;

			// Mouse double click with a valid selection -> try to parse form id
			if ((GetTickCount64() - lastDoubleClickTime > 1000) || abs(selChange->chrg.cpMax - selChange->chrg.cpMin) <= 0)
				break;

			if (selChange->chrg.cpMin == 0 && selChange->chrg.cpMax == -1)
				break;

			// Get the line number from the selected range
			LRESULT lineIndex = SendMessageA(richEditHwnd, EM_LINEFROMCHAR, selChange->chrg.cpMin, 0);

			char lineData[4096];
			*(uint16_t *)&lineData[0] = ARRAYSIZE(lineData);

			LRESULT charCount = SendMessageA(richEditHwnd, EM_GETLINE, lineIndex, (LPARAM)&lineData);

			if (charCount > 0)
			{
				lineData[charCount - 1] = '\0';

				// Capture each form id with regex "(XXXXXXXX)"
				static const std::regex formIdRegex("\\(([1234567890abcdefABCDEF]*?)\\)");
				std::smatch sm;

				for (std::string line = lineData; std::regex_search(line, sm, formIdRegex); line = sm.suffix())
				{
					// Parse to integer, then bring up the menu
					uint32_t id = strtoul(sm[1].str().c_str(), nullptr, 16);

					auto GetFormById = (__int64(__fastcall *)(uint32_t))(g_ModuleBase + 0x16B8780);
					__int64 form = GetFormById(id);

					if (form)
						(*(void(__fastcall **)(__int64, HWND, __int64, __int64))(*(__int64 *)form + 720i64))(form, EditorUI_GetMainWindow(), 0, 1);
				}
			}
		}
	}
	break;

	case WM_TIMER:
	{
		if (wParam != UI_LOG_CMD_ADDTEXT)
			break;

		if (g_LogPendingMessages.size() <= 0)
			break;

		return EditorUI_LogWndProc(Hwnd, UI_LOG_CMD_ADDTEXT, 0, 0);
	}
	return 0;

	case UI_LOG_CMD_ADDTEXT:
	{
		SendMessageA(richEditHwnd, WM_SETREDRAW, FALSE, 0);

		// Save old position if not scrolling
		POINT scrollRange;

		if (!autoScroll)
			SendMessageA(richEditHwnd, EM_GETSCROLLPOS, 0, (WPARAM)&scrollRange);

		// Get a copy of all elements and clear the global list
		tbb::concurrent_vector<const char *> messages;

		if (!wParam)
			messages.swap(g_LogPendingMessages);
		else
			messages.push_back((const char *)wParam);

		for (const char *message : messages)
		{
			// Move caret to the end, then write
			CHARRANGE range;
			range.cpMin = LONG_MAX;
			range.cpMax = LONG_MAX;

			SendMessageA(richEditHwnd, EM_EXSETSEL, 0, (LPARAM)&range);
			SendMessageA(richEditHwnd, EM_REPLACESEL, FALSE, (LPARAM)message);

			free((void *)message);
		}

		if (!autoScroll)
			SendMessageA(richEditHwnd, EM_SETSCROLLPOS, 0, (WPARAM)&scrollRange);

		SendMessageA(richEditHwnd, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(richEditHwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
	}
	return 0;

	case UI_LOG_CMD_CLEARTEXT:
	{
		char emptyString[1];
		emptyString[0] = '\0';

		SendMessageA(richEditHwnd, WM_SETTEXT, 0, (LPARAM)&emptyString);
	}
	return 0;

	case UI_LOG_CMD_AUTOSCROLL:
		autoScroll = (bool)wParam;
		return 0;
	}

	return DefWindowProc(Hwnd, Message, wParam, lParam);
}

void EditorUI_LogVa(const char *Format, va_list Va)
{
	char buffer[2048];
	int len = _vsnprintf_s(buffer, _TRUNCATE, Format, Va);

	if (len <= 0)
		return;

	if (g_LogMessageBlacklist.count(XUtil::MurmurHash64A(buffer, len)))
		return;

	if (len >= 2 && buffer[len - 1] != '\n')
		strcat_s(buffer, "\n");

	if (g_LogFile)
	{
		fputs(buffer, g_LogFile);
		fflush(g_LogFile);
	}

	if (g_LogPendingMessages.size() < 50000)
		g_LogPendingMessages.push_back(_strdup(buffer));
}

void EditorUI_Log(const char *Format, ...)
{
	va_list va;

	va_start(va, Format);
	EditorUI_LogVa(Format, va);
	va_end(va);
}

void EditorUI_Warning(int Type, const char *Format, ...)
{
	static const char *typeList[30] =
	{
		"DEFAULT",
		"COMBAT",
		"ANIMATION",
		"AI",
		"SCRIPTS",
		"SAVELOAD",
		"DIALOGUE",
		"QUESTS",
		"PACKAGES",
		"EDITOR",
		"MODELS",
		"TEXTURES",
		"PLUGINS",
		"MASTERFILE",
		"FORMS",
		"MAGIC",
		"SHADERS",
		"RENDERING",
		"PATHFINDING",
		"MENUS",
		"AUDIO",
		"CELLS",
		"HAVOK",
		"FACEGEN",
		"WATER",
		"INGAME",
		"MEMORY",
		"PERFORMANCE",
		"JOBS",
		"SYSTEM"
	};

	char buffer[2048];
	va_list va;

	va_start(va, Format);
	_vsnprintf_s(buffer, _TRUNCATE, Format, va);
	va_end(va);

	EditorUI_Log("%s: %s", typeList[Type], buffer);
}

void EditorUI_WarningUnknown1(const char *Format, ...)
{
	va_list va;

	va_start(va, Format);
	EditorUI_LogVa(Format, va);
	va_end(va);
}

void EditorUI_WarningUnknown2(__int64 Unused, const char *Format, ...)
{
	va_list va;

	va_start(va, Format);
	EditorUI_LogVa(Format, va);
	va_end(va);
}

void EditorUI_Assert(const char *File, int Line, const char *Message, ...)
{
	if (!Message)
		Message = "<No message>";

	char buffer[2048];
	va_list va;

	va_start(va, Message);
	_vsnprintf_s(buffer, _TRUNCATE, Message, va);
	va_end(va);

	EditorUI_Log("ASSERTION: %s (%s line %d)", buffer, File, Line);
}