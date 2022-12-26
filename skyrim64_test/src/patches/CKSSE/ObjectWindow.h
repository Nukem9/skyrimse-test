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

#pragma once

#include "../../common.h"

#include "UIMenus.h"
#include "UIBaseWindow.h"
#include "UICheckboxControl.h"

namespace ObjectWindow
{
	extern DLGPROC OldObjectWindowProc;

	namespace Classes = Core::Classes::UI;

	typedef struct tagOBJWND_CONTROLS
	{
		Classes::CUIBaseControl TreeList;
		Classes::CUIBaseControl ItemList;
		Classes::CUIBaseControl EditFilter;
		Classes::CUIBaseControl Spliter;
		Classes::CUICheckbox ActiveOnly;
	} OBJWND_CONTROLS, * POBJWND_CONTROLS, * LPOBJWND_CONTROLS;

	typedef struct tagOBJWND
	{
		BOOL StartResize;
		OBJWND_CONTROLS Controls;
		Classes::CUICustomWindow ObjectWindow;
	} OBJWND, * POBJWND, * LPOBJWND;

	extern DLGPROC OldDlgProc;
	typedef std::unordered_map<HWND, LPOBJWND> OBJWNDS;
	extern OBJWNDS ObjectWindows;

	OBJWNDS& FIXAPI GetAllWindowObj(VOID);

	INT_PTR CALLBACK ObjectWindowProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	BOOL WINAPI hk_MoveWindow(HWND hWindow, INT32 X, INT32 Y, INT32 nWidth, INT32 nHeight, BOOL bRepaint);
	void UpdateTreeView(void *Thisptr, HWND ControlHandle);
}