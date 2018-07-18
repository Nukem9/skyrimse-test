#include <future>
#include <set>
#include "../common.h"
#include "dinput8.h"

#define WM_APP_THREAD_TASK		(WM_APP + 1)
#define WM_APP_UPDATE_CURSOR	(WM_APP + 2)

HWND g_SkyrimWindow;
WNDPROC g_OriginalWndProc;
DWORD MessageThreadId;

std::recursive_mutex dialogMutex;
std::set<HWND> g_ParentCreateDialogHwnds;
std::set<HWND> g_ParentDialogHwnds;
std::atomic<uint64_t> g_OpenDialogCount;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ui::HandleInput(hwnd, uMsg, wParam, lParam);

	// Always-forwarded game wndproc commands
	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGED:
	case WM_NCACTIVATE:
	case WM_DESTROY:
	case WM_SIZE:
	case WM_SYSCOMMAND:
	case WM_MOUSEMOVE:
	case WM_IME_SETCONTEXT:
		return CallWindowProc(g_OriginalWndProc, hwnd, uMsg, wParam, lParam);
	}

	// Fix for mouse cursor not staying within fullscreen area
	if (uMsg == WM_APP_UPDATE_CURSOR)
	{
		if (ui::IsMouseDragging())
		{
			// Free roam
			ClipCursor(nullptr);
		}
		else
		{
			RECT rcClip;
			GetWindowRect(hwnd, &rcClip);

			// 1 pixel of padding
			rcClip.left += 1;
			rcClip.top += 1;

			rcClip.right -= 1;
			rcClip.bottom -= 1;

			ClipCursor(&rcClip);
		}

		return 0;
	}

	// Handle window getting/losing focus (hide/show cursor)
	if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE || uMsg == WM_SETFOCUS)
	{
		// Gained focus
		if ((uMsg == WM_ACTIVATEAPP && wParam == TRUE) ||
			(uMsg == WM_ACTIVATE && wParam != WA_INACTIVE) ||
			(uMsg == WM_SETFOCUS))
		{
			while (ShowCursor(FALSE) >= 0) {}

			if (GetForegroundWindow() == g_SkyrimWindow)
				ProxyIDirectInputDevice8A::ToggleGlobalInput(true);
		}

		// Lost focus
		if ((uMsg == WM_ACTIVATEAPP && wParam == FALSE) ||
			(uMsg == WM_ACTIVATE && wParam == WA_INACTIVE))
		{
			while (ShowCursor(TRUE) < 0) {}

			ProxyIDirectInputDevice8A::ToggleGlobalInput(false);
		}

		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI MessageThread(LPVOID)
{
	SetThreadName(GetCurrentThreadId(), "Game Message Loop");

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		if (msg.message == WM_APP_THREAD_TASK)
		{
			// Check for hk_CreateWindowExA wanting to execute here
			(* reinterpret_cast<std::packaged_task<HWND()> *>(msg.wParam))();
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// GetForegroundWindow hack since alt+tab or windows key don't always play nice
		if (msg.message == WM_MOUSEMOVE && msg.hwnd == g_SkyrimWindow && msg.hwnd == GetForegroundWindow())
			WindowProc(msg.hwnd, WM_APP_UPDATE_CURSOR, 0, 0);
	}

	// Message loop exited (WM_QUIT) or there was an error
	return 0;
}

HWND WINAPI hk_CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	// Create this window on a separate thread
	auto threadTask = std::packaged_task<HWND()>([&]()
	{
		HWND wnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

		if (wnd && !g_SkyrimWindow && !strcmp(lpClassName, "Skyrim Special Edition"))
		{
			// The original pointer must be saved BEFORE swapping it out
			g_SkyrimWindow = wnd;
			g_OriginalWndProc = (WNDPROC)GetWindowLongPtr(wnd, GWLP_WNDPROC);

			SetWindowLongPtr(wnd, GWLP_WNDPROC, (LONG_PTR)&WindowProc);
		}

		return wnd;
	});

	// Wait for completion...
	auto taskVar = threadTask.get_future();
	PostThreadMessage(MessageThreadId, WM_APP_THREAD_TASK, (WPARAM)&threadTask, 0);

	return taskVar.get();
}

HWND WINAPI hk_CreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	// NOTE: This is NOT an actual dialog. It uses CreateWindowExA internally.
	dialogMutex.lock();
	g_ParentCreateDialogHwnds.insert(hWndParent);
	dialogMutex.unlock();

	return CreateDialogParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR WINAPI hk_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	dialogMutex.lock();
	Assert(IsWindow(hWndParent));
	Assert(g_ParentDialogHwnds.count(hWndParent) <= 0);

	g_ParentDialogHwnds.insert(hWndParent);
	dialogMutex.unlock();

	g_OpenDialogCount++;
	return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

BOOL WINAPI hk_EndDialog(HWND hDlg, INT_PTR nResult)
{
	Assert(hDlg);
	HWND parent = GetParent(hDlg);

	dialogMutex.lock();
	{
		if (g_OpenDialogCount <= 0)
		{
			// Fix for the CK calling EndDialog on a CreateDialogParamA window
			if (g_ParentCreateDialogHwnds.count(parent) > 0)
			{
				g_ParentCreateDialogHwnds.erase(parent);
				DestroyWindow(hDlg);
			}

			dialogMutex.unlock();
			return FALSE;
		}

		AssertMsg(g_ParentDialogHwnds.count(parent) > 0, "Still calling EndDialog on a non-dialog window?");
		g_ParentDialogHwnds.erase(parent);
	}
	dialogMutex.unlock();

	g_OpenDialogCount--;
	return EndDialog(hDlg, nResult);
}

LRESULT WINAPI hk_SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (hWnd && Msg == WM_DESTROY)
	{
		dialogMutex.lock();
		bool found = g_ParentDialogHwnds.count(GetParent(hWnd)) > 0;
		dialogMutex.unlock();

		if (found)
		{
			// This is a dialog, we can't call DestroyWindow on it
			return 0;
		}

		DestroyWindow(hWnd);
		return 0;
	}

	return SendMessageA(hWnd, Msg, wParam, lParam);
}

void PatchWindow()
{
	if (g_IsCreationKit)
	{
		PatchIAT(hk_CreateDialogParamA, "USER32.DLL", "CreateDialogParamA");
		PatchIAT(hk_DialogBoxParamA, "USER32.DLL", "DialogBoxParamA");
		PatchIAT(hk_EndDialog, "USER32.DLL", "EndDialog");
		PatchIAT(hk_SendMessageA, "USER32.DLL", "SendMessageA");
	}
	else
	{
		PatchMemory(g_ModuleBase + 0x5AF310, (PBYTE)"\xE9\xD3\x00\x00\x00", 5);
		CreateThread(nullptr, 0, MessageThread, nullptr, 0, &MessageThreadId);

		PatchIAT(hk_CreateWindowExA, "USER32.DLL", "CreateWindowExA");
	}
}
