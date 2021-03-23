#include "../../common.h"
#include <CommCtrl.h>
#include <Richedit.h>
#include "ObjectWindow.h"
#include "EditorUI.h"

namespace DataDialogWindow
{
	constexpr int VisibleGroupId = 0;
	constexpr int FilteredGroupId = 1;

	DLGPROC OldDataDialogProc;

	INT_PTR CALLBACK DataDialogProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		if (Message == WM_INITDIALOG)
		{
			INT_PTR result = OldDataDialogProc(DialogHwnd, Message, wParam, lParam);
			HWND pluginListHandle = GetDlgItem(DialogHwnd, UI_DATA_DIALOG_PLUGINLISTVIEW);

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

			return result;
		}
		else if (Message == WM_COMMAND)
		{
			// Text boxes use WM_COMMAND instead of WM_NOTIFY. Why? Well, who knows.
			if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == UI_DATA_DIALOG_FILTERBOX)
			{
				HWND pluginListHandle = GetDlgItem(DialogHwnd, UI_DATA_DIALOG_PLUGINLISTVIEW);
				char filter[1024] = {};

				GetWindowTextA(reinterpret_cast<HWND>(lParam), filter, static_cast<int>(std::ssize(filter)));

				if (strlen(filter) <= 0)
				{
					// No filtering
					SendMessageA(pluginListHandle, LVM_ENABLEGROUPVIEW, FALSE, 0);
				}
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