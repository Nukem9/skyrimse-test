#pragma once

#include "../../common.h"

namespace DataDialogWindow
{
	extern DLGPROC OldDataDialogProc;

	INT_PTR CALLBACK DataDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
}