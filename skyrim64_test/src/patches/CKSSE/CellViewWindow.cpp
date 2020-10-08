#include "../../common.h"
#include <CommCtrl.h>
#include "CellViewWindow.h"
#include "EditorUI.h"
#include "TESForm_CK.h"

namespace CellViewWindowo
{
	DLGPROC OldCellViewProc;

	INT_PTR CALLBACK CellViewProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (Message == WM_INITDIALOG)
		{
			// Eliminate the flicker when changing cells
			ListView_SetExtendedListViewStyleEx(GetDlgItem(DialogHwnd, 1155), LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
			ListView_SetExtendedListViewStyleEx(GetDlgItem(DialogHwnd, 1156), LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);

			ShowWindow(GetDlgItem(DialogHwnd, 1007), SW_HIDE);
		}
		else if (Message == WM_SIZE)
		{
			auto labelRect = (RECT *)OFFSET(0x3AFB570, 1530);

			// Fix the "World Space" label positioning on window resize
			RECT label;
			GetClientRect(GetDlgItem(DialogHwnd, 1164), &label);

			RECT rect;
			GetClientRect(GetDlgItem(DialogHwnd, 2083), &rect);

			int ddMid = rect.left + ((rect.right - rect.left) / 2);
			int labelMid = (label.right - label.left) / 2;

			SetWindowPos(GetDlgItem(DialogHwnd, 1164), nullptr, ddMid - (labelMid / 2), labelRect->top, 0, 0, SWP_NOSIZE);

			// Force the dropdown to extend the full length of the column
			labelRect->right = 0;
		}
		else if (Message == WM_COMMAND)
		{
			const uint32_t param = LOWORD(wParam);

			if (param == UI_CELL_VIEW_CHECKBOX)
			{
				bool enableFilter = SendMessage(reinterpret_cast<HWND>(lParam), BM_GETCHECK, 0, 0) == BST_CHECKED;
				SetPropA(DialogHwnd, "ActiveOnly", reinterpret_cast<HANDLE>(enableFilter));

				// Fake the dropdown list being activated
				SendMessageA(DialogHwnd, WM_COMMAND, MAKEWPARAM(2083, 1), 0);
				return 1;
			}
		}
		else if (Message == UI_CELL_VIEW_ADD_CELL_ITEM)
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

		return OldCellViewProc(DialogHwnd, Message, wParam, lParam);
	}

	void UpdateCellList(void *Thisptr, HWND ControlHandle, __int64 Unknown)
	{
		SendMessage(ControlHandle, WM_SETREDRAW, FALSE, 0);
		((void(__fastcall *)(void *, HWND, __int64))OFFSET(0x147FA70, 1530))(Thisptr, ControlHandle, Unknown);
		SendMessage(ControlHandle, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(ControlHandle, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
	}

	void UpdateObjectList(void *Thisptr, HWND *ControlHandle)
	{
		SendMessage(*ControlHandle, WM_SETREDRAW, FALSE, 0);
		((void(__fastcall *)(void *, HWND *))OFFSET(0x13E0CE0, 1530))(Thisptr, ControlHandle);
		SendMessage(*ControlHandle, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(*ControlHandle, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
	}
}