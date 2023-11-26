#pragma once

#include "../../common.h"

namespace LogWindow
{
	HWND GetWindow();
	HANDLE GetStdoutListenerPipe();

	bool Initialize();
	bool CreateStdoutListener();
	void LoadWarningBlacklist();

	LRESULT CALLBACK WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void Clear();
	void BringToFront();
	void EnableAutoscroll(bool Enable);
	void Log(const char *Format, ...);
	void LogVa(const char *Format, va_list Va);
	void LogWarning(int Type, const char *Format, ...);
	void LogWarningUnknown1(const char *Format, ...);
	void LogWarningUnknown2(__int64 Unused, const char *Format, ...);
	void LogAssert(const char *File, int Line, const char *Message, ...);
}

#define _MESSAGE(x)				LogWindow::Log(x)
#define _MESSAGE_FMT(x, ...)	LogWindow::Log(x, ##__VA_ARGS__)