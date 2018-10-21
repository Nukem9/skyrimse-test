#include <tbb/concurrent_vector.h>
#include <regex>
#include <Richedit.h>
#include <CommCtrl.h>
#include "../../common.h"
#include "../../typeinfo/ms_rtti.h"
#include "EditorUI.h"

#pragma comment(lib, "comctl32.lib")

#define UI_CMD_ADDLOGTEXT	(WM_APP + 1)
#define UI_CMD_CLEARLOGTEXT (WM_APP + 2)

#define UI_EXTMENU_ID			51001
#define UI_EXTMENU_SHOWLOG		51002
#define UI_EXTMENU_CLEARLOG		51003
#define UI_EXTMENU_SPACER		51004
#define UI_EXTMENU_DUMPNIRTTI	51005
#define UI_EXTMENU_DUMPRTTI		51006

HWND g_MainHwnd;
HWND g_ConsoleHwnd;
WNDPROC OldEditorUI_WndProc;
void ExportTest(FILE *File);

void EditorUI_Initialize()
{
	InitCommonControls();
	LoadLibraryA("MSFTEDIT.dll");

	if (!EditorUI_CreateLogWindow())
		MessageBoxA(nullptr, "Failed to create console log window", "Error", MB_ICONERROR);

	if (!EditorUI_CreateStdoutListener())
		MessageBoxA(nullptr, "Failed to create output listener for external processes", "Error", MB_ICONERROR);
}

bool EditorUI_CreateLogWindow()
{
	const uint32_t width = 1024;
	const uint32_t height = 480;
	HINSTANCE instance = (HINSTANCE)GetModuleHandle(nullptr);

	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hInstance = instance;
	wc.hIcon = LoadIcon(instance, IDI_APPLICATION);
	wc.hIconSm = wc.hIcon;
	wc.lpfnWndProc = EditorUI_LogWndProc;
	wc.lpszClassName = TEXT("RTEDITLOG");
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

	if (!RegisterClassEx(&wc))
		return false;

	g_ConsoleHwnd = CreateWindowExA(0, "RTEDITLOG", "Log", WS_OVERLAPPEDWINDOW, 64, 64, width, height, nullptr, nullptr, instance, nullptr);

	if (!g_ConsoleHwnd)
		return false;

	ShowWindow(g_ConsoleHwnd, SW_SHOW);
	return true;
}

bool EditorUI_CreateExtensionMenu(HWND MainWindow, HMENU MainMenu)
{
	// Create extended menu options
	HMENU subMenu = CreateMenu();

	BOOL result = TRUE;
	result = result && InsertMenu(subMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_SHOWLOG, "Show Log");
	result = result && InsertMenu(subMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_CLEARLOG, "Clear Log");
	result = result && InsertMenu(subMenu, -1, MF_BYPOSITION | MF_SEPARATOR, (UINT_PTR)UI_EXTMENU_SPACER, "");
	result = result && InsertMenu(subMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_DUMPNIRTTI, "Dump NiRTTI data");
	result = result && InsertMenu(subMenu, -1, MF_BYPOSITION | MF_STRING, (UINT_PTR)UI_EXTMENU_DUMPRTTI, "Dump RTTI data");

	MENUITEMINFO menuInfo;
	memset(&menuInfo, 0, sizeof(MENUITEMINFO));
	menuInfo.cbSize = sizeof(MENUITEMINFO);
	menuInfo.fMask = MIIM_SUBMENU | MIIM_ID | MIIM_STRING;
	menuInfo.hSubMenu = subMenu;
	menuInfo.wID = UI_EXTMENU_ID;
	menuInfo.dwTypeData = "Extensions";
	menuInfo.cch = (uint32_t)strlen(menuInfo.dwTypeData);
	result = result && InsertMenuItem(MainMenu, -1, TRUE, &menuInfo);

	AssertMsg(result, "Failed to create extension submenu");
	return result ? true : false;
}

bool EditorUI_CreateStdoutListener()
{
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = nullptr;

	HANDLE pipeRead;
	HANDLE pipeWrite;

	if (!CreatePipe(&pipeRead, &pipeWrite, &saAttr, 0))
		return false;

	// Ensure the read handle to the pipe for STDOUT is not inherited
	if (!SetHandleInformation(pipeRead, HANDLE_FLAG_INHERIT, 0))
		return false;

	std::thread pipeReader([pipeRead, pipeWrite]()
	{
		while (true)
		{
			char buffer[4096];
			DWORD bytesRead;
			bool succeeded = ReadFile(pipeRead, buffer, ARRAYSIZE(buffer), &bytesRead, nullptr) != 0;

			// Bail if there's nothing left or the process exited
			if (!succeeded || bytesRead <= 0)
				break;

			buffer[bytesRead - 1] = '\0';
			EditorUI_Log("%s\n", buffer);
		}

		CloseHandle(pipeRead);
		CloseHandle(pipeWrite);
	});

	pipeReader.detach();
	return true;
}

void EditorUI_LogVa(const char *Format, va_list Va)
{
	char buffer[2048];
	int len = _vsnprintf_s(buffer, _TRUNCATE, Format, Va);

	if (len <= 0)
		return;

	if (len >= 2 && buffer[len - 1] != '\n')
		strcat_s(buffer, "\n");

	// Buffer messages if the window hasn't been created yet
	static tbb::concurrent_vector<const char *> pendingMessages;

	if (g_ConsoleHwnd && pendingMessages.size() > 0)
	{
		for (size_t i = 0; i < pendingMessages.size(); i++)
			PostMessageA(g_ConsoleHwnd, UI_CMD_ADDLOGTEXT, 0, (LPARAM)pendingMessages[i]);

		pendingMessages.clear();
	}

	if (!g_ConsoleHwnd)
		pendingMessages.push_back(_strdup(buffer));
	else
		PostMessageA(g_ConsoleHwnd, UI_CMD_ADDLOGTEXT, 0, (LPARAM)_strdup(buffer));
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

void EditorUI_Assert(const char *File, int Line, const char *Message)
{
	EditorUI_Log("ASSERTION: %s (%s line %d)", Message, File, Line);
}

namespace ui::log
{
	// This is really a hack for the undefined symbol
	void Add(const char *Format, ...)
	{
		va_list va;

		va_start(va, Format);
		EditorUI_LogVa(Format, va);
		va_end(va);
	}
}

LRESULT CALLBACK EditorUI_WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_CREATE)
	{
		static bool editorUIInit = false;

		if (!editorUIInit)
		{
			const CREATESTRUCT *createInfo = (CREATESTRUCT *)lParam;

			if (_stricmp(createInfo->lpszName, "Creation Kit") != 0 || _stricmp(createInfo->lpszName, "Creation Kit") != 0)
			{
				AssertMsgVa(false, "Trying to initialize menus with an unknown window name: %s", createInfo->lpszName);
				ExitProcess(0);
			}

			editorUIInit = true;
			g_MainHwnd = Hwnd;
			EditorUI_CreateExtensionMenu(Hwnd, createInfo->hMenu);
		}
	}
	else if (Message == WM_COMMAND)
	{
		const uint32_t param = LOWORD(wParam);

		switch (param)
		{
		case UI_EXTMENU_SHOWLOG:
		{
			ShowWindow(g_ConsoleHwnd, SW_SHOW);
			SetForegroundWindow(g_ConsoleHwnd);
		}
		return 0;

		case UI_EXTMENU_CLEARLOG:
		{
			PostMessageA(g_ConsoleHwnd, UI_CMD_CLEARLOGTEXT, 0, 0);
		}
		return 0;

		case UI_EXTMENU_DUMPNIRTTI:
		case UI_EXTMENU_DUMPRTTI:
		{
			char filePath[MAX_PATH];
			memset(filePath, 0, sizeof(filePath));

			OPENFILENAME ofnData;
			memset(&ofnData, 0, sizeof(OPENFILENAME));
			ofnData.lStructSize = sizeof(OPENFILENAME);
			ofnData.lpstrFilter = "Text Files\0*.txt\0\0";
			ofnData.lpstrFile = filePath;
			ofnData.nMaxFile = ARRAYSIZE(filePath);
			ofnData.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
			ofnData.lpstrDefExt = "txt";

			if (GetSaveFileName(&ofnData))
			{
				if (FILE *f; fopen_s(&f, filePath, "w") == 0)
				{
					if (param == UI_EXTMENU_DUMPNIRTTI)
						ExportTest(f);
					else if (param == UI_EXTMENU_DUMPRTTI)
						MSRTTI::Dump(f);

					fclose(f);
				}
			}
		}
		return 0;
		}
	}
	else if (Message == WM_SETTEXT && Hwnd == g_MainHwnd)
	{
		// Continue normal execution but with a custom string
		char customTitle[1024];
		sprintf_s(customTitle, "%s (CK64 Fixes Rev. %s)", (const char *)lParam, g_GitVersion);

		return CallWindowProc(OldEditorUI_WndProc, Hwnd, Message, wParam, (LPARAM)customTitle);
	}

	return CallWindowProc(OldEditorUI_WndProc, Hwnd, Message, wParam, lParam);
}

LRESULT CALLBACK EditorUI_LogWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND richEditHwnd;

	switch (Message)
	{
	case WM_CREATE:
	{
		const CREATESTRUCT *info = (CREATESTRUCT *)lParam;

		// Create the rich edit control (https://docs.microsoft.com/en-us/windows/desktop/Controls/rich-edit-controls)
		uint32_t style = WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_LEFT | ES_NOHIDESEL | ES_AUTOVSCROLL | ES_READONLY;
		richEditHwnd = CreateWindowW(MSFTEDIT_CLASS, L"", style, 0, 0, info->cx, info->cy, Hwnd, nullptr, info->hInstance, nullptr);

		if (!richEditHwnd)
			return -1;

		// Subscribe to EN_MSGFILTER and EN_SELCHANGE
		SendMessageW(richEditHwnd, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_SELCHANGE);
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
						(*(void(__fastcall **)(__int64, HWND, __int64, __int64))(*(__int64 *)form + 720i64))(form, g_MainHwnd, 0, 1);
				}
			}
		}
	}
	break;

	case UI_CMD_ADDLOGTEXT:
	{
		void *textData = (void *)lParam;

		// Move caret to the end, then write
		CHARRANGE range;
		range.cpMin = LONG_MAX;
		range.cpMax = LONG_MAX;

		SendMessageA(richEditHwnd, EM_EXSETSEL, 0, (LPARAM)&range);
		SendMessageA(richEditHwnd, EM_REPLACESEL, FALSE, (LPARAM)textData);

		free(textData);
	}
	return 0;

	case UI_CMD_CLEARLOGTEXT:
	{
		char emptyString[1];
		emptyString[0] = '\0';

		SendMessageA(richEditHwnd, WM_SETTEXT, 0, (LPARAM)&emptyString);
	}
	return 0;
	}

	return DefWindowProc(Hwnd, Message, wParam, lParam);
}