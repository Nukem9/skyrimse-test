#include "../stdafx.h"
#include <future>

#define WM_APP_THREAD_TASK		(WM_APP + 1)
#define WM_APP_UPDATE_CURSOR	(WM_APP + 2)

HWND g_SkyrimWindow;
WNDPROC g_OriginalWndProc;
DWORD MessageThreadId;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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

	// Hack/fix for mouse cursor not staying within fullscreen area
	if (uMsg == WM_APP_UPDATE_CURSOR)
	{
		RECT rcClip;
		GetWindowRect(hwnd, &rcClip);

		POINT p;
		GetCursorPos(&p);

		int midX = (rcClip.left + rcClip.right) / 2;
		int midY = (rcClip.top + rcClip.bottom) / 2;

		if (abs(p.x - midX) > 200 || abs(p.y - midY) > 200)
			SetCursorPos(midX, midY);

		return 0;
	}

	// Keyboard input
	if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR)
	{
		switch (wParam)
		{
		case VK_LWIN:		// Left windows key
		case VK_RWIN:		// Right windows key
		case VK_LSHIFT:		// Left shift
		case VK_RSHIFT:		// Right shift
		case VK_CAPITAL:	// Caps lock
		case VK_CONTROL:	// Control
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}

		ImGui_ImplDX11_WndProcHandler(hwnd, uMsg, wParam, lParam);
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
			if (g_SwapChain)
				g_SwapChain->SetFullscreenState(TRUE, nullptr);

			ShowWindow(hwnd, SW_RESTORE);
			while (ShowCursor(FALSE) >= 0) {}
			ProxyIDirectInputDevice8A::ToggleGlobalInput(true);
		}

		// Lost focus
		if ((uMsg == WM_ACTIVATEAPP && wParam == FALSE) ||
			(uMsg == WM_ACTIVATE && wParam == WA_INACTIVE))
		{
			if (g_SwapChain)
				g_SwapChain->SetFullscreenState(FALSE, nullptr);

			ShowWindow(hwnd, SW_MINIMIZE);
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
			(* (std::packaged_task<HWND()> *)msg.wParam)();
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_MOUSEMOVE && msg.hwnd == g_SkyrimWindow)
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

		if (wnd)
		{
			g_SkyrimWindow = wnd;

			// The original pointer must be saved BEFORE swapping it out
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

void PatchWindow()
{
	PatchMemory(g_ModuleBase + 0x5ADE40, (PBYTE)"\xE9\xD3\x00\x00\x00", 5);
	CreateThread(nullptr, 0, MessageThread, nullptr, 0, &MessageThreadId);

	PatchIAT(hk_CreateWindowExA, "user32.dll", "CreateWindowExA");
}
