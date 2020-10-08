#include "../../common.h"
#include <CommCtrl.h>
#include "ObjectWindow.h"
#include "EditorUI.h"
#include "TESForm_CK.h"

namespace ObjectWindow
{
	DLGPROC OldObjectWindowProc;

	INT_PTR CALLBACK ObjectWindowProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (Message == WM_INITDIALOG)
		{
			// Eliminate the flicker when changing categories
			ListView_SetExtendedListViewStyleEx(GetDlgItem(DialogHwnd, 1041), LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
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