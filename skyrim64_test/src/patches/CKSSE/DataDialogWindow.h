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

#include "EditorUI.h"
#include "UIBaseWindow.h"

#define UI_LISTVIEW_PLUGINS					UI_DATA_DIALOG_PLUGINLISTVIEW
#define UI_EDIT_SEARCH_PLUGIN_BY_NAME		(UI_CUSTOM_MESSAGE + 4)	
#define UI_NEW_LISTVIEW_CONTROL_TO_RESULT	(UI_CUSTOM_MESSAGE + 5)	

namespace DataDialogWindow
{
	extern DLGPROC OldDataDialogProc;

	INT_PTR CALLBACK DataDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam);
}