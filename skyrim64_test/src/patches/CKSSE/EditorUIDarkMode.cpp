#include "../../common.h"
#include <tbb/concurrent_unordered_map.h>
#include <vssym32.h>
#include <Richedit.h>
#include "LogWindow.h"
#include "EditorUIDarkMode.h"

#pragma comment(lib, "uxtheme.lib")

enum class CustomThemeType
{
	None,
	ScrollBar,
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

tbb::concurrent_unordered_map<HTHEME, CustomThemeType> g_ThemeHandles;
bool g_EnableTheming;

void EditorUIDarkMode_Initialize()
{
	g_EnableTheming = true;
}

BOOL CALLBACK EditorUIDarkMode_EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	CustomThemeType themeType = CustomThemeType::None;
	HTHEME scrollBarTheme = nullptr;

	char className[256] = {};
	GetClassName(hWnd, className, ARRAYSIZE(className));

	if (!_stricmp(className, "MDICLIENT"))
	{
		SetWindowSubclass(hWnd, EditorUIDarkMode_MDIClientSubclass, 0, 0);

		themeType = CustomThemeType::MDIClient;
	}
	else if (!_stricmp(className, "Static"))
		themeType = CustomThemeType::Static;
	else if (!_stricmp(className, "Edit"))
		themeType = CustomThemeType::Edit;
	else if (!_stricmp(className, "RICHEDIT50W"))
	{
		CHARFORMAT2A format = {};
		format.cbSize = sizeof(format);
		format.dwMask = CFM_COLOR;
		format.crTextColor = RGB(255, 255, 255);
		SendMessageA(hWnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
		SendMessageA(hWnd, EM_SETBKGNDCOLOR, FALSE, RGB(56, 56, 56));

		themeType = CustomThemeType::Edit;
	}
	else if (!_stricmp(className, "Button"))
		themeType = CustomThemeType::Button;
	else if (!_stricmp(className, "ComboBox"))
		themeType = CustomThemeType::ComboBox;
	else if (!_stricmp(className, "SysHeader32"))
		themeType = CustomThemeType::Header;
	else if (!_stricmp(className, "SysListView32"))
	{
		SetWindowSubclass(hWnd, EditorUIDarkMode_ListViewSubclass, 0, 0);

		ListView_SetTextColor(hWnd, RGB(255, 255, 255));
		ListView_SetTextBkColor(hWnd, RGB(32, 32, 32));
		ListView_SetBkColor(hWnd, RGB(32, 32, 32));

		themeType = CustomThemeType::ListView;
		scrollBarTheme = OpenThemeData(hWnd, VSCLASS_SCROLLBAR);
	}
	else if (!_stricmp(className, "SysTreeView32"))
	{
		TreeView_SetTextColor(hWnd, RGB(255, 255, 255));
		TreeView_SetBkColor(hWnd, RGB(32, 32, 32));

		themeType = CustomThemeType::TreeView;
		scrollBarTheme = OpenThemeData(hWnd, VSCLASS_SCROLLBAR);
	}
	else if (!_stricmp(className, "SysTabControl32"))
	{
		SetWindowLongPtrA(hWnd, GWL_STYLE, (GetWindowLongPtrA(hWnd, GWL_STYLE) & ~TCS_BUTTONS) | TCS_TABS);
		SetWindowTheme(hWnd, nullptr, nullptr);

		themeType = CustomThemeType::TabControl;
	}
	else
	{
		EditorUI_Log("Unhandled class type: %s\n", className);
	}

	if (scrollBarTheme)
	{
		g_ThemeHandles.emplace(scrollBarTheme, CustomThemeType::ScrollBar);

		// TODO: This is a hack...the handle should be valid as long as at least one window is still open
		CloseThemeData(scrollBarTheme);
	}

	if (HTHEME windowTheme = GetWindowTheme(hWnd); windowTheme)
		g_ThemeHandles.emplace(windowTheme, themeType);

	return TRUE;
}

std::optional<INT_PTR> EditorUIDarkMode_ApplyMessageHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!g_EnableTheming)
		return std::nullopt;

	constexpr COLORREF generalBackgroundColor = RGB(56, 56, 56);
	constexpr COLORREF generalTextColor = RGB(255, 255, 255);
	static HBRUSH generalBackgroundBrush = CreateSolidBrush(generalBackgroundColor);

	switch (uMsg)
	{
	case WM_INITDIALOG:
	case WM_CREATE:
	{
		// Prevent running this code on the same window multiple times
		if (!static_cast<bool>(GetProp(hWnd, "CKSSE_DMINIT")))
		{
			EditorUIDarkMode_EnumWindowsProc(hWnd, 0);
			EnumChildWindows(hWnd, EditorUIDarkMode_EnumWindowsProc, 0);

			SetProp(hWnd, "CKSSE_DMINIT", reinterpret_cast<HANDLE>(true));
		}
	}
	break;

	case WM_DESTROY:
	{
		// NOTE/TODO: Properties are leaked when using DialogBoxParam or anything with EndDialog()
		RemoveProp(hWnd, "CKSSE_DMINIT");
	}
	break;

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

	return std::nullopt;
}

LRESULT CALLBACK EditorUIDarkMode_ListViewSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		// Paint normally, then apply custom grid lines
		LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

		RECT headerRect;
		GetClientRect(ListView_GetHeader(hWnd), &headerRect);

		RECT listRect;
		GetClientRect(hWnd, &listRect);

		if (HDC hdc = GetDC(hWnd); hdc)
		{
			HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(DC_PEN));
			int x = 0 - GetScrollPos(hWnd, SB_HORZ);

			LVCOLUMN colInfo;
			colInfo.mask = LVCF_WIDTH;

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

	case WM_NOTIFY:
	{
		if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW)
		{
			auto customDraw = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

			switch (customDraw->dwDrawStage)
			{
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;

			case CDDS_ITEMPREPAINT:
			{
				// Newer versions of windows have bugs: this is only ever called for header items
				SetTextColor(customDraw->hdc, RGB(211, 211, 211));
			}
			return CDRF_DODEFAULT;
			}
		}
	}
	break;

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

LRESULT CALLBACK EditorUIDarkMode_MDIClientSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	static HBRUSH backgroundColorBrush = CreateSolidBrush(RGB(32, 32, 32));

	switch (uMsg)
	{
	case WM_PAINT:
	{
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
	auto themeType = CustomThemeType::None;

	if (auto itr = g_ThemeHandles.find(hTheme); itr != g_ThemeHandles.end())
		themeType = itr->second;

	if (themeType == CustomThemeType::ScrollBar)
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
		case SBP_GRIPPERHORZ:	// Horizontal resize scrollbar
		case SBP_GRIPPERVERT:	// Vertical resize scrollbar
		case SBP_SIZEBOX:		// Resize box, bottom right
		case SBP_SIZEBOXBKGND:	// Resize box, background, unused
		break;
		}
	}
	else if (themeType == CustomThemeType::Edit)
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
	else if (themeType == CustomThemeType::Button)
	{
		static HBRUSH buttonBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
		static HBRUSH buttonBorderHighlighted = CreateSolidBrush(RGB(0, 120, 215));
		static HBRUSH buttonFill = CreateSolidBrush(RGB(32, 32, 32));

		switch (iPartId)
		{
		case BP_PUSHBUTTON:
		{
			bool isHighlight = iStateId == PBS_DEFAULTED || iStateId == PBS_HOT;

			FillRect(hdc, pRect, iStateId == PBS_DISABLED ? buttonBorder : buttonFill);
			FrameRect(hdc, pRect, isHighlight ? buttonBorderHighlighted : buttonBorder);
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
	else if (themeType == CustomThemeType::ComboBox)
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

		case CP_DROPDOWNBUTTONRIGHT:// Dropdown arrow
		case CP_DROPDOWNBUTTONLEFT:	// Dropdown arrow
		break;

		case CP_DROPDOWNBUTTON:
		case CP_BACKGROUND:
		case CP_TRANSPARENTBACKGROUND:
		case CP_BORDER:
		case CP_CUEBANNER:
		case CP_DROPDOWNITEM:
		return S_OK;
		}
	}
	else if (themeType == CustomThemeType::Header)
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

				std::array<DWORD, 2> counts = { 2, 2 };
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
	else if (themeType == CustomThemeType::TabControl)
	{
		static HBRUSH tabControlButtonBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
		static HBRUSH tabControlButtonFill = CreateSolidBrush(RGB(56, 56, 56));

		switch (iPartId)
		{
		case TABP_TOPTABITEM:			// Middle tab buttons
		case TABP_TOPTABITEMLEFTEDGE:	// Leftmost tab button
		case TABP_TOPTABITEMRIGHTEDGE:	// Rightmost tab button
		case TABP_TOPTABITEMBOTHEDGE:	// ???
		{
			RECT paddedRect = *pRect;
			RECT insideRect = { pRect->left + 1, pRect->top + 1, pRect->right - 1, pRect->bottom - 1 };

			bool isHover = (iPartId == TABP_TOPTABITEM && iStateId == TTIS_HOT) ||
				(iPartId == TABP_TOPTABITEMLEFTEDGE && iStateId == TTILES_HOT) ||
				(iPartId == TABP_TOPTABITEMRIGHTEDGE && iStateId == TTIRES_HOT) ||
				(iPartId == TABP_TOPTABITEMBOTHEDGE && iStateId == TTIBES_HOT);

			if ((iPartId == TABP_TOPTABITEM && iStateId == TTIS_SELECTED) ||
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