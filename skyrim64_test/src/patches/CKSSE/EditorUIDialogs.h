#pragma once

#include "../../common.h"

namespace EditorUIDialogs
{
	INT_PTR CALLBACK LipRecordDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT CSScript_PickScriptsToCompileDlgProc(void *This, UINT Message, WPARAM wParam, LPARAM lParam);
}