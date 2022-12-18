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

#include "../../common.h"
#include <CommCtrl.h>
#include <Richedit.h>
#include "ObjectWindow.h"
#include "DataDialogWindow.h"
#include "UIThemeMode.h"
#include "UIImageList.h"

#include "../../../resource.h"

namespace DataDialogWindow
{
	constexpr int VisibleGroupId = 0;
	constexpr int FilteredGroupId = 1;

	DLGPROC OldDataDialogProc;
	Core::Classes::UI::CUIBaseWindow wnd;
	Core::Classes::UI::CUIBaseControl pluginList;
	Core::Classes::UI::CUIImageList ImageList;

	INT_PTR CALLBACK DataDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (Message == WM_INITDIALOG)
		{
			INT_PTR result = OldDataDialogProc(DialogHwnd, Message, wParam, lParam);
			HWND pluginListHandle = GetDlgItem(DialogHwnd, UI_LISTVIEW_PLUGINS);
			
			wnd = DialogHwnd;
			pluginList = pluginListHandle;

			wnd.Style = WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_SYSMENU;

			// Subscribe to notifications when the user types in the filter text box
			SendMessageA(GetDlgItem(DialogHwnd, UI_DATA_DIALOG_FILTERBOX), EM_SETEVENTMASK, 0, ENM_CHANGE);

			// Prevent flickering & adjust width to fit file names
			ListView_SetExtendedListViewStyleEx(pluginListHandle, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
			ListView_SetColumnWidth(pluginListHandle, 0, 250);

			// Create two separate list view groups: one for default items and one for hidden (filtered) items. This has to be run
			// after WM_INITDIALOG because list views can't have groups with no items present.
			LVGROUP defaultGroup
			{
				.cbSize = sizeof(LVGROUP),
				.mask = LVGF_GROUPID,
				.iGroupId = VisibleGroupId,
			};

			LVGROUP hiddenGroup
			{
				.cbSize = sizeof(LVGROUP),
				.mask = LVGF_GROUPID | LVGF_STATE,
				.iGroupId = FilteredGroupId,
				.stateMask = LVGS_HIDDEN,
				.state = LVGS_HIDDEN,
			};

			ListView_InsertGroup(pluginListHandle, -1, &defaultGroup);
			ListView_InsertGroup(pluginListHandle, -1, &hiddenGroup);

			if (UITheme::IsEnabledMode())
				pluginList.SetStyle(pluginList.GetStyle() | LVS_OWNERDRAWFIXED);

			// Bethesda probably doesn't know about the existence of Check. 
			// They have created icons that mimic pictorially for the user.
			// I completely take everything from there, although I'm not happy about it, but this is a ready-made mechanism, and I'm just trying to make a search in it.
			HIMAGELIST hImageList = ListView_GetImageList(pluginListHandle, LVSIL_SMALL);
			ImageList_Destroy(hImageList);

			ImageList.ReCreate(16, 16, TRUE, Core::Classes::UI::ilct24Bit);

			if (UITheme::IsEnabledMode() && ((UITheme::Theme::GetTheme() == UITheme::Theme::Theme_Dark) ||
				(UITheme::Theme::GetTheme() == UITheme::Theme::Theme_DarkGray))) {
				ImageList.AddFromResource((HINSTANCE)g_hModule, MAKEINTRESOURCEA(IDB_BITMAP4), RGB(32, 32, 32));
				ImageList.AddFromResource((HINSTANCE)g_hModule, MAKEINTRESOURCEA(IDB_BITMAP2), RGB(32, 32, 32));
			}
			else {
				ImageList.AddFromResource((HINSTANCE)g_hModule, MAKEINTRESOURCEA(IDB_BITMAP3), RGB(255, 255, 255));
				ImageList.AddFromResource((HINSTANCE)g_hModule, MAKEINTRESOURCEA(IDB_BITMAP1), RGB(255, 255, 255));
			}

			ListView_SetImageList(pluginListHandle, ImageList.Handle, LVSIL_SMALL);
			
			// fix no checked in list 
			RedrawWindow(pluginListHandle, NULL, NULL, RDW_UPDATENOW | RDW_NOCHILDREN);

			return result;
		}
		else if (Message == WM_COMMAND)
		{
			// Text boxes use WM_COMMAND instead of WM_NOTIFY. Why? Well, who knows.
			if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == UI_DATA_DIALOG_FILTERBOX)
			{
				HWND pluginListHandle = GetDlgItem(DialogHwnd, UI_LISTVIEW_PLUGINS);
				char filter[1024] = {};

				GetWindowTextA(reinterpret_cast<HWND>(lParam), filter, static_cast<int>(std::ssize(filter)));

				if (strlen(filter) <= 0)
					// No filtering
					SendMessageA(pluginListHandle, LVM_ENABLEGROUPVIEW, FALSE, 0);
				else
				{
					SendMessageA(pluginListHandle, LVM_ENABLEGROUPVIEW, TRUE, 0);

					// Iterate over each item in the list, compare its file name text, then assign it to the relevant group
					int itemCount = ListView_GetItemCount(pluginListHandle);

					for (int i = 0; i < itemCount; i++)
					{
						char itemText[MAX_PATH] = {};

						LVITEMA getItem
						{
							.mask = LVIF_TEXT,
							.iItem = i,
							.iSubItem = 0,
							.pszText = itemText,
							.cchTextMax = static_cast<int>(std::ssize(itemText)),
						};

						ListView_GetItem(pluginListHandle, &getItem);

						// Case insensitive strstr
						bool isVisible = [&]()
						{
							for (auto c = getItem.pszText; *c != '\0'; c++)
							{
								if (_strnicmp(c, filter, strlen(filter)) == 0)
									return true;
							}

							return false;
						}();

						LVITEMA setItem
						{
							.mask = LVIF_GROUPID,
							.iItem = i,
							.iGroupId = isVisible ? VisibleGroupId : FilteredGroupId,
						};

						ListView_SetItem(pluginListHandle, &setItem);
					}
				}

				return 1;
			}
		}

		return OldDataDialogProc(DialogHwnd, Message, wParam, lParam);
	}
}