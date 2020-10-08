#pragma once

#include "../../common.h"

namespace CellViewWindowo
{
	extern DLGPROC OldCellViewProc;

	INT_PTR CALLBACK CellViewProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void UpdateCellList(void *Thisptr, HWND ControlHandle, __int64 Unknown);
	void UpdateObjectList(void *Thisptr, HWND *ControlHandle);
}