//////////////////////////////////////////
/*
* Copyright (c) 2020 Nukem9 <email:Nukem@outlook.com>
* Copyright (c) 2022 Perchik71 <email:perchik71@outlook.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this
* software and associated documentation files (the "Software"), to deal in the Software
* without restriction, including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or
* substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
* PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
* FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
* OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
//////////////////////////////////////////

#include "../../common.h"
#include <CommCtrl.h>
#include "ObjectWindow.h"
#include "EditorUI.h"
#include "TESForm_CK.h"
#include "MainWindow.h"

#include "UIThemeMode.h"
#include "EditorUIDarkMode.h"

#define UI_CMD_CHANGE_SPLITTER_OBJECTWINDOW		(UI_CUSTOM_MESSAGE + 4)

namespace ObjectWindow
{
	OBJWNDS ObjectWindows;
	DLGPROC OldObjectWindowProc;

	OBJWNDS& FIXAPI GetAllWindowObj(VOID)
	{
		return ObjectWindows;
	}

	VOID FIXAPI ResizeObjectWndChildControls(LPOBJWND lpObjWnd)
	{
		// The perfectionist in me is dying....

		lpObjWnd->Controls.TreeList.LockUpdate();
		lpObjWnd->Controls.ItemList.LockUpdate();
		lpObjWnd->Controls.EditFilter.LockUpdate();

		auto WndRect = lpObjWnd->ObjectWindow.ClientRect();

		LONG w_tree = lpObjWnd->Controls.TreeList.Width;
		lpObjWnd->Controls.ActiveOnly.Width = w_tree;

		LONG w_left = w_tree - lpObjWnd->Controls.EditFilter.Left;
		lpObjWnd->Controls.EditFilter.Width = w_left;

		auto TopT = lpObjWnd->Controls.TreeList.Top;

		lpObjWnd->Controls.ItemList.Left = w_tree + 5;
		lpObjWnd->Controls.ItemList.Width = WndRect.Width - (w_tree + 5);
		lpObjWnd->Controls.ItemList.Height = WndRect.Height;
		lpObjWnd->Controls.TreeList.Height = WndRect.Height - TopT;
		lpObjWnd->Controls.Spliter.Height = WndRect.Height - TopT;

		// fix bad pic
		auto handle = lpObjWnd->ObjectWindow.Handle;
		RECT r = { 0, 0, lpObjWnd->Controls.ItemList.Left, TopT };
		InvalidateRect(handle, &r, TRUE);
		UpdateWindow(handle);

		lpObjWnd->Controls.EditFilter.UnlockUpdate();
		lpObjWnd->Controls.ItemList.UnlockUpdate();
		lpObjWnd->Controls.TreeList.UnlockUpdate();
		lpObjWnd->Controls.EditFilter.Repaint();
	}

	VOID FIXAPI SplitterResizeObjectWndChildControls(LPOBJWND lpObjWnd)
	{
		lpObjWnd->Controls.TreeList.LockUpdate();
		lpObjWnd->Controls.ItemList.LockUpdate();
		lpObjWnd->Controls.EditFilter.LockUpdate();

		auto WndRect = lpObjWnd->ObjectWindow.ClientRect();

		LONG w_tree = lpObjWnd->Controls.TreeList.Width;
		lpObjWnd->Controls.ActiveOnly.Width = w_tree;

		LONG w_left = w_tree - lpObjWnd->Controls.EditFilter.Left;
		lpObjWnd->Controls.EditFilter.Width = w_left;

		auto TopT = lpObjWnd->Controls.TreeList.Top;

		lpObjWnd->Controls.ItemList.Left = w_tree + 5;
		lpObjWnd->Controls.ItemList.Width = WndRect.Width - (w_tree + 5);

		// fix bad pic
		auto handle = lpObjWnd->ObjectWindow.Handle; 
		RECT r = { 0, 0, lpObjWnd->Controls.ItemList.Left, TopT };
		InvalidateRect(handle, &r, TRUE);
		UpdateWindow(handle);

		lpObjWnd->Controls.EditFilter.UnlockUpdate();
		lpObjWnd->Controls.ItemList.UnlockUpdate();
		lpObjWnd->Controls.TreeList.UnlockUpdate();

		lpObjWnd->Controls.EditFilter.Repaint();
	}

	BOOL WINAPI hk_MoveWindow(HWND hWindow, INT32 X, INT32 Y, INT32 nWidth, INT32 nHeight, BOOL bRepaint)
	{
		BOOL bResult = MoveWindow(hWindow, X, Y, nWidth, nHeight, bRepaint);

		if (auto iterator = ObjectWindows.find(GetParent(hWindow)); iterator != ObjectWindows.end())
		{
			LPOBJWND lpObjWnd = (*iterator).second;
			if (lpObjWnd) lpObjWnd->ObjectWindow.Perform(WM_COMMAND, UI_CMD_CHANGE_SPLITTER_OBJECTWINDOW, 0);
		}

		return bResult;
	}

	INT_PTR CALLBACK ObjectWindowProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (Message == WM_INITDIALOG)
		{
			LPOBJWND lpObjWnd = new OBJWND;

			lpObjWnd->ObjectWindow = DialogHwnd;

			lpObjWnd->Controls.TreeList = lpObjWnd->ObjectWindow.GetControl(2093);
			lpObjWnd->Controls.ItemList = lpObjWnd->ObjectWindow.GetControl(1041);
			lpObjWnd->Controls.EditFilter = lpObjWnd->ObjectWindow.GetControl(2581);
			lpObjWnd->Controls.Spliter = lpObjWnd->ObjectWindow.GetControl(2157);
			lpObjWnd->Controls.ActiveOnly.CreateWnd(lpObjWnd->ObjectWindow, lpObjWnd->ObjectWindow.GetControl(UI_OBJECT_WINDOW_CHECKBOX), UI_OBJECT_WINDOW_CHECKBOX);

			// Eliminate the flicker when changing categories
			ListView_SetExtendedListViewStyleEx(lpObjWnd->Controls.ItemList.Handle, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
			// Eliminate the flicker when changing size trees
			SendMessage(lpObjWnd->Controls.TreeList.Handle, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
			// 
			ListView_SetExtendedListViewStyleEx(lpObjWnd->Controls.ItemList.Handle, LVS_EX_INFOTIP, LVS_EX_INFOTIP);

			// Erase Icon and SysMenu
			if (!ObjectWindows.size())
				lpObjWnd->ObjectWindow.Style = WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME;
			else
				lpObjWnd->ObjectWindow.Style = WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_SYSMENU;

			ObjectWindows.emplace(DialogHwnd, lpObjWnd);
		}
		// Don't let us reduce the window too much
		else if (Message == WM_GETMINMAXINFO)
		{
			LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
			lpMMI->ptMinTrackSize.x = 350;
			lpMMI->ptMinTrackSize.y = 200;

			return S_OK;
		}
		else if (Message == WM_ERASEBKGND) {
			if (auto iterator = ObjectWindows.find(DialogHwnd);  iterator != ObjectWindows.end())
			{
				LPOBJWND lpObjWnd = (*iterator).second;
				if (lpObjWnd) {
					HDC dc = (HDC)wParam;
					auto Rect = lpObjWnd->Controls.ItemList.BoundsRect;
					ExcludeClipRect(dc, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom);
					Rect = lpObjWnd->Controls.TreeList.BoundsRect;
					ExcludeClipRect(dc, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom);
					Rect = lpObjWnd->Controls.ActiveOnly.BoundsRect;
					ExcludeClipRect(dc, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom);
					Rect = lpObjWnd->Controls.EditFilter.BoundsRect;
					ExcludeClipRect(dc, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom);

					HBRUSH brush;

					if (UITheme::IsEnabledMode())
						brush = UITheme::Comctl32GetSysColorBrush(COLOR_BTNFACE);
					else if (EditorUIDarkMode::IsInitialize())
						brush = EditorUIDarkMode::Comctl32GetSysColorBrush(COLOR_BTNFACE);
					else
						brush = GetSysColorBrush(COLOR_BTNFACE);

					RECT rc;
					GetClipBox(dc, &rc);
					FillRect(dc, &rc, brush);

					return 1;
				}
			}
		}
		else if (Message == WM_SIZE)
		{
			if (auto iterator = ObjectWindows.find(DialogHwnd);  iterator != ObjectWindows.end())
			{
				LPOBJWND lpObjWnd = (*iterator).second;
				if (lpObjWnd) {
					ResizeObjectWndChildControls(lpObjWnd);
					return 0;
				}
			}
		}
		else if (Message == WM_COMMAND)
		{
			const uint32_t param = LOWORD(wParam);

			if (param == UI_OBJECT_WINDOW_CHECKBOX)
			{
				bool enableFilter = SendMessage(reinterpret_cast<HWND>(lParam), BM_GETCHECK, 0, 0) == BST_CHECKED;
				SetPropA(DialogHwnd, "ActiveOnly", reinterpret_cast<HANDLE>(enableFilter));

				// Force the list items to update as if it was by timer
				SendMessageA(DialogHwnd, WM_TIMER, 0x4D, 0);
				return 1;
			}
			else if (param == UI_CMD_CHANGE_SPLITTER_OBJECTWINDOW) {
				if (auto iterator = ObjectWindows.find(DialogHwnd); iterator != ObjectWindows.end())
				{
					LPOBJWND lpObjWnd = (*iterator).second;
					if (lpObjWnd) SplitterResizeObjectWndChildControls(lpObjWnd);
				}
				return S_OK;
			}
		}
		else if (Message == UI_OBJECT_WINDOW_ADD_ITEM)
		{
			auto form = reinterpret_cast<const TESForm_CK *>(wParam);
			auto allowInsert = reinterpret_cast<bool *>(lParam);

			*allowInsert = true;

			// Skip the entry if "Show only active forms" is checked
			if (static_cast<bool>(GetPropA(DialogHwnd, "ActiveOnly")))
			{
				if (form && !form->GetActive())
					*allowInsert = false;
			}

			return 1;
		}
		else if (Message == WM_SHOWWINDOW)
		{
			if (auto iterator = ObjectWindows.find(DialogHwnd); iterator != ObjectWindows.end())
			{
				LPOBJWND lpObjWnd = (*iterator).second;
				if (lpObjWnd) ResizeObjectWndChildControls(lpObjWnd);
			}
		}
		else if (Message == WM_DESTROY)
		{
			LPOBJWND lpObjWnd = ObjectWindows.at(DialogHwnd);
			if (lpObjWnd)
			{
				ObjectWindows.erase(DialogHwnd);

				delete lpObjWnd;
				lpObjWnd = NULL;
			}
		}

		return OldObjectWindowProc(DialogHwnd, Message, wParam, lParam);
	}

	void UpdateTreeView(void *Thisptr, HWND ControlHandle)
	{
		SendMessage(ControlHandle, WM_SETREDRAW, FALSE, 0);
		((void(__fastcall *)(void *, HWND))OFFSET(0x12D8710, 1530))(Thisptr, ControlHandle);
		SendMessage(ControlHandle, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(ControlHandle, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
	}
}