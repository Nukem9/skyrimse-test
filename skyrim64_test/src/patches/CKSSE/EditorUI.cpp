#include "../../common.h"
#include "EditorUI.h"
#include "EditorUIDarkMode.h"
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

#include "UIThemeMode.h"
#include "MainWindow.h"
#include "LogWindow.h"

namespace EditorUI
{
	struct DialogOverrideData
	{
		DLGPROC DialogFunc;	// Original function pointer
		bool IsDialog;		// True if it requires EndDialog()
	};

	std::recursive_mutex DialogOverrideMutex;
	std::unordered_map<HWND, DialogOverrideData> DialogOverrides;
	thread_local DialogOverrideData ThreadDialogData;

	bool UseDeferredDialogInsert;
	HWND DeferredListView;
	HWND DeferredComboBox;
	uintptr_t DeferredStringLength;
	bool DeferredAllowResize;
	std::vector<std::pair<const char*, void*>> DeferredMenuItems;

	void Initialize()
	{
		InitCommonControls();

		UITheme::InitializeThread();
		EditorUIDarkMode::InitializeThread();
		MainWindow::Initialize();

		if (!LogWindow::Initialize())
			MessageBoxA(nullptr, "Failed to create console log window", "Error", MB_ICONERROR);

		if (g_INI.GetBoolean("CreationKit", "FaceFXDebugOutput", false))
		{
			if (!LogWindow::CreateStdoutListener())
				MessageBoxA(nullptr, "Failed to create output listener for external processes", "Error", MB_ICONERROR);
		}
	}

	HWND WINAPI hk_CreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
	{
		// EndDialog MUST NOT be used
		ThreadDialogData.DialogFunc = lpDialogFunc;
		ThreadDialogData.IsDialog = false;

		// Override certain default dialogs to use this DLL's resources
		switch (reinterpret_cast<uintptr_t>(lpTemplateName))
		{
		case 0x7A:// "Object Window"
		case 0x8D:// "Reference"
		case 0xA2:// "Data"
		case 0xAF:// "Cell View"
		case 0xDC:// "Use Report"
			hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
			break;
		}

		return CreateDialogParamA(hInstance, lpTemplateName, hWndParent, DialogFuncOverride, dwInitParam);
	}

	INT_PTR WINAPI hk_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
	{
		// EndDialog MUST be used
		ThreadDialogData.DialogFunc = lpDialogFunc;
		ThreadDialogData.IsDialog = true;

		// Override certain default dialogs to use this DLL's resources
		switch (reinterpret_cast<uintptr_t>(lpTemplateName))
		{
		case 0x7A:// "Object Window"
		case 0x8D:// "Reference"
		case 0xA2:// "Data"
		case 0xAF:// "Cell View"
		case 0xDC:// "Use Report"
			hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
			break;
		}

		return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, DialogFuncOverride, dwInitParam);
	}

	BOOL WINAPI hk_EndDialog(HWND hDlg, INT_PTR nResult)
	{
		std::lock_guard lock(DialogOverrideMutex);

		// Fix for the CK calling EndDialog on a CreateDialogParamA window
		if (auto itr = DialogOverrides.find(hDlg); itr != DialogOverrides.end() && !itr->second.IsDialog)
		{
			DestroyWindow(hDlg);
			return TRUE;
		}

		return EndDialog(hDlg, nResult);
	}

	LRESULT WINAPI hk_SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		if (hWnd && Msg == WM_DESTROY)
		{
			std::lock_guard lock(DialogOverrideMutex);

			// If this is a dialog, we can't call DestroyWindow on it
			if (auto itr = DialogOverrides.find(hWnd); itr != DialogOverrides.end())
			{
				if (!itr->second.IsDialog)
					DestroyWindow(hWnd);
			}

			return 0;
		}

		return SendMessageA(hWnd, Msg, wParam, lParam);
	}

	BOOL WINAPI hk_EnableWindow(HWND hwndDlg, BOOL bEnable) {
		return EnableWindow(hwndDlg, TRUE);
	}

	INT_PTR CALLBACK DialogFuncOverride(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DLGPROC proc = nullptr;

		DialogOverrideMutex.lock();
		{
			if (auto itr = DialogOverrides.find(hwndDlg); itr != DialogOverrides.end())
				proc = itr->second.DialogFunc;

			// if (is new entry)
			if (!proc)
			{
				DialogOverrides[hwndDlg] = ThreadDialogData;
				proc = ThreadDialogData.DialogFunc;

				ThreadDialogData.DialogFunc = nullptr;
				ThreadDialogData.IsDialog = false;
			}

			// Purge old entries every now and then
			if (DialogOverrides.size() >= 50)
			{
				for (auto itr = DialogOverrides.begin(); itr != DialogOverrides.end();)
				{
					if (itr->first == hwndDlg || IsWindow(itr->first))
					{
						itr++;
						continue;
					}

					itr = DialogOverrides.erase(itr);
				}
			}
		}
		DialogOverrideMutex.unlock();

		return proc(hwndDlg, uMsg, wParam, lParam);
	}

	void ListViewInsertItemDeferred(HWND ListViewHandle, void* Parameter, bool UseImage, int ItemIndex)
	{
		if (ItemIndex == -1)
			ItemIndex = INT_MAX;

		LVITEMA item
		{
			.mask = LVIF_PARAM | LVIF_TEXT,
			.iItem = ItemIndex,
			.pszText = LPSTR_TEXTCALLBACK,
			.lParam = reinterpret_cast<LPARAM>(Parameter)
		};

		if (UseImage)
		{
			item.mask |= LVIF_IMAGE;
			item.iImage = I_IMAGECALLBACK;
		}

		if (UseDeferredDialogInsert)
		{
			AssertMsg(!DeferredListView || (DeferredListView == ListViewHandle), "Got handles to different list views? Reset probably wasn't called.");

			if (!DeferredListView)
			{
				DeferredListView = ListViewHandle;
				SendMessage(ListViewHandle, WM_SETREDRAW, FALSE, 0);
			}
		}

		ListView_InsertItem(ListViewHandle, &item);
	}

	BOOL ListViewSetItemState(HWND ListViewHandle, WPARAM Index, UINT Data, UINT Mask)
	{
		// Microsoft's implementation of this define is broken (ListView_SetItemState)
		LVITEMA item
		{
			.mask = LVIF_STATE,
			.state = Data,
			.stateMask = Mask
		};

		return static_cast<BOOL>(SendMessageA(ListViewHandle, LVM_SETITEMSTATE, Index, reinterpret_cast<LPARAM>(&item)));
	}

	void ListViewSelectItem(HWND ListViewHandle, int ItemIndex, bool KeepOtherSelections)
	{
		if (!KeepOtherSelections)
			ListViewSetItemState(ListViewHandle, -1, 0, LVIS_SELECTED);

		if (ItemIndex != -1)
		{
			ListView_EnsureVisible(ListViewHandle, ItemIndex, FALSE);
			ListViewSetItemState(ListViewHandle, ItemIndex, LVIS_SELECTED, LVIS_SELECTED);
		}
	}

	void ListViewFindAndSelectItem(HWND ListViewHandle, void* Parameter, bool KeepOtherSelections)
	{
		if (!KeepOtherSelections)
			ListViewSetItemState(ListViewHandle, -1, 0, LVIS_SELECTED);

		LVFINDINFOA findInfo
		{
			.flags = LVFI_PARAM,
			.lParam = reinterpret_cast<LPARAM>(Parameter)
		};

		int index = ListView_FindItem(ListViewHandle, -1, &findInfo);

		if (index != -1)
			ListViewSelectItem(ListViewHandle, index, KeepOtherSelections);
	}

	void* ListViewGetSelectedItem(HWND ListViewHandle)
	{
		if (!ListViewHandle)
			return nullptr;

		int index = ListView_GetNextItem(ListViewHandle, -1, LVNI_SELECTED);

		if (index == -1)
			return nullptr;

		LVITEMA item
		{
			.mask = LVIF_PARAM,
			.iItem = index
		};

		ListView_GetItem(ListViewHandle, &item);
		return reinterpret_cast<void*>(item.lParam);
	}

	void ListViewDeselectItem(HWND ListViewHandle, void* Parameter)
	{
		LVFINDINFOA findInfo
		{
			.flags = LVFI_PARAM,
			.lParam = reinterpret_cast<LPARAM>(Parameter)
		};

		int index = ListView_FindItem(ListViewHandle, -1, &findInfo);

		if (index != -1)
			ListViewSetItemState(ListViewHandle, index, 0, LVIS_SELECTED);
	}

	void* ListViewGetUserData(HWND ListViewHandle, int ItemIndex) {
		if (!ListViewHandle || ItemIndex == -1)
			return nullptr;

		LVITEMA item
		{
			.mask = LVIF_PARAM,
			.iItem = ItemIndex
		};

		ListView_GetItem(ListViewHandle, &item);
		return reinterpret_cast<void*>(item.lParam);
	}

	void ComboBoxInsertItemDeferred(HWND ComboBoxHandle, const char *DisplayText, void *Value, bool AllowResize)
	{
		if (!ComboBoxHandle)
			return;

		if (!DisplayText)
			DisplayText = "NONE";

		if (UseDeferredDialogInsert)
		{
			AssertMsg(!DeferredComboBox || (DeferredComboBox == ComboBoxHandle), "Got handles to different combo boxes? Reset probably wasn't called.");

			DeferredComboBox = ComboBoxHandle;
			DeferredStringLength += strlen(DisplayText) + 1;
			DeferredAllowResize |= AllowResize;

			// A copy must be created since lifetime isn't guaranteed after this function returns
			DeferredMenuItems.emplace_back(_strdup(DisplayText), Value);
		}
		else
		{
			if (AllowResize)
			{
				if (HDC hdc = GetDC(ComboBoxHandle); hdc)
				{
					if (SIZE size; GetTextExtentPoint32A(hdc, DisplayText, static_cast<int>(strlen(DisplayText)), &size))
					{
						LRESULT currentWidth = SendMessageA(ComboBoxHandle, CB_GETDROPPEDWIDTH, 0, 0);

						if (size.cx > currentWidth)
							SendMessageA(ComboBoxHandle, CB_SETDROPPEDWIDTH, size.cx, 0);
					}

					ReleaseDC(ComboBoxHandle, hdc);
				}
			}

			LRESULT index = SendMessageA(ComboBoxHandle, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(DisplayText));

			if (index != CB_ERR && index != CB_ERRSPACE)
				SendMessageA(ComboBoxHandle, CB_SETITEMDATA, index, reinterpret_cast<LPARAM>(Value));
		}
	}

	void TabControlDeleteItem(HWND TabControlHandle, uint32_t TabIndex)
	{
		TCITEMA itemInfo = {};

		if (TabCtrl_GetItem(TabControlHandle, TabIndex, &itemInfo))
			TabCtrl_DeleteItem(TabControlHandle, TabIndex);
	}

	void RegisterHotkeyFunction(void *Thisptr, void(*Callback)(), const char **HotkeyFunction, const char **DisplayText, char VirtualKey, bool Alt, bool Ctrl, bool Shift)
	{
#if DUMP_KEYBINDS
		std::string decodedKey;

		if (Ctrl)
			decodedKey += "CTRL+";

		if (Shift)
			decodedKey += "SHIFT+";

		if (Alt)
			decodedKey += "ALT+";

		if (VirtualKey == VK_ESCAPE)
			decodedKey += "ESC";
		else if (VirtualKey == VK_DELETE)
			decodedKey += "DEL";
		else if (VirtualKey == VK_TAB)
			decodedKey += "TAB";
		else if (VirtualKey >= VK_F1 && VirtualKey <= VK_F12)
			decodedKey += "F" + std::to_string(VirtualKey - VK_F1 + 1);
		else if (VirtualKey < VK_PRIOR || VirtualKey > VK_F15)
			decodedKey += static_cast<char>(MapVirtualKeyA(VirtualKey, MAPVK_VK_TO_CHAR));
		else
			decodedKey += VirtualKey;

		std::string logLine;
		logLine += *HotkeyFunction;
		logLine += "=\"" + decodedKey + "\"";

		for (int i = 60 - static_cast<int>(logLine.length()); i > 0; i--)
			logLine += " ";

		logLine += "; ";
		logLine += *DisplayText;

		LogWindow::Log(logLine.c_str());
#endif

		// Read the setting, strip spaces/quotes, then split by each '+' modifier
		std::string newKeybind = g_INI.Get("CreationKit_Hotkeys", *HotkeyFunction, "");

		for (size_t i; (i = newKeybind.find("\"")) != std::string::npos;)
			newKeybind.replace(i, 1, "");

		for (size_t i; (i = newKeybind.find(" ")) != std::string::npos;)
			newKeybind.replace(i, 1, "");

		if (!newKeybind.empty())
		{
			std::transform(newKeybind.begin(), newKeybind.end(), newKeybind.begin(), toupper);

			VirtualKey = 0;
			Alt = false;
			Ctrl = false;
			Shift = false;

			char *context = nullptr;
			const char *t = strtok_s(newKeybind.data(), "+", &context);

			do
			{
				if (!strcmp(t, "CTRL"))
					Ctrl = true;
				else if (!strcmp(t, "SHIFT"))
					Shift = true;
				else if (!strcmp(t, "ALT"))
					Alt = true;
				else if (!strcmp(t, "ESC"))
					VirtualKey = VK_ESCAPE;
				else if (!strcmp(t, "DEL"))
					VirtualKey = VK_DELETE;
				else if (!strcmp(t, "TAB"))
					VirtualKey = VK_TAB;
				else if (strlen(t) > 1 && t[0] == 'F')
				{
					// Parse function keys F1 to F12
					int index = atoi(&t[1]);

					AssertMsgVa(index >= 1 && index <= 12, "Invalid function key index '%s' for hotkey function '%s'", t, *HotkeyFunction);

					VirtualKey = VK_F1 + index - 1;
				}
				else
				{
					// Parse a regular character
					AssertMsgVa(strlen(t) == 1, "Invalid or unknown key binding '%s' for hotkey function '%s'", t, *HotkeyFunction);

					// This should be translated with VkKeyScan but virtual keys make things difficult...
					VirtualKey = t[0];
				}
			} while (t = strtok_s(nullptr, "+", &context));
		}

		((decltype(&RegisterHotkeyFunction))OFFSET(0x12FCB70, 1530))(Thisptr, Callback, HotkeyFunction, DisplayText, VirtualKey, Alt, Ctrl, Shift);
	}

	void ResetUIDefer()
	{
		UseDeferredDialogInsert = false;
		DeferredListView = nullptr;
		DeferredComboBox = nullptr;
		DeferredStringLength = 0;
		DeferredAllowResize = false;
		DeferredMenuItems.clear();
	}

	void BeginUIDefer()
	{
		ResetUIDefer();
		UseDeferredDialogInsert = true;
	}

	void EndUIDefer()
	{
		if (!UseDeferredDialogInsert)
			return;

		if (DeferredListView)
		{
			SendMessage(DeferredListView, WM_SETREDRAW, TRUE, 0);
			RedrawWindow(DeferredListView, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
		}

		if (!DeferredMenuItems.empty())
		{
			const HWND control = DeferredComboBox;

			// Sort alphabetically if requested to try and speed up inserts
			int finalWidth = 0;
			LONG_PTR style = GetWindowLongPtr(control, GWL_STYLE);

			if ((style & CBS_SORT) == CBS_SORT)
			{
				std::sort(DeferredMenuItems.begin(), DeferredMenuItems.end(),
					[](const auto& a, const auto& b) -> bool
				{
					return _stricmp(a.first, b.first) > 0;
				});
			}

			SendMessage(control, CB_INITSTORAGE, DeferredMenuItems.size(), DeferredStringLength * sizeof(char));

			if (HDC hdc = GetDC(control); hdc)
			{
				SuspendComboBoxUpdates(control, true);

				// Pre-calculate font widths for resizing, starting with TrueType
				std::array<int, UCHAR_MAX + 1> fontWidths;
				std::array<ABC, UCHAR_MAX + 1> trueTypeFontWidths;

				if (!GetCharABCWidthsA(hdc, 0, static_cast<UINT>(trueTypeFontWidths.size() - 1), trueTypeFontWidths.data()))
				{
					BOOL result = GetCharWidthA(hdc, 0, static_cast<UINT>(fontWidths.size() - 1), fontWidths.data());
					AssertMsg(result, "Failed to determine any font widths");
				}
				else
				{
					for (int i = 0; i < fontWidths.size(); i++)
						fontWidths[i] = trueTypeFontWidths[i].abcB;
				}

				// Insert everything all at once
				for (auto [display, value] : DeferredMenuItems)
				{
					LRESULT index = SendMessageA(control, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display));
					int lineSize = 0;

					if (index != CB_ERR && index != CB_ERRSPACE)
						SendMessageA(control, CB_SETITEMDATA, index, reinterpret_cast<LPARAM>(value));

					for (const char *c = display; *c != '\0'; c++)
						lineSize += fontWidths[*c];

					finalWidth = std::max(finalWidth, lineSize);

					free(const_cast<char *>(display));
				}

				SuspendComboBoxUpdates(control, false);
				ReleaseDC(control, hdc);
			}

			// Resize to fit
			if (DeferredAllowResize)
			{
				LRESULT currentWidth = SendMessage(control, CB_GETDROPPEDWIDTH, 0, 0);

				if (finalWidth > currentWidth)
					SendMessage(control, CB_SETDROPPEDWIDTH, finalWidth, 0);
			}
		}

		ResetUIDefer();
	}

	void SuspendComboBoxUpdates(HWND ComboHandle, bool Suspend)
	{
		COMBOBOXINFO info
		{
			.cbSize = sizeof(COMBOBOXINFO)
		};

		if (!GetComboBoxInfo(ComboHandle, &info))
			return;

		if (!Suspend)
		{
			SendMessage(info.hwndList, WM_SETREDRAW, TRUE, 0);
			SendMessage(ComboHandle, CB_SETMINVISIBLE, 30, 0);
			SendMessage(ComboHandle, WM_SETREDRAW, TRUE, 0);
		}
		else
		{
			SendMessage(ComboHandle, WM_SETREDRAW, FALSE, 0);// Prevent repainting until finished
			SendMessage(ComboHandle, CB_SETMINVISIBLE, 1, 0);// Possible optimization for older libraries (source: MSDN forums)
			SendMessage(info.hwndList, WM_SETREDRAW, FALSE, 0);
		}
	}
}