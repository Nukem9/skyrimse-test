#include "../../common.h"
#include <CommCtrl.h>
#include "UITheme/TimeOfDay.h"

namespace PreferencesWindow
{
	namespace ToD = Core::UI::Theme::TimeOfDay;

	VOID FIXAPI hk_InitializeTimeOfDay(HWND hDlg, INT nIDDlgItem, FLOAT value, INT a4) {
		auto hwndTrackBar = GetDlgItem(hDlg, 0x3F6);
		auto hwndEdit = GetDlgItem(hDlg, 0x3E8);

		INT32 iPos = ToD::NewUITimeOfDayComponents.hWndTrackBar.Perform(TBM_GETPOS, 0, 0);

		SendMessage(hwndTrackBar, TBM_SETPOS, TRUE, (LPARAM)iPos);
		SetWindowText(hwndEdit, ToD::NewUITimeOfDayComponents.hWndEdit.Caption.c_str());
		EnableWindow(hwndEdit, FALSE);
	}

	VOID FIXAPI hk_SetNewValueTimeOfDay(HWND hDlg, INT nIDDlgItem, FLOAT value, INT a4) {
		auto hwndTrackBar = GetDlgItem(hDlg, 0x3F6);
		auto hwndEdit = GetDlgItem(hDlg, 0x3E8);

		INT32 iPos = SendMessage(hwndTrackBar, TBM_GETPOS, 0, 0);

		ToD::NewUITimeOfDayComponents.hWndTrackBar.Perform(TBM_SETPOS, TRUE, (LPARAM)iPos);

		CHAR szBuf[24];
		sprintf_s(szBuf, "%.2f", value);
		ToD::NewUITimeOfDayComponents.hWndEdit.Caption = szBuf;

		((VOID(__fastcall*)(HWND, INT, FLOAT, INT))OFFSET(0x1318600, 16438))(hDlg, nIDDlgItem, value, a4);
	}
}