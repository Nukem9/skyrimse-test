#include "../../common.h"
#include <tbb/concurrent_unordered_map.h>
#include <vssym32.h>
#include <Richedit.h>
#include "EditorUIDarkMode.h"

namespace EditorUIDarkMode
{
	enum class ThemeType
	{
		None,
		ScrollBar,
		StatusBar,
		MDIClient,
		Static,
		Edit,
		RichEdit,
		Button,
		ComboBox,
		Header,
		ListView,
		TreeView,
		TabControl,
	};

	const std::unordered_map<std::string_view, ThemeType> TargetWindowThemes
	{
		{ "msctls_statusbar32", ThemeType::StatusBar },
		{ "MDIClient", ThemeType::MDIClient },
		{ "Static", ThemeType::Static },
		{ "Edit", ThemeType::Edit },
		{ "RichEdit20A", ThemeType::RichEdit },
		{ "RICHEDIT50W", ThemeType::RichEdit },
		{ "Button", ThemeType::Button },
		{ "ComboBox", ThemeType::ComboBox },
		{ "SysHeader32", ThemeType::Header },
		{ "SysListView32", ThemeType::ListView },
		{ "SysTreeView32", ThemeType::TreeView },
		{ "SysTabControl32", ThemeType::TabControl },
	};

	const std::unordered_set<std::string_view> PermanentWindowSubclasses
	{
		"Creation Kit",
		"Creation Kit SE",
		"Creation Kit SSE",
		"ActivatorClass",
		"AlchemyClass",
		"ArmorClass",
		"CreatureClass",
		"LockPickClass",
		"NPCClass",
		"WeaponClass",
		"FaceClass",
		"PlaneClass",
		"MonitorClass",
		"ViewerClass",
		"SpeakerClass",
		"LandClass",
		"RenderWindow",
		"#32770",
		// "BABYGRID",
		// "NiTreeCtrl",
	};

	bool EnableThemeHooking;
	tbb::concurrent_unordered_map<HTHEME, ThemeType> ThemeHandles;

	void Initialize()
	{
		EnableThemeHooking = true;
	}

	BOOL IsInitialize() {
		return EnableThemeHooking;
	}

	void InitializeThread()
	{
		if (EnableThemeHooking)
			SetWindowsHookExA(WH_CALLWNDPROC, CallWndProcCallback, nullptr, GetCurrentThreadId());
	}

	LRESULT CALLBACK CallWndProcCallback(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION)
		{
			auto messageData = reinterpret_cast<CWPSTRUCT *>(lParam);

			switch (messageData->message)
			{
			case WM_CREATE:
				SetWindowSubclass(messageData->hwnd, WindowSubclass, 0, reinterpret_cast<DWORD_PTR>(WindowSubclass));
				break;

			case WM_INITDIALOG:
				SetWindowSubclass(messageData->hwnd, DialogWindowSubclass, 0, reinterpret_cast<DWORD_PTR>(DialogWindowSubclass));
				break;
			}
		}

		return CallNextHookEx(nullptr, nCode, wParam, lParam);
	}

	LRESULT CALLBACK WindowSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		constexpr COLORREF generalBackgroundColor = RGB(56, 56, 56);
		constexpr COLORREF generalTextColor = RGB(255, 255, 255);
		static HBRUSH generalBackgroundBrush = CreateSolidBrush(generalBackgroundColor);

		switch (uMsg)
		{
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (HDC hdc = reinterpret_cast<HDC>(wParam); hdc)
			{
				SetTextColor(hdc, generalTextColor);
				SetBkColor(hdc, generalBackgroundColor);
			}

			return reinterpret_cast<INT_PTR>(generalBackgroundBrush);
		}
		break;
		}

		LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

		switch (uMsg)
		{
		case WM_INITDIALOG:
		case WM_CREATE:
		{
			// Theme settings are initialized after WM_CREATE is processed
			auto themeType = ThemeType::None;
			HTHEME scrollBarTheme = nullptr;

			char className[256] = {};
			GetClassName(hWnd, className, ARRAYSIZE(className));

			if (auto itr = TargetWindowThemes.find(className); itr != TargetWindowThemes.end())
				themeType = itr->second;

			switch (themeType)
			{
			case ThemeType::MDIClient:
				SetWindowSubclass(hWnd, MDIClientSubclass, 0, 0);
				break;

			case ThemeType::RichEdit:
			{
				CHARFORMAT2A format = {};
				format.cbSize = sizeof(format);
				format.dwMask = CFM_COLOR;
				format.crTextColor = RGB(255, 255, 255);
				SendMessageA(hWnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
				SendMessageA(hWnd, EM_SETBKGNDCOLOR, 0, RGB(56, 56, 56));
			}
			break;

			case ThemeType::ListView:
				SetWindowSubclass(hWnd, ListViewSubclass, 0, 0);

				ListView_SetTextColor(hWnd, RGB(255, 255, 255));
				ListView_SetTextBkColor(hWnd, RGB(32, 32, 32));
				ListView_SetBkColor(hWnd, RGB(32, 32, 32));

				scrollBarTheme = OpenThemeData(hWnd, VSCLASS_SCROLLBAR);
				break;

			case ThemeType::TreeView:
				TreeView_SetTextColor(hWnd, RGB(255, 255, 255));
				TreeView_SetBkColor(hWnd, RGB(32, 32, 32));

				scrollBarTheme = OpenThemeData(hWnd, VSCLASS_SCROLLBAR);
				break;

			case ThemeType::TabControl:
				SetWindowLongPtrA(hWnd, GWL_STYLE, (GetWindowLongPtrA(hWnd, GWL_STYLE) & ~TCS_BUTTONS) | TCS_TABS);
				SetWindowTheme(hWnd, nullptr, nullptr);
				break;
			}

			if (scrollBarTheme)
			{
				ThemeHandles.emplace(scrollBarTheme, ThemeType::ScrollBar);

				// TODO: This is a hack...the handle should be valid as long as at least one window is still open
				CloseThemeData(scrollBarTheme);
			}

			if (HTHEME windowTheme = GetWindowTheme(hWnd); windowTheme)
				ThemeHandles.emplace(windowTheme, themeType);

			if (!PermanentWindowSubclasses.count(className))
				RemoveWindowSubclass(hWnd, reinterpret_cast<SUBCLASSPROC>(dwRefData), 0);
		}
		break;
		}

		return result;
	}

	LRESULT CALLBACK DialogWindowSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		constexpr COLORREF generalBackgroundColor = RGB(56, 56, 56);
		static HBRUSH generalBackgroundBrush = CreateSolidBrush(generalBackgroundColor);

		LRESULT result = WindowSubclass(hWnd, uMsg, wParam, lParam, uIdSubclass, dwRefData);

		switch (uMsg)
		{
		case WM_PAINT:
		{
			// Special override for DialogBoxIndirectParam (MessageBox) since the bottom half doesn't get themed correctly. ReactOS
			// says this is MSGBOX_IDTEXT. Standard message boxes will have 3 buttons or less.
			if (GetDlgItem(hWnd, 0xFFFF))
			{
				int buttonCount = 0;

				EnumChildWindows(hWnd, [](HWND ChildWindow, LPARAM Param)
				{
					char classname[256] = {};
					GetClassName(ChildWindow, classname, std::size(classname));

					if (!_stricmp(classname, "Button"))
						(*reinterpret_cast<int *>(Param))++;

					return TRUE;
				}, reinterpret_cast<LPARAM>(&buttonCount));

				if (buttonCount <= 3)
				{
					if (HDC hdc = GetDC(hWnd); hdc)
					{
						RECT windowArea;
						GetClientRect(hWnd, &windowArea);

						FillRect(hdc, &windowArea, generalBackgroundBrush);
						ReleaseDC(hWnd, hdc);
					}
				}
			}
		}
		break;
		}

		return result;
	}

	LRESULT CALLBACK ListViewSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		switch (uMsg)
		{
		case WM_PAINT:
		{
			// Paint normally, then apply custom grid lines
			LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

			if (HDC hdc = GetDC(hWnd); hdc)
			{
				RECT headerRect;
				GetClientRect(ListView_GetHeader(hWnd), &headerRect);

				RECT listRect;
				GetClientRect(hWnd, &listRect);

				HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(DC_PEN));
				int x = 0 - GetScrollPos(hWnd, SB_HORZ);

				LVCOLUMN colInfo
				{
					.mask = LVCF_WIDTH,
				};

				for (int col = 0; ListView_GetColumn(hWnd, col, &colInfo); col++)
				{
					x += colInfo.cx;

					// Stop drawing if outside the listview client area
					if (x >= listRect.right)
						break;

					// Right border
					std::array<POINT, 2> verts
					{{
						{ x - 2, headerRect.bottom },
						{ x - 2, listRect.bottom },
					}};

					SetDCPenColor(hdc, RGB(65, 65, 65));
					Polyline(hdc, verts.data(), verts.size());

					// Right border shadow
					verts[0].x += 1;
					verts[1].x += 1;

					SetDCPenColor(hdc, RGB(29, 38, 48));
					Polyline(hdc, verts.data(), verts.size());
				}

				SelectObject(hdc, oldPen);
				ReleaseDC(hWnd, hdc);
			}

			return result;
		}

		case LVM_SETEXTENDEDLISTVIEWSTYLE:
		{
			// Prevent the OS grid separators from drawing
			wParam &= ~static_cast<WPARAM>(LVS_EX_GRIDLINES);
			lParam &= ~static_cast<LPARAM>(LVS_EX_GRIDLINES);
		}
		break;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	LRESULT CALLBACK MDIClientSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		static HBRUSH backgroundColorBrush = CreateSolidBrush(RGB(32, 32, 32));

		switch (uMsg)
		{
		case WM_PAINT:
		{
			// Custom background color
			if (HDC hdc = GetDC(hWnd); hdc)
			{
				RECT windowArea;
				GetClientRect(hWnd, &windowArea);

				FillRect(hdc, &windowArea, backgroundColorBrush);
				ReleaseDC(hWnd, hdc);
			}
		}
		break;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	DWORD WINAPI Comctl32GetSysColor(int nIndex)
	{
		switch (nIndex)
		{
		case COLOR_BTNFACE: return RGB(56, 56, 56);
		case COLOR_BTNTEXT: return RGB(255, 255, 255);
		}

		return GetSysColor(nIndex);
	}

	HBRUSH WINAPI Comctl32GetSysColorBrush(int nIndex)
	{
		static HBRUSH btnFaceBrush = CreateSolidBrush(Comctl32GetSysColor(COLOR_BTNFACE));
		static HBRUSH btnTextBrush = CreateSolidBrush(Comctl32GetSysColor(COLOR_BTNTEXT));

		switch (nIndex)
		{
		case COLOR_BTNFACE: return btnFaceBrush;
		case COLOR_BTNTEXT: return btnTextBrush;
		}

		return GetSysColorBrush(nIndex);
	}

	HRESULT WINAPI Comctl32DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, LPCRECT pRect)
	{
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(255, 255, 255));

		RECT temp = *pRect;
		DrawTextW(hdc, pszText, cchText, &temp, dwTextFlags);

		return S_OK;
	}

	HRESULT WINAPI Comctl32DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, LPCRECT pClipRect)
	{
		auto themeType = ThemeType::None;

		if (auto itr = ThemeHandles.find(hTheme); itr != ThemeHandles.end())
			themeType = itr->second;

		if (themeType == ThemeType::ScrollBar)
		{
			static HBRUSH scrollbarFill = CreateSolidBrush(RGB(77, 77, 77));
			static HBRUSH scrollbarFillHighlighted = CreateSolidBrush(RGB(122, 122, 122));
			static HBRUSH scrollbarBackground = CreateSolidBrush(RGB(23, 23, 23));

			switch (iPartId)
			{
			case SBP_THUMBBTNHORZ:	// Horizontal drag bar
			case SBP_THUMBBTNVERT:	// Vertical drag bar
			{
				if (iStateId == SCRBS_HOT || iStateId == SCRBS_PRESSED)
					FillRect(hdc, pRect, scrollbarFillHighlighted);
				else
					FillRect(hdc, pRect, scrollbarFill);
			}
			return S_OK;

			case SBP_LOWERTRACKHORZ:// Horizontal background
			case SBP_UPPERTRACKHORZ:// Horizontal background
			case SBP_LOWERTRACKVERT:// Vertical background
			case SBP_UPPERTRACKVERT:// Vertical background
			{
				FillRect(hdc, pRect, scrollbarBackground);
			}
			return S_OK;

			case SBP_ARROWBTN:		// Arrow button
			{
				// Assume the perspective of the arrow pointing upward ( /\ ) in GDI coordinates. NOTE: (0, 0) is the
				// top left corner of the screen. Awful code, but it works.
				const int arrowWidth = std::ceil(std::abs(pRect->left - pRect->right) * 0.4f);
				const int arrowHeight = std::ceil(std::abs(pRect->top - pRect->bottom) * 0.35f);

				std::array<DWORD, 6> counts { 2, 2, 2, 2, 2, 2 };
				std::array<POINT, 12> verts
				{{
					// Left segment
					{ 0, -0 },
					{ (arrowWidth / 2) + 1, -arrowHeight + 2 },

					{ 0, -1 },
					{ (arrowWidth / 2) + 1, -arrowHeight + 1 },

					{ 0, -2 },
					{ (arrowWidth / 2) + 1, -arrowHeight + 0 },

					// Right segment (final vertex Y adjusted to avoid a stray pixel)
					{ arrowWidth - 1, -0 },
					{ arrowWidth / 2, -arrowHeight + 2 },

					{ arrowWidth - 1, -1 },
					{ arrowWidth / 2, -arrowHeight + 1 },

					{ arrowWidth - 1, -2 },
					{ arrowWidth / 2, -arrowHeight + 1 },
				}};

				bool isHot = false;
				bool isDisabled = false;

				for (auto& vert : verts)
				{
					switch (iStateId)
					{
					case ABS_UPHOT:// Up
					case ABS_UPPRESSED:
						isHot = true;
					case ABS_UPDISABLED:
						isDisabled = true;
					case ABS_UPNORMAL:
					case ABS_UPHOVER:
						vert.x += pRect->left + arrowHeight - 1;
						vert.y += pRect->bottom - arrowHeight;
						break;

					case ABS_DOWNHOT:// Down
					case ABS_DOWNPRESSED:
						isHot = true;
					case ABS_DOWNDISABLED:
						isDisabled = true;
					case ABS_DOWNNORMAL:
					case ABS_DOWNHOVER:
						vert.x += pRect->left + arrowHeight - 1;
						vert.y = -vert.y + pRect->top + arrowHeight - 1;
						break;

					case ABS_LEFTHOT:// Left
					case ABS_LEFTPRESSED:
						isHot = true;
					case ABS_LEFTDISABLED:
						isDisabled = true;
					case ABS_LEFTNORMAL:
					case ABS_LEFTHOVER:
						std::swap(vert.x, vert.y);
						vert.x += pRect->right - arrowHeight;
						vert.y += pRect->top + arrowHeight - 1;
						break;

					case ABS_RIGHTHOT:// Right
					case ABS_RIGHTPRESSED:
						isHot = true;
					case ABS_RIGHTDISABLED:
						isDisabled = true;
					case ABS_RIGHTNORMAL:
					case ABS_RIGHTHOVER:
						std::swap(vert.x, vert.y);
						vert.x = -vert.x + pRect->left + arrowHeight - 1;
						vert.y += pRect->top + arrowHeight - 1;
						break;
					}
				}

				HBRUSH fillColor = scrollbarFill;
				HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(DC_PEN));

				if (isHot)
					fillColor = scrollbarFillHighlighted;
				else if (isDisabled)
					fillColor = scrollbarFill;

				FillRect(hdc, pRect, fillColor);

				SetDCPenColor(hdc, RGB(255, 255, 255));
				PolyPolyline(hdc, verts.data(), counts.data(), counts.size());

				SelectObject(hdc, oldPen);
			}
			return S_OK;

			case SBP_GRIPPERHORZ:	// Horizontal resize scrollbar
			case SBP_GRIPPERVERT:	// Vertical resize scrollbar
			case SBP_SIZEBOX:		// Resize box, bottom right
			case SBP_SIZEBOXBKGND:	// Resize box, background, unused
				break;
			}
		}
		else if (themeType == ThemeType::StatusBar)
		{
			static HBRUSH statusBarBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
			static HBRUSH statusBarFill = CreateSolidBrush(RGB(56, 56, 56));

			switch (iPartId)
			{
			case 0:
			{
				// Outside border (top, right)
				FillRect(hdc, pRect, statusBarBorder);
			}
			return S_OK;

			case SP_PANE:
			case SP_GRIPPERPANE:
			case SP_GRIPPER:
			{
				// Everything else
				FillRect(hdc, pRect, statusBarFill);
			}
			return S_OK;
			}
		}
		else if (themeType == ThemeType::Edit)
		{
			static HBRUSH editControlBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
			static HBRUSH editControlFill = CreateSolidBrush(RGB(32, 32, 32));

			switch (iPartId)
			{
			case EP_EDITBORDER_NOSCROLL:
			{
				FillRect(hdc, pRect, editControlFill);
				FrameRect(hdc, pRect, editControlBorder);
			}
			return S_OK;
			}
		}
		else if (themeType == ThemeType::Button)
		{
			static HBRUSH buttonBorder = CreateSolidBrush(RGB(130, 135, 144));
			static HBRUSH buttonBorderHighlighted = CreateSolidBrush(RGB(0, 120, 215));
			static HBRUSH buttonFill = CreateSolidBrush(RGB(32, 32, 32));
			static HBRUSH buttonPressed = CreateSolidBrush(RGB(83, 83, 83));

			switch (iPartId)
			{
			case BP_PUSHBUTTON:
			{
				HBRUSH frameColor = buttonBorder;
				HBRUSH fillColor = buttonFill;

				switch (iStateId)
				{
				case PBS_HOT:
				case PBS_DEFAULTED:
					frameColor = buttonBorderHighlighted;
					break;

				case PBS_DISABLED:
					fillColor = buttonBorder;
					break;

				case PBS_PRESSED:
					fillColor = buttonPressed;
					break;
				}

				FillRect(hdc, pRect, fillColor);
				FrameRect(hdc, pRect, frameColor);
			}
			return S_OK;

			case BP_CHECKBOX:
			{
				if (iStateId == CBS_UNCHECKEDDISABLED ||
					iStateId == CBS_CHECKEDDISABLED ||
					iStateId == CBS_MIXEDDISABLED ||
					iStateId == CBS_IMPLICITDISABLED ||
					iStateId == CBS_EXCLUDEDDISABLED)
				{
					FrameRect(hdc, pRect, buttonBorder);
					return S_OK;
				}
			}
			break;
			}
		}
		else if (themeType == ThemeType::ComboBox)
		{
			static HBRUSH comboBoxBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
			static HBRUSH comboBoxFill = CreateSolidBrush(RGB(32, 32, 32));

			switch (iPartId)
			{
			case CP_READONLY:			// Main control
			{
				FillRect(hdc, pRect, iStateId == CBRO_DISABLED ? comboBoxBorder : comboBoxFill);
				FrameRect(hdc, pRect, comboBoxBorder);
			}
			return S_OK;

			case CP_BORDER:				// Main control with text edit
			{
				// Special case: dropdown arrow needs to be drawn
				DrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
				FillRect(hdc, pRect, comboBoxFill);

				if (iStateId == CBB_DISABLED)
					FillRect(hdc, pRect, comboBoxBorder);
				else
					FrameRect(hdc, pRect, comboBoxBorder);
			}
			return S_OK;

			case CP_DROPDOWNBUTTONRIGHT:// Dropdown arrow
			case CP_DROPDOWNBUTTONLEFT:	// Dropdown arrow
				break;

			case CP_DROPDOWNBUTTON:
			case CP_BACKGROUND:
			case CP_TRANSPARENTBACKGROUND:
			case CP_CUEBANNER:
			case CP_DROPDOWNITEM:
				return S_OK;
			}
		}
		else if (themeType == ThemeType::Header)
		{
			static HBRUSH headerFill = CreateSolidBrush(RGB(77, 77, 77));

			switch (iPartId)
			{
			case 0:
			case HP_HEADERITEM:
			case HP_HEADERITEMLEFT:
			case HP_HEADERITEMRIGHT:
			{
				HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(DC_PEN));

				for (int i = 0; i < 2; i++)
				{
					if (i == 0)
						SetDCPenColor(hdc, RGB(65, 65, 65));
					else
						SetDCPenColor(hdc, RGB(29, 38, 48));

					std::array<DWORD, 2> counts { 2, 2 };
					std::array<POINT, 4> verts
					{{
						// Right border
						{ pRect->right - 2 + i, pRect->top },
						{ pRect->right - 2 + i, pRect->bottom },

						// Bottom border
						{ pRect->left - 1, pRect->bottom - 2 + i },
						{ pRect->right - 2, pRect->bottom - 2 + i },
					}};

					PolyPolyline(hdc, verts.data(), counts.data(), counts.size());
				}

				// Fill background on hover (1px padding for border shadow)
				if ((iPartId == 0 && iStateId == HIS_HOT) ||
					(iPartId == HP_HEADERITEM && iStateId == HIS_HOT) ||
					(iPartId == HP_HEADERITEMLEFT && iStateId == HILS_HOT) ||
					(iPartId == HP_HEADERITEMRIGHT && iStateId == HIRS_HOT))
				{
					RECT padded = { pRect->left - 1, pRect->top, pRect->right - 1, pRect->bottom - 1 };

					FillRect(hdc, &padded, headerFill);
				}

				SelectObject(hdc, oldPen);
			}
			return S_OK;
			}
		}
		else if (themeType == ThemeType::TabControl)
		{
			static HBRUSH tabControlButtonBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
			static HBRUSH tabControlButtonFill = CreateSolidBrush(RGB(56, 56, 56));

			switch (iPartId)
			{
			case TABP_TABITEM:				// TCS_MULTILINE middle buttons
			case TABP_TABITEMLEFTEDGE:		// TCS_MULTILINE leftmost button
			case TABP_TABITEMRIGHTEDGE:		// TCS_MULTILINE rightmost button
			case TABP_TABITEMBOTHEDGE:		// TCS_MULTILINE ???
			case TABP_TOPTABITEM:			// Middle buttons
			case TABP_TOPTABITEMLEFTEDGE:	// Leftmost button
			case TABP_TOPTABITEMRIGHTEDGE:	// Rightmost button
			case TABP_TOPTABITEMBOTHEDGE:	// ???
			{
				RECT paddedRect = *pRect;
				RECT insideRect = { pRect->left + 1, pRect->top + 1, pRect->right - 1, pRect->bottom - 1 };

				bool isHover = (iPartId == TABP_TABITEM && iStateId == TIS_HOT) ||
					(iPartId == TABP_TABITEMLEFTEDGE && iStateId == TILES_HOT) ||
					(iPartId == TABP_TABITEMRIGHTEDGE && iStateId == TIRES_HOT) ||
					(iPartId == TABP_TABITEMBOTHEDGE && iStateId == TIBES_HOT) ||
					(iPartId == TABP_TOPTABITEM && iStateId == TTIS_HOT) ||
					(iPartId == TABP_TOPTABITEMLEFTEDGE && iStateId == TTILES_HOT) ||
					(iPartId == TABP_TOPTABITEMRIGHTEDGE && iStateId == TTIRES_HOT) ||
					(iPartId == TABP_TOPTABITEMBOTHEDGE && iStateId == TTIBES_HOT);

				if ((iPartId == TABP_TABITEM && iStateId == TIS_SELECTED) ||
					(iPartId == TABP_TABITEMLEFTEDGE && iStateId == TILES_SELECTED) ||
					(iPartId == TABP_TABITEMRIGHTEDGE && iStateId == TIRES_SELECTED) ||
					(iPartId == TABP_TABITEMBOTHEDGE && iStateId == TIBES_SELECTED) ||
					(iPartId == TABP_TOPTABITEM && iStateId == TTIS_SELECTED) ||
					(iPartId == TABP_TOPTABITEMLEFTEDGE && iStateId == TTILES_SELECTED) ||
					(iPartId == TABP_TOPTABITEMRIGHTEDGE && iStateId == TTIRES_SELECTED) ||
					(iPartId == TABP_TOPTABITEMBOTHEDGE && iStateId == TTIBES_SELECTED))
				{
					paddedRect.top += 1;
					paddedRect.bottom -= 2;

					// Allow the rect to overlap so the bottom border outline is removed
					insideRect.top += 1;
					insideRect.bottom += 1;
				}

				FrameRect(hdc, &paddedRect, tabControlButtonBorder);
				FillRect(hdc, &insideRect, isHover ? tabControlButtonBorder : tabControlButtonFill);
			}
			return S_OK;

			case TABP_PANE:
				return S_OK;
			}
		}

		return DrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
	}
}