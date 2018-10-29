#include <libdeflate/libdeflate.h>
#include <mutex>
#include <xmmintrin.h>
#include <smmintrin.h>
#include <windows.h>
#include <CommCtrl.h>
#include "../../common.h"
#include "Editor.h"

#pragma comment(lib, "libdeflate.lib")

struct z_stream_s
{
	const void *next_in;
	uint32_t avail_in;
	uint32_t total_in;
	void *next_out;
	uint32_t avail_out;
	uint32_t total_out;
	const char *msg;
	struct internal_state *state;
};

struct DialogOverrideData
{
	DLGPROC DialogFunc;	// Original function pointer
	bool IsDialog;		// True if it requires EndDialog()
};

std::recursive_mutex g_DialogMutex;
std::unordered_map<HWND, DialogOverrideData> g_DialogOverrides;
thread_local DialogOverrideData DlgData;

INT_PTR CALLBACK DialogFuncOverride(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Assert(IsWindow(hwndDlg));

	DLGPROC proc = nullptr;

	g_DialogMutex.lock();
	{
		if (auto itr = g_DialogOverrides.find(hwndDlg); itr != g_DialogOverrides.end())
			proc = itr->second.DialogFunc;

		// if (is new entry)
		if (!proc)
		{
			g_DialogOverrides[hwndDlg] = DlgData;
			proc = DlgData.DialogFunc;

			DlgData.DialogFunc = nullptr;
			DlgData.IsDialog = false;
		}

		// Purge old entries every now and then
		if (g_DialogOverrides.size() >= 50)
		{
			for (auto itr = g_DialogOverrides.begin(); itr != g_DialogOverrides.end();)
			{
				if (itr->first == hwndDlg || IsWindow(itr->first))
				{
					itr++;
					continue;
				}

				itr = g_DialogOverrides.erase(itr);
			}
		}
	}
	g_DialogMutex.unlock();

	return proc(hwndDlg, uMsg, wParam, lParam);
}

HWND WINAPI hk_CreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	// EndDialog MUST NOT be used
	DlgData.DialogFunc = lpDialogFunc;
	DlgData.IsDialog = false;

	return CreateDialogParamA(hInstance, lpTemplateName, hWndParent, DialogFuncOverride, dwInitParam);
}

INT_PTR WINAPI hk_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	// EndDialog MUST be used
	DlgData.DialogFunc = lpDialogFunc;
	DlgData.IsDialog = true;

	return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, DialogFuncOverride, dwInitParam);
}

BOOL WINAPI hk_EndDialog(HWND hDlg, INT_PTR nResult)
{
	Assert(hDlg);
	std::lock_guard<std::recursive_mutex> lock(g_DialogMutex);

	// Fix for the CK calling EndDialog on a CreateDialogParamA window
	if (auto itr = g_DialogOverrides.find(hDlg); itr != g_DialogOverrides.end() && !itr->second.IsDialog)
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
		std::lock_guard<std::recursive_mutex> lock(g_DialogMutex);

		// If this is a dialog, we can't call DestroyWindow on it
		if (auto itr = g_DialogOverrides.find(hWnd); itr != g_DialogOverrides.end())
		{
			if (!itr->second.IsDialog)
				DestroyWindow(hWnd);
		}

		return 0;
	}

	return SendMessageA(hWnd, Msg, wParam, lParam);
}

int hk_inflateInit(z_stream_s *Stream, const char *Version, int Mode)
{
	// Force inflateEnd to error out and skip frees
	Stream->state = nullptr;

	return 0;
}

int hk_inflate(z_stream_s *Stream, int Flush)
{
	size_t outBytes = 0;
	libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();

	libdeflate_result result = libdeflate_zlib_decompress(decompressor, Stream->next_in, Stream->avail_in, Stream->next_out, Stream->avail_out, &outBytes);
	libdeflate_free_decompressor(decompressor);

	if (result == LIBDEFLATE_SUCCESS)
	{
		Assert(outBytes < std::numeric_limits<uint32_t>::max());

		Stream->total_in = Stream->avail_in;
		Stream->total_out = (uint32_t)outBytes;

		return 1;
	}

	if (result == LIBDEFLATE_INSUFFICIENT_SPACE)
		return -5;

	return -2;
}

bool OpenPluginSaveDialog(HWND ParentWindow, const char *BasePath, bool IsESM, char *Buffer, uint32_t BufferSize, const char *Directory)
{
	if (!BasePath)
		BasePath = "\\Data";

	const char *filter = "TES Plugin Files (*.esp)\0*.esp\0TES Master Files (*.esm)\0*.esm\0\0";
	const char *title = "Select Target Plugin";
	const char *extension = "esp";

	if (IsESM)
	{
		filter = "TES Master Files (*.esm)\0*.esm\0\0";
		title = "Select Target Master";
		extension = "esm";
	}

	return ((bool(__fastcall *)(HWND, const char *, const char *, const char *, const char *, void *, bool, bool, char *, uint32_t, const char *, void *))
		(g_ModuleBase + 0x14824B0))(ParentWindow, BasePath, filter, title, extension, nullptr, false, true, Buffer, BufferSize, Directory, nullptr);
}

bool IsBSAVersionCurrent(class BSFile *File)
{
	char fullPath[MAX_PATH];
	GetCurrentDirectory(ARRAYSIZE(fullPath), fullPath);

	strcat_s(fullPath, "\\");
	strcat_s(fullPath, (const char *)((__int64)File + 0x64));

	HANDLE file = CreateFileA(fullPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (file != INVALID_HANDLE_VALUE)
	{
		struct
		{
			uint32_t Marker = 0;
			uint32_t Version = 0;
		} header;

		DWORD bytesRead;
		ReadFile(file, &header, sizeof(header), &bytesRead, nullptr);
		CloseHandle(file);

		// LE: Version 0x68
		// SSE: Version 0x69
		if (header.Marker != '\0ASB' || header.Version < 0x69)
			return false;

		return true;
	}

	return false;
}

bool IsLipDataPresent(void *Thisptr)
{
	char currentDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, currentDir);
	strcat_s(currentDir, "\\Data\\Sound\\Voice\\Processing\\Temp.lip");

	return GetFileAttributesA(currentDir) != INVALID_FILE_ATTRIBUTES;
}

bool WriteLipData(void *Thisptr, const char *Path, int Unkown1, bool Unknown2, bool Unknown3)
{
	char srcDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, srcDir);
	strcat_s(srcDir, "\\Data\\Sound\\Voice\\Processing\\Temp.lip");

	char destDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, destDir);
	strcat_s(destDir, "\\");
	strcat_s(destDir, Path);

	return MoveFile(srcDir, destDir) != FALSE;
}

std::vector<std::string> g_CCEslNames;

void ParseCreationClubContentFile()
{
	static int unused = []
	{
		if (FILE *f; fopen_s(&f, "Skyrim.ccc", "r") == 0)
		{
			char name[MAX_PATH];

			while (fgets(name, ARRAYSIZE(name), f))
			{
				if (strchr(name, '\n'))
					*strchr(name, '\n') = '\0';

				g_CCEslNames.push_back(name);
			}

			fclose(f);
		}

		return 0;
	}();
}

uint32_t GetESLMasterCount()
{
	ParseCreationClubContentFile();

	return (uint32_t)g_CCEslNames.size();
}

const char *GetESLMasterName(uint32_t Index)
{
	ParseCreationClubContentFile();

	if (Index < g_CCEslNames.size())
		return g_CCEslNames[Index].c_str();

	return nullptr;
}

bool IsESLMaster(const char *Name)
{
	ParseCreationClubContentFile();

	if (!Name || strlen(Name) <= 0)
		return false;

	for (std::string& s : g_CCEslNames)
	{
		if (!_stricmp(s.c_str(), Name))
			return true;
	}

	return false;
}

bool sub_141477DA0_SSE41(__int64 a1)
{
	// Checks if the 16-byte structure is 0 (list->next pointer, list->data pointer) (Branchless)
	__m128i data = _mm_loadu_si128((__m128i *)a1);
	return _mm_testz_si128(data, data);
}

bool sub_141477DA0(__int64 a1)
{
	// Checks if the 16-byte structure is 0 (list->next pointer, list->data pointer)
	return *(__int64 *)(a1 + 0) == 0 && *(__int64 *)(a1 + 8) == 0;
}

void UpdateLoadProgressBar()
{
	static float lastPercent = 0.0f;

	// Only update every quarter percent, rather than every single form load
	float newPercent = ((float)*(uint32_t *)0x143BBDA8C / (float)*(uint32_t *)0x143BBDA88) * 100.0f;

	if (abs(lastPercent - newPercent) <= 0.25f)
		return;

	((void(__fastcall *)())(g_ModuleBase + 0x13A4640))();
	lastPercent = newPercent;
}

void UpdateObjectWindowTreeView(void *Thisptr, HWND ControlHandle)
{
	SendMessage(ControlHandle, WM_SETREDRAW, FALSE, 0);
	((void(__fastcall *)(void *, HWND))(g_ModuleBase + 0x12D8710))(Thisptr, ControlHandle);
	SendMessage(ControlHandle, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(ControlHandle, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
}

bool g_UseDeferredDialogInsert;
HWND g_DeferredListView;
HWND g_DeferredComboBox;
uintptr_t g_DeferredStringLength;
bool g_AllowResize;
std::vector<std::pair<const char *, const char *>> g_DeferredMenuItems;

void ResetUIDefer()
{
	g_UseDeferredDialogInsert = false;
	g_DeferredListView = nullptr;
	g_DeferredComboBox = nullptr;
	g_DeferredStringLength = 0;
	g_AllowResize = false;
	g_DeferredMenuItems.clear();
}

void BeginUIDefer()
{
	ResetUIDefer();
	g_UseDeferredDialogInsert = true;
}

void EndUIDefer()
{
	if (!g_UseDeferredDialogInsert)
		return;

	if (g_DeferredListView)
	{
		SendMessage(g_DeferredListView, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(g_DeferredListView, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
	}

	if (g_DeferredMenuItems.size() > 0)
	{
		const HWND control = g_DeferredComboBox;

		SendMessage(control, WM_SETREDRAW, FALSE, 0);// Prevent repainting until finished
		SendMessage(control, CB_SETMINVISIBLE, 1, 0);// Possible optimization for older libraries (source: MSDN forums)

		// Sort alphabetically if requested, but do it manually instead of on every new item insertion
		LONG_PTR style = GetWindowLongPtr(control, GWL_STYLE);

		if ((style & CBS_SORT) == CBS_SORT)
		{
			LONG_PTR newStyle = SetWindowLongPtr(control, GWL_STYLE, style & ~CBS_SORT);
			Assert(newStyle != 0);

			std::sort(g_DeferredMenuItems.begin(), g_DeferredMenuItems.end(),
				[](const std::pair<const char *, const char *>& a, const std::pair<const char *, const char *>& b) -> bool
			{
				return strcmp(a.first, b.first) < 0;
			});
		}

		// Insert everything all at once
		SendMessage(control, CB_INITSTORAGE, g_DeferredMenuItems.size(), g_DeferredStringLength * sizeof(char));
		{
			HDC hdc = GetDC(control);
			uint32_t boxWidth = 0;

			Assert(hdc);

			for (auto&[display, value] : g_DeferredMenuItems)
			{
				LRESULT index = SendMessageA(control, CB_ADDSTRING, 0, (LPARAM)display);
				SIZE size;

				if (index != CB_ERR && index != CB_ERRSPACE)
					SendMessageA(control, CB_SETITEMDATA, index, (LPARAM)value);

				if (g_AllowResize && GetTextExtentPoint32A(hdc, display, (int)strlen(display), &size))
					boxWidth = std::max<uint32_t>(boxWidth, size.cx);
			}

			// Resize to fit
			if (g_AllowResize)
			{
				LRESULT currentWidth = SendMessage(control, CB_GETDROPPEDWIDTH, 0, 0);

				if (boxWidth > currentWidth)
					SendMessage(control, CB_SETDROPPEDWIDTH, boxWidth, 0);
			}

			ReleaseDC(control, hdc);
		}

		// Restore original style
		if ((style & CBS_SORT) == CBS_SORT)
			SetWindowLongPtr(control, GWL_STYLE, style);

		SendMessage(control, CB_SETMINVISIBLE, 30, 0);
		SendMessage(control, WM_SETREDRAW, TRUE, 0);
	}

	ResetUIDefer();
}

void InsertComboBoxItem(HWND ComboBoxHandle, const char *DisplayText, const char *Value, bool AllowResize)
{
	if (!ComboBoxHandle)
		return;

	if (!DisplayText)
		DisplayText = "NONE";

	if (g_UseDeferredDialogInsert)
	{
		AssertMsg(!g_DeferredComboBox || (g_DeferredComboBox == ComboBoxHandle), "Got handles to different combo boxes...? Reset probably wasn't called.");

		g_DeferredComboBox = ComboBoxHandle;
		g_DeferredStringLength += strlen(DisplayText);
		g_AllowResize |= AllowResize;
		g_DeferredMenuItems.emplace_back(DisplayText, Value);
		return;
	}

	if (AllowResize)
	{
		if (HDC hdc = GetDC(ComboBoxHandle); hdc)
		{
			if (SIZE size; GetTextExtentPoint32A(hdc, DisplayText, (int)strlen(DisplayText), &size))
			{
				LRESULT currentWidth = SendMessageA(ComboBoxHandle, CB_GETDROPPEDWIDTH, 0, 0);

				if (size.cx > currentWidth)
					SendMessageA(ComboBoxHandle, CB_SETDROPPEDWIDTH, size.cx, 0);
			}

			ReleaseDC(ComboBoxHandle, hdc);
		}
	}

	LRESULT index = SendMessageA(ComboBoxHandle, CB_ADDSTRING, 0, (LPARAM)DisplayText);

	if (index != CB_ERR && index != CB_ERRSPACE)
		SendMessageA(ComboBoxHandle, CB_SETITEMDATA, index, (LPARAM)Value);
}

void InsertListViewItem(HWND ListViewHandle, void *Parameter, bool UseImage, int ItemIndex)
{
	LVITEMA item;
	memset(&item, 0, sizeof(item));

	if (ItemIndex == -1)
		ItemIndex = 0x7FFFFFFF;

	item.mask = LVIF_PARAM | LVIF_TEXT;
	item.iItem = ItemIndex;
	item.lParam = (LPARAM)Parameter;
	item.pszText = LPSTR_TEXTCALLBACK;

	if (UseImage)
	{
		item.mask |= LVIF_IMAGE;
		item.iImage = I_IMAGECALLBACK;
	}

	if (g_UseDeferredDialogInsert)
	{
		AssertMsg(!g_DeferredListView || (g_DeferredListView == ListViewHandle), "Got handles to different list views? Reset probably wasn't called.");

		if (!g_DeferredListView)
		{
			g_DeferredListView = ListViewHandle;
			SendMessage(ListViewHandle, WM_SETREDRAW, FALSE, 0);
		}
	}

	SendMessageA(ListViewHandle, LVM_INSERTITEMA, 0, (LPARAM)&item);
}

void PatchTemplatedFormIterator()
{
	//
	// Add a callback that sets a global variable indicating UI dropdown menu entries can be
	// deferred to prevent redrawing/resorting after every new item insert, reducing dialog
	// initialization time.
	//
	// The _templated_ function is designed to iterate over all FORMs of a specific type - this
	// requires hooking 100-200 separate functions in the EXE as a result. False positives are
	// a non-issue as long as ctor/dtor calls are balanced.
	//
	const char *patternStr = "\xE8\x00\x00\x00\x00\x48\x89\x44\x24\x30\x48\x8B\x44\x24\x30\x48\x89\x44\x24\x38\x48\x8B\x54\x24\x38\x48\x8D\x4C\x24\x28";
	const char *maskStr = "x????xxxxxxxxxxxxxxxxxxxxxxxxx";

	for (uintptr_t i = g_CodeBase; i < g_CodeEnd;)
	{
		uintptr_t addr = FindPatternSimple(i, g_CodeEnd - i, (uint8_t *)patternStr, maskStr);

		if (!addr)
			break;

		i = addr + 1;

		// Make sure the next call points to sub_14102CBEF or it's a nop from ExperimentalPatchEditAndContinue
		addr += strlen(maskStr) + 11;
		uintptr_t destination = addr + *(int32_t *)(addr + 1) + 5;

		if (destination != 0x14102CBEF && *(uint8_t *)addr != 0x0F)
			continue;

		// Now look for the matching destructor call
		uintptr_t end = 0;

		for (int j = 0; j < 2; j++)
		{
			const char *endPattern;
			const char *endMask = "x????xxx????x";

			if (j == 0)
				endPattern = "\xE8\x00\x00\x00\x00\x48\x81\xC4\x00\x00\x00\x00\xC3";// sub_140FF81CE
			else
				endPattern = "\x0F\x00\x00\x00\x00\x48\x81\xC4\x00\x00\x00\x00\xC3";// nopped version

			end = FindPatternSimple(addr, std::min<uintptr_t>(g_CodeEnd - addr, 1000), (uint8_t *)endPattern, endMask);

			if (end)
				break;
		}

		if (!end)
			continue;

		Detours::X64::DetourFunctionClass((PBYTE)addr, &BeginUIDefer);
		PatchMemory(addr, (PBYTE)"\xE8", 1);
		Detours::X64::DetourFunctionClass((PBYTE)end, &EndUIDefer);
		PatchMemory(end, (PBYTE)"\xE8", 1);
	}
}

LRESULT CSScript_PickScriptsToCompileDlg_WindowMessage(void *Thisptr, UINT Message, WPARAM WParam, LPARAM LParam)
{
	thread_local bool disableListViewUpdates;

	auto updateListViewItems = [Thisptr]
	{
		if (!disableListViewUpdates)
			((void(__fastcall *)(void *))(g_ModuleBase + 0x20A9870))(Thisptr);
	};

	switch (Message)
	{
	case WM_SIZE:
		((void(__fastcall *)(void *))(g_ModuleBase + 0x20A9CF0))(Thisptr);
		break;

	case WM_NOTIFY:
	{
		LPNMHDR notification = (LPNMHDR)LParam;

		// "SysListView32" control
		if (notification->idFrom == 5401 && notification->code == LVN_ITEMCHANGED)
		{
			updateListViewItems();
			return 1;
		}
	}
	break;

	case WM_INITDIALOG:
		disableListViewUpdates = true;
		((void(__fastcall *)(void *))(g_ModuleBase + 0x20A99C0))(Thisptr);
		disableListViewUpdates = false;

		// Update it ONCE after everything is inserted
		updateListViewItems();
		break;

	case WM_COMMAND:
	{
		const uint32_t param = LOWORD(WParam);

		// "Check All", "Uncheck All", "Check All Checked-Out"
		if (param == 5474 || param == 5475 || param == 5602)
		{
			disableListViewUpdates = true;
			if (param == 5474)
				((void(__fastcall *)(void *))(g_ModuleBase + 0x20AA080))(Thisptr);
			else if (param == 5475)
				((void(__fastcall *)(void *))(g_ModuleBase + 0x20AA130))(Thisptr);
			else if (param == 5602)
				((void(__fastcall *)(void *))(g_ModuleBase + 0x20AA1E0))(Thisptr);
			disableListViewUpdates = false;

			updateListViewItems();
			return 1;
		}
		else if (param == 1)
		{
			// "Compile" button
			((void(__fastcall *)(void *))(g_ModuleBase + 0x20A9F30))(Thisptr);
		}
	}
	break;
	}

	return ((LRESULT(__fastcall *)(void *, UINT, WPARAM, LPARAM))(g_ModuleBase + 0x20ABD90))(Thisptr, Message, WParam, LParam);
}

void DialogueInfoSort(__int64 TESDataHandler, uint32_t FormType, void *SortFunction)
{
	static std::unordered_map<uintptr_t, std::pair<uintptr_t, uint32_t>> arrayCache;

	__int64 arrayInstance = TESDataHandler + 24i64 * FormType + 104;
	__int64 arrayBase = *(__int64 *)(arrayInstance);
	uint32_t count = *(uint32_t *)(arrayInstance + 0x10);

	auto itr = arrayCache.find(arrayInstance);

	// If not previously found or any counters changed...
	if (itr == arrayCache.end() || itr->second.first != arrayBase || itr->second.second != count)
	{
		arrayCache.erase(itr);
		arrayCache.emplace(arrayInstance, std::make_pair(arrayBase, count));

		// Update and resort the array
		((void(__fastcall *)(__int64, void *))(g_ModuleBase + 0x1651590))(arrayInstance, SortFunction);
	}
}

bool BSShaderResourceManager::FindIntersectionsTriShapeFastPath(class NiPoint3 *P1, class NiPoint3 *P2, class NiPick *Pick, class BSTriShape *Shape)
{
	// Pretend this is a regular BSTriShape when using BSMultiIndexTriShape
	uint8_t& type = *(uint8_t *)((uintptr_t)Shape + 0x150);
	uint8_t oldType = type;

	if (type == 7)
		type = 3;

	bool result = ((bool(__thiscall *)(void *, void *, void *, void *, void *))(g_ModuleBase + 0x2E17BE0))(this, P1, P2, Pick, Shape);

	if (oldType == 7)
		type = 7;

	return result;
}

void QuitHandler()
{
	ExitProcess(0);
}