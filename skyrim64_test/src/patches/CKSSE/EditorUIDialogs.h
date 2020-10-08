#pragma once

#include "../../common.h"

namespace EditorUIDialogs
{
	LRESULT CALLBACK DialogTabProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK LipRecordDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT CSScript_PickScriptsToCompileDlgProc(void *This, UINT Message, WPARAM wParam, LPARAM lParam);
}