#include "../../common.h"
#include <tbb/concurrent_vector.h>
#include <Richedit.h>
#include "EditorUI.h"
#include "EditorUIDarkMode.h"
#include "MainWindow.h"
#include "LogWindow.h"

#define UI_LOG_CMD_ADDTEXT		(WM_APP + 1)
#define UI_LOG_CMD_CLEARTEXT	(WM_APP + 2)
#define UI_LOG_CMD_AUTOSCROLL	(WM_APP + 3)

namespace LogWindow
{
	HWND LogWindowHandle;
	HANDLE ExternalPipeReaderHandle;
	HANDLE ExternalPipeWriterHandle;
	FILE *OutputFileHandle;

	tbb::concurrent_vector<const char *> PendingMessages;
	std::unordered_set<uint64_t> MessageBlacklist;

	HWND GetWindow()
	{
		return LogWindowHandle;
	}

	HANDLE GetStdoutListenerPipe()
	{
		return ExternalPipeWriterHandle;
	}

	bool Initialize()
	{
		if (!LoadLibraryA("MSFTEDIT.dll"))
			return false;

		// Build warning blacklist stored in the ini file
		LoadWarningBlacklist();

		// File output
		const std::string logPath = g_INI.Get("CreationKit_Log", "OutputFile", "none");

		if (logPath != "none")
		{
			if (fopen_s(&OutputFileHandle, logPath.c_str(), "w") != 0)
				OutputFileHandle = nullptr;

			AssertMsgVa(OutputFileHandle, "Unable to open the log file '%s' for writing. To disable, set the 'OutputFile' INI option to 'none'.", logPath.c_str());
		}

		std::thread asyncLogThread([]()
		{
			EditorUIDarkMode::InitializeThread();

			// Output window
			auto instance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));

			WNDCLASSEX wc
			{
				.cbSize = sizeof(WNDCLASSEX),
				.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
				.lpfnWndProc = WndProc,
				.hInstance = instance,
				.hIcon = LoadIcon(instance, MAKEINTRESOURCE(0x13E)),
				.hCursor = LoadCursor(nullptr, IDC_ARROW),
				.hbrBackground = static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)),
				.lpszClassName = TEXT("RTEDITLOG"),
				.hIconSm = wc.hIcon,
			};

			if (!RegisterClassEx(&wc))
				return false;

			LogWindowHandle = CreateWindowEx(0, TEXT("RTEDITLOG"), TEXT("Log"), WS_OVERLAPPEDWINDOW, 64, 64, 1024, 480, nullptr, nullptr, instance, nullptr);

			if (!LogWindowHandle)
				return false;

			// Poll every 100ms for new lines
			SetTimer(LogWindowHandle, UI_LOG_CMD_ADDTEXT, 100, nullptr);
			ShowWindow(LogWindowHandle, SW_SHOW);
			UpdateWindow(LogWindowHandle);

			MSG msg;
			while (GetMessage(&msg, nullptr, 0, 0) > 0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			return true;
		});

		asyncLogThread.detach();
		return true;
	}

	bool CreateStdoutListener()
	{
		SECURITY_ATTRIBUTES saAttr
		{
			.nLength = sizeof(SECURITY_ATTRIBUTES),
			.lpSecurityDescriptor = nullptr,
			.bInheritHandle = TRUE,
		};

		if (!CreatePipe(&ExternalPipeReaderHandle, &ExternalPipeWriterHandle, &saAttr, 0))
			return false;

		// Ensure the read handle to the pipe for STDOUT is not inherited
		if (!SetHandleInformation(ExternalPipeReaderHandle, HANDLE_FLAG_INHERIT, 0))
			return false;

		std::thread pipeReader([]()
		{
			char logBuffer[16384] = {};

			while (true)
			{
				char buffer[4096] = {};
				DWORD bytesRead;

				bool succeeded = ReadFile(ExternalPipeReaderHandle, buffer, ARRAYSIZE(buffer) - 1, &bytesRead, nullptr) != 0;

				// Bail if there's nothing left or the process exited
				if (!succeeded || bytesRead <= 0)
					break;

				strcat_s(logBuffer, buffer);

				// Flush on every newline and skip empty/whitespace strings
				while (char *end = strchr(logBuffer, '\n'))
				{
					*end = '\0';
					auto len = static_cast<size_t>(end - logBuffer);

					while (strchr(logBuffer, '\r'))
						*strchr(logBuffer, '\r') = ' ';

					if (len > 0 && (len > 1 || logBuffer[0] != ' '))
						LogWarning(6, "%s", logBuffer);

					strcpy_s(logBuffer, end + 1);
				}
			}

			CloseHandle(ExternalPipeReaderHandle);
			CloseHandle(ExternalPipeWriterHandle);
		});

		pipeReader.detach();
		return true;
	}

	void LoadWarningBlacklist()
	{
		if (!g_INI.GetBoolean("CreationKit", "WarningBlacklist", false))
			return;

		// Keep reading entries until an empty one is hit
		for (uint32_t i = 0;; i++)
		{
			std::string message = g_INI.Get("CreationKit_Warnings", "W" + std::to_string(i), "");

			if (message.empty())
				break;

			// Un-escape newline and carriage return characters
			for (size_t i; (i = message.find("\\n")) != std::string::npos;)
				message.replace(i, 2, "\n");

			for (size_t i; (i = message.find("\\r")) != std::string::npos;)
				message.replace(i, 2, "\r");

			MessageBlacklist.emplace(XUtil::MurmurHash64A(message.c_str(), message.length()));
		}
	}

	LRESULT CALLBACK WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		static HWND richEditHwnd;
		static bool autoScroll;

		switch (Message)
		{
		case WM_CREATE:
		{
			auto info = reinterpret_cast<const CREATESTRUCT *>(lParam);

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

			// Set a better font & convert points to Twips (1 point = 20 Twips)
			CHARFORMAT2A format = {};
			format.cbSize = sizeof(format);
			format.dwMask = CFM_FACE | CFM_SIZE | CFM_WEIGHT;
			format.yHeight = g_INI.GetInteger("CreationKit_Log", "FontSize", 10) * 20;
			format.wWeight = static_cast<WORD>(g_INI.GetInteger("CreationKit_Log", "FontWeight", FW_NORMAL));
			strncpy_s(format.szFaceName, g_INI.Get("CreationKit_Log", "Font", "Consolas").c_str(), _TRUNCATE);

			SendMessageA(richEditHwnd, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));

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
			static uint64_t lastClickTime;
			auto notification = reinterpret_cast<const LPNMHDR>(lParam);

			if (notification->code == EN_MSGFILTER)
			{
				auto msgFilter = reinterpret_cast<const MSGFILTER *>(notification);

				if (msgFilter->msg == WM_LBUTTONDBLCLK)
					lastClickTime = GetTickCount64();
			}
			else if (notification->code == EN_SELCHANGE)
			{
				auto selChange = reinterpret_cast<const SELCHANGE *>(notification);

				// Mouse double click with a valid selection -> try to parse form id
				if ((GetTickCount64() - lastClickTime > 1000) || selChange->seltyp == SEL_EMPTY)
					break;

				char lineData[4096];
				*reinterpret_cast<uint16_t *>(&lineData[0]) = ARRAYSIZE(lineData);

				// Get the line number & text from the selected range
				LRESULT lineIndex = SendMessageA(richEditHwnd, EM_LINEFROMCHAR, selChange->chrg.cpMin, 0);
				LRESULT charCount = SendMessageA(richEditHwnd, EM_GETLINE, lineIndex, reinterpret_cast<LPARAM>(&lineData));

				if (charCount > 0)
				{
					lineData[charCount - 1] = '\0';

					// Capture each hexadecimal form id in the format of "(XXXXXXXX)"
					for (char *p = lineData; p[0] != '\0'; p++)
					{
						if (p[0] == '(' && strlen(p) >= 10 && p[9] == ')')
						{
							uint32_t id = strtoul(&p[1], nullptr, 16);
							PostMessageA(MainWindow::GetWindow(), WM_COMMAND, UI_EDITOR_OPENFORMBYID, id);
						}
					}
				}

				lastClickTime = GetTickCount64() + 1000;
			}
		}
		break;

		case WM_TIMER:
		{
			if (wParam != UI_LOG_CMD_ADDTEXT)
				break;

			if (PendingMessages.size() <= 0)
				break;

			return WndProc(Hwnd, UI_LOG_CMD_ADDTEXT, 0, 0);
		}
		return 0;

		case UI_LOG_CMD_ADDTEXT:
		{
			SendMessageA(richEditHwnd, WM_SETREDRAW, FALSE, 0);

			// Save old position if not scrolling
			POINT scrollRange;

			if (!autoScroll)
				SendMessageA(richEditHwnd, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scrollRange));

			// Get a copy of all elements and clear the global list
			auto messages(std::move(PendingMessages));

			for (const char *message : messages)
			{
				// Move caret to the end, then write
				CHARRANGE range
				{
					.cpMin = LONG_MAX,
					.cpMax = LONG_MAX,
				};

				SendMessageA(richEditHwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
				SendMessageA(richEditHwnd, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(message));

				free(const_cast<char *>(message));
			}

			if (!autoScroll)
				SendMessageA(richEditHwnd, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scrollRange));

			SendMessageA(richEditHwnd, WM_SETREDRAW, TRUE, 0);
			RedrawWindow(richEditHwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
		}
		return 0;

		case UI_LOG_CMD_CLEARTEXT:
		{
			// Set to an empty string
			SendMessageA(richEditHwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(""));
		}
		return 0;

		case UI_LOG_CMD_AUTOSCROLL:
			autoScroll = static_cast<bool>(wParam);
			return 0;
		}

		return DefWindowProc(Hwnd, Message, wParam, lParam);
	}

	void Clear()
	{
		PostMessageA(LogWindowHandle, UI_LOG_CMD_CLEARTEXT, 0, 0);
	}

	void BringToFront()
	{
		ShowWindow(LogWindowHandle, SW_SHOW);
		SetForegroundWindow(LogWindowHandle);
	}

	void EnableAutoscroll(bool Enable)
	{
		PostMessageA(LogWindowHandle, UI_LOG_CMD_AUTOSCROLL, static_cast<WPARAM>(Enable), 0);
	}

	void Log(const char *Format, ...)
	{
		va_list va;

		va_start(va, Format);
		LogVa(Format, va);
		va_end(va);
	}

	void LogVa(const char *Format, va_list Va)
	{
		char buffer[2048];
		int len = _vsnprintf_s(buffer, _TRUNCATE, Format, Va);

		if (len <= 0)
			return;

		if (MessageBlacklist.count(XUtil::MurmurHash64A(buffer, len)))
			return;

		if (len >= 2 && buffer[len - 1] != '\n')
			strcat_s(buffer, "\n");

		if (OutputFileHandle)
		{
			fputs(buffer, OutputFileHandle);
			fflush(OutputFileHandle);
		}

		if (PendingMessages.size() < 50000)
			PendingMessages.emplace_back(_strdup(buffer));
	}

	void LogWarning(int Type, const char *Format, ...)
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

		Log("%s: %s", typeList[Type], buffer);
	}

	void LogWarningUnknown1(const char *Format, ...)
	{
		va_list va;

		va_start(va, Format);
		LogVa(Format, va);
		va_end(va);
	}

	void LogWarningUnknown2(__int64 Unused, const char *Format, ...)
	{
		va_list va;

		va_start(va, Format);
		LogVa(Format, va);
		va_end(va);
	}

	void LogAssert(const char *File, int Line, const char *Message, ...)
	{
		if (!Message)
			Message = "<No message>";

		char buffer[2048];
		va_list va;

		va_start(va, Message);
		_vsnprintf_s(buffer, _TRUNCATE, Message, va);
		va_end(va);

		Log("ASSERTION: %s (%s line %d)", buffer, File, Line);
	}
}