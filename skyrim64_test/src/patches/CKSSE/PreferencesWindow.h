#pragma once

#include "UIBaseWindow.h"

namespace PreferencesWindow
{
	VOID FIXAPI hk_InitializeTimeOfDay(HWND hDlg, INT nIDDlgItem, FLOAT value, INT a4);
	VOID FIXAPI hk_SetNewValueTimeOfDay(HWND hDlg, INT nIDDlgItem, FLOAT value, INT a4);
}