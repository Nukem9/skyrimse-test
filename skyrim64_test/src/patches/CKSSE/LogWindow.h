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

	void LogVa(const char *Format, va_list Va);
	void Log(const char *Format, ...);
	void LogWarning(int Type, const char *Format, ...);
	void LogWarningUnknown1(const char *Format, ...);
	void LogWarningUnknown2(__int64 Unused, const char *Format, ...);
	void LogAssert(const char *File, int Line, const char *Message, ...);
}