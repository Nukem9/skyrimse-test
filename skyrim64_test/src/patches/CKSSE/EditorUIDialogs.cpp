#include "../../common.h"
#include <CommCtrl.h>
#include "EditorUI.h"
#include "EditorUIDialogs.h"

namespace EditorUIDialogs
{
	LRESULT CALLBACK DialogTabProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (Message == WM_INITDIALOG)
		{
			// If it's the weapon sound dialog tab (id 3327), remap the "Unequip Sound" button (id 3682) to
			// a non-conflicting one (id 3683)
			char className[256];

			if (GetClassNameA(Hwnd, className, ARRAYSIZE(className)) > 0)
			{
				if (!strcmp(className, "WeaponClass"))
					SetWindowLongPtr(GetDlgItem(Hwnd, 3682), GWLP_ID, 3683);
			}

			ShowWindow(Hwnd, SW_HIDE);
			return 1;
		}

		return 0;
	}

	INT_PTR CALLBACK LipRecordDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		// Id's for "Recording..." dialog window
		switch (Message)
		{
		case WM_APP:
			// Don't actually kill the dialog, just hide it. It gets destroyed later when the parent window closes.
			SendMessageA(GetDlgItem(DialogHwnd, 31007), PBM_SETPOS, 0, 0);
			ShowWindow(DialogHwnd, SW_HIDE);
			PostQuitMessage(0);
			return TRUE;

		case WM_INITDIALOG:
			SendMessageA(GetDlgItem(DialogHwnd, 31007), PBM_SETRANGE, 0, 32768 * 1000);
			SendMessageA(GetDlgItem(DialogHwnd, 31007), PBM_SETSTEP, 1, 0);
			return TRUE;

		case WM_COMMAND:
			// Stop recording
			if (LOWORD(wParam) != UI_LIPRECORD_DIALOG_STOPRECORD)
				return FALSE;

			*(bool *)OFFSET(0x3AFAE28, 1530) = false;

			if (FAILED(((HRESULT(__fastcall *)(bool))OFFSET(0x13D5310, 1530))(false)))
				MessageBoxA(DialogHwnd, "Error with DirectSoundCapture buffer.", "DirectSound Error", MB_ICONERROR);

			return LipRecordDialogProc(DialogHwnd, WM_APP, 0, 0);

		case UI_LIPRECORD_DIALOG_STARTRECORD:
			// Start recording
			ShowWindow(DialogHwnd, SW_SHOW);
			*(bool *)OFFSET(0x3AFAE28, 1530) = true;

			if (FAILED(((HRESULT(__fastcall *)(bool))OFFSET(0x13D5310, 1530))(true)))
			{
				MessageBoxA(DialogHwnd, "Error with DirectSoundCapture buffer.", "DirectSound Error", MB_ICONERROR);
				return LipRecordDialogProc(DialogHwnd, WM_APP, 0, 0);
			}
			return TRUE;
		}

		return FALSE;
	}

	LRESULT CSScript_PickScriptsToCompileDlgProc(void *This, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		thread_local bool disableListViewUpdates;

		auto updateListViewItems = [This]
		{
			if (!disableListViewUpdates)
				((void(__fastcall *)(void *))OFFSET(0x20A9870, 1530))(This);
		};

		switch (Message)
		{
		case WM_SIZE:
			((void(__fastcall *)(void *))OFFSET(0x20A9CF0, 1530))(This);
			break;

		case WM_NOTIFY:
		{
			auto notification = reinterpret_cast<LPNMHDR>(lParam);

			// "SysListView32" control
			if (notification->idFrom == UI_COMIPLESCRIPT_DIALOG_LISTVIEW && notification->code == LVN_ITEMCHANGED)
			{
				updateListViewItems();
				return 1;
			}
		}
		break;

		case WM_INITDIALOG:
			disableListViewUpdates = true;
			((void(__fastcall *)(void *))OFFSET(0x20A99C0, 1530))(This);
			disableListViewUpdates = false;

			// Update it ONCE after everything is inserted
			updateListViewItems();
			break;

		case WM_COMMAND:
		{
			const uint32_t param = LOWORD(wParam);

			if (param == UI_COMIPLESCRIPT_DIALOG_CHECKALL || param == UI_COMIPLESCRIPT_DIALOG_UNCHECKALL || param == UI_COMIPLESCRIPT_DIALOG_CHECKALLCHECKEDOUT)
			{
				disableListViewUpdates = true;
				if (param == UI_COMIPLESCRIPT_DIALOG_CHECKALL)
					((void(__fastcall *)(void *))OFFSET(0x20AA080, 1530))(This);
				else if (param == UI_COMIPLESCRIPT_DIALOG_UNCHECKALL)
					((void(__fastcall *)(void *))OFFSET(0x20AA130, 1530))(This);
				else if (param == UI_COMIPLESCRIPT_DIALOG_CHECKALLCHECKEDOUT)
					((void(__fastcall *)(void *))OFFSET(0x20AA1E0, 1530))(This);
				disableListViewUpdates = false;

				updateListViewItems();
				return 1;
			}
			else if (param == UI_COMIPLESCRIPT_DIALOG_COMPILE)
			{
				// "Compile" button
				((void(__fastcall *)(void *))OFFSET(0x20A9F30, 1530))(This);
			}
		}
		break;
		}

		return ((LRESULT(__fastcall *)(void *, UINT, WPARAM, LPARAM))OFFSET(0x20ABD90, 1530))(This, Message, wParam, lParam);
	}
}