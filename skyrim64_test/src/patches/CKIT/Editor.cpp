#include "../../common.h"
#include <libdeflate/libdeflate.h>
#include <mutex>
#include <smmintrin.h>
#include <CommCtrl.h>
#include "../TES/MemoryManager.h"
#include "../TES/NiMain/NiColor.h"
#include "../TES/NiMain/NiPointer.h"
#include "../TES/NiMain/NiSourceTexture.h"
#include "../TES/NiMain/BSTriShape.h"
#include "../TES/BSShader/BSShaderProperty.h"
#include "../TES/BSShader/Shaders/BSEffectShaderMaterial.h"
#include "Editor.h"
#include "EditorUI.h"
#include "TESWater.h"

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

	// Override the default "Use Report" window sizing
	if (lpTemplateName == (LPCSTR)0xDC)
		hInstance = (HINSTANCE)&__ImageBase;

	return CreateDialogParamA(hInstance, lpTemplateName, hWndParent, DialogFuncOverride, dwInitParam);
}

INT_PTR WINAPI hk_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	// EndDialog MUST be used
	DlgData.DialogFunc = lpDialogFunc;
	DlgData.IsDialog = true;

	// Override the default "Data" dialog with a custom one in this dll
	if (lpTemplateName == (LPCSTR)0xA2)
		hInstance = (HINSTANCE)&__ImageBase;

	return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, DialogFuncOverride, dwInitParam);
}

BOOL WINAPI hk_EndDialog(HWND hDlg, INT_PTR nResult)
{
	Assert(hDlg);
	std::lock_guard lock(g_DialogMutex);

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
		std::lock_guard lock(g_DialogMutex);

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

void CreateLipGenProcess(__int64 a1)
{
	char procIdString[32];
	sprintf_s(procIdString, "%d", GetCurrentProcessId());
	SetEnvironmentVariableA("Ckpid", procIdString);

	char procToolPath[MAX_PATH];
	strcpy_s(procToolPath, (const char *)(a1 + 0x0));
	strcat_s(procToolPath, (const char *)(a1 + 0x104));

	PROCESS_INFORMATION *procInfo = (PROCESS_INFORMATION *)(a1 + 0x228);
	memset(procInfo, 0, sizeof(PROCESS_INFORMATION));

	STARTUPINFOA startupInfo;
	memset(&startupInfo, 0, sizeof(STARTUPINFOA));
	startupInfo.cb = sizeof(STARTUPINFOA);

	if (EditorUI_GetStdoutListenerPipe())
	{
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdError = EditorUI_GetStdoutListenerPipe();
		startupInfo.hStdOutput = EditorUI_GetStdoutListenerPipe();
		startupInfo.hStdInput = nullptr;
	}

	if (!CreateProcessA(procToolPath, nullptr, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startupInfo, procInfo))
		EditorUI_Log("FaceFXWrapper could not be started (%d). LIP generation will be disabled.\n", GetLastError());
	else
		EditorUI_Log("FaceFXWrapper background process started.\n");
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

	bool status = MoveFileEx(srcDir, destDir, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;

	EditorUI_Warning(6, "Moving temporary LIP file to '%s' (%s)", destDir, status ? "Succeeded" : "Failed");
	return status;
}

int IsWavDataPresent(const char *Path, __int64 a2, __int64 a3, __int64 a4)
{
	return ((int(__fastcall *)(const char *, __int64, __int64, __int64))(g_ModuleBase + 0x264D120))("Sound\\Voice\\Temp.wav", a2, a3, a4);
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

bool sub_141477DA0(__int64 a1)
{
	// Checks if the 16-byte structure is 0 (list->next pointer, list->data pointer)
	return *(__int64 *)(a1 + 0) == 0 && *(__int64 *)(a1 + 8) == 0;
}

bool sub_141477DA0_SSE41(__int64 a1)
{
	// Checks if the 16-byte structure is 0 (list->next pointer, list->data pointer) (Branchless)
	__m128i data = _mm_loadu_si128((__m128i *)a1);
	return _mm_testz_si128(data, data);
}

uint32_t sub_1414974E0(BSTArray<void *>& Array, const void *&Target, uint32_t StartIndex, __int64 Unused)
{
	for (uint32_t i = StartIndex; i < Array.QSize(); i++)
	{
		if (Array[i] == Target)
			return i;
	}

	return 0xFFFFFFFF;
}

uint32_t sub_1414974E0_SSE41(BSTArray<void *>& Array, const void *&Target, uint32_t StartIndex, __int64 Unused)
{
	const uint32_t size = Array.QSize();
	const __int64 *data = (const __int64 *)Array.QBuffer();

	const uint32_t comparesPerIter = 4;
	const uint32_t alignedSize = size & ~(comparesPerIter - 1);

	// Compare 4 pointers per iteration (Strips off the last 2 bits from array size)
	const __m128i targets = _mm_set1_epi64x((__int64)Target);

	for (uint32_t i = StartIndex; i < alignedSize; i += comparesPerIter)
	{
		//
		// Set bit 0 if 'a1'=='a2', set bit 1 if 'b1'=='b2', set bit X...
		// AVX: mask = _mm256_movemask_pd(_mm256_castsi256_pd(_mm256_cmpeq_epi64(targets, _mm256_loadu_si256((__m256i *)&data[i]))));
		//
		__m128i test1 = _mm_cmpeq_epi64(targets, _mm_loadu_si128((__m128i *)&data[i + 0]));
		__m128i test2 = _mm_cmpeq_epi64(targets, _mm_loadu_si128((__m128i *)&data[i + 2]));

		int mask = _mm_movemask_pd(_mm_castsi128_pd(_mm_or_si128(test1, test2)));

		if (mask != 0)
		{
			for (; i < size; i++)
			{
				if (data[i] == (__int64)Target)
					return i;
			}

			__debugbreak();
			__assume(0);
		}
	}

	for (uint32_t i = alignedSize; i < size; i++)
	{
		if (data[i] == (__int64)Target)
			return i;
	}

	return 0xFFFFFFFF;
}

bool sub_1415D5640(__int64 a1, uint32_t *a2)
{
	while (a1 && (*(uint32_t *)a1 != *a2))
		a1 = *(__int64 *)(a1 + 8);

	return a1 != 0;
}

void UpdateLoadProgressBar()
{
	static float lastPercent = 0.0f;

	// Only update every quarter percent, rather than every single form load
	float newPercent = ((float)*(uint32_t *)(g_ModuleBase + 0x3BBDA8C) / (float)*(uint32_t *)(g_ModuleBase + 0x3BBDA88)) * 100.0f;

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

void UpdateCellViewCellList(void *Thisptr, HWND ControlHandle, __int64 Unknown)
{
	SendMessage(ControlHandle, WM_SETREDRAW, FALSE, 0);
	((void(__fastcall *)(void *, HWND, __int64))(g_ModuleBase + 0x147FA70))(Thisptr, ControlHandle, Unknown);
	SendMessage(ControlHandle, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(ControlHandle, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
}

void UpdateCellViewObjectList(void *Thisptr, HWND *ControlHandle)
{
	SendMessage(*ControlHandle, WM_SETREDRAW, FALSE, 0);
	((void(__fastcall *)(void *, HWND *))(g_ModuleBase + 0x13E0CE0))(Thisptr, ControlHandle);
	SendMessage(*ControlHandle, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(*ControlHandle, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
}

bool g_UseDeferredDialogInsert;
HWND g_DeferredListView;
HWND g_DeferredComboBox;
uintptr_t g_DeferredStringLength;
bool g_AllowResize;
std::vector<std::pair<const char *, void *>> g_DeferredMenuItems;

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

void SuspendComboBoxUpdates(HWND ComboHandle, bool Suspend)
{
	COMBOBOXINFO info = {};
	info.cbSize = sizeof(COMBOBOXINFO);

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

void EndUIDefer()
{
	if (!g_UseDeferredDialogInsert)
		return;

	if (g_DeferredListView)
	{
		SendMessage(g_DeferredListView, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(g_DeferredListView, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
	}

	if (!g_DeferredMenuItems.empty())
	{
		const HWND control = g_DeferredComboBox;

		// Sort alphabetically if requested to try and speed up inserts
		int finalWidth = 0;
		LONG_PTR style = GetWindowLongPtr(control, GWL_STYLE);

		if ((style & CBS_SORT) == CBS_SORT)
		{
			std::sort(g_DeferredMenuItems.begin(), g_DeferredMenuItems.end(),
				[](const std::pair<const char *, void *>& a, const std::pair<const char *, void *>& b) -> bool
			{
				return _stricmp(a.first, b.first) > 0;
			});
		}

		SendMessage(control, CB_INITSTORAGE, g_DeferredMenuItems.size(), g_DeferredStringLength * sizeof(char));

		if (HDC hdc = GetDC(control); hdc)
		{
			SuspendComboBoxUpdates(control, true);

			// Pre-calculate font widths for resizing, starting with TrueType
			int fontWidths[UCHAR_MAX + 1];
			ABC trueTypeFontWidths[UCHAR_MAX + 1];

			if (!GetCharABCWidthsA(hdc, 0, ARRAYSIZE(trueTypeFontWidths) - 1, trueTypeFontWidths))
			{
				BOOL result = GetCharWidthA(hdc, 0, ARRAYSIZE(fontWidths) - 1, fontWidths);
				AssertMsg(result, "Failed to determine any font widths");
			}
			else
			{
				for (int i = 0; i < ARRAYSIZE(fontWidths); i++)
					fontWidths[i] = trueTypeFontWidths[i].abcB;
			}

			// Insert everything all at once
			for (auto [display, value] : g_DeferredMenuItems)
			{
				LRESULT index = SendMessageA(control, CB_ADDSTRING, 0, (LPARAM)display);
				int lineSize = 0;

				if (index != CB_ERR && index != CB_ERRSPACE)
					SendMessageA(control, CB_SETITEMDATA, index, (LPARAM)value);

				for (const char *c = display; *c != '\0'; c++)
					lineSize += fontWidths[*c];

				finalWidth = std::max<int>(finalWidth, lineSize);

				free((void *)display);
			}

			SuspendComboBoxUpdates(control, false);
			ReleaseDC(control, hdc);
		}

		// Resize to fit
		if (g_AllowResize)
		{
			LRESULT currentWidth = SendMessage(control, CB_GETDROPPEDWIDTH, 0, 0);

			if (finalWidth > currentWidth)
				SendMessage(control, CB_SETDROPPEDWIDTH, finalWidth, 0);
		}
	}

	ResetUIDefer();
}

void InsertComboBoxItem(HWND ComboBoxHandle, const char *DisplayText, void *Value, bool AllowResize)
{
	if (!ComboBoxHandle)
		return;

	if (!DisplayText)
		DisplayText = "NONE";

	if (g_UseDeferredDialogInsert)
	{
		AssertMsg(!g_DeferredComboBox || (g_DeferredComboBox == ComboBoxHandle), "Got handles to different combo boxes? Reset probably wasn't called.");

		g_DeferredComboBox = ComboBoxHandle;
		g_DeferredStringLength += strlen(DisplayText) + 1;
		g_AllowResize |= AllowResize;

		// A copy must be created since lifetime isn't guaranteed after this function returns
		g_DeferredMenuItems.emplace_back(_strdup(DisplayText), Value);
	}
	else
	{
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
}

void InsertListViewItem(HWND ListViewHandle, void *Parameter, bool UseImage, int ItemIndex)
{
	if (ItemIndex == -1)
		ItemIndex = INT_MAX;

	LVITEMA item;
	memset(&item, 0, sizeof(item));

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

	ListView_InsertItem(ListViewHandle, &item);
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
		uintptr_t addr = XUtil::FindPattern(i, g_CodeEnd - i, (uint8_t *)patternStr, maskStr);

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

			end = XUtil::FindPattern(addr, std::min<uintptr_t>(g_CodeEnd - addr, 1000), (uint8_t *)endPattern, endMask);

			if (end)
				break;
		}

		if (!end)
			continue;

		// Blacklisted (000000014148C1FF): The "Use Info" dialog which has more than one list view and causes problems
		// Blacklisted (000000014169DFAD): Adding a new faction to an NPC has more than one list view
		if (addr == 0x000000014148C1FF || addr == 0x000000014169DFAD)
			continue;

		XUtil::DetourCall(addr, &BeginUIDefer);
		XUtil::DetourCall(end, &EndUIDefer);
	}
}

void SortFormArray(BSTArray<class TESForm *> *Array, int(*SortFunction)(const void *, const void *))
{
	// NOTE: realSort must be updated on a separate line to avoid one-time initialization
	thread_local int(__fastcall *realSort)(const void *, const void *);
	realSort = SortFunction;

	qsort(Array->QBuffer(), Array->QSize(), sizeof(class TESForm *), [](const void *a, const void *b)
	{
		return realSort(*(const void **)a, *(const void **)b);
	});
}

void SortDialogueInfo(__int64 TESDataHandler, uint32_t FormType, int(*SortFunction)(const void *, const void *))
{
	static std::unordered_map<BSTArray<class TESForm *> *, std::pair<void *, uint32_t>> arrayCache;

	auto *formArray = &((BSTArray<class TESForm *> *)(TESDataHandler + 104))[FormType];
	auto itr = arrayCache.find(formArray);

	// If not previously found or any counters changed...
	if (itr == arrayCache.end() || itr->second.first != formArray->QBuffer() || itr->second.second != formArray->QSize())
	{
		// Update and resort the array
		SortFormArray(formArray, SortFunction);

		arrayCache[formArray] = std::make_pair(formArray->QBuffer(), formArray->QSize());
	}
}

bool BSShaderResourceManager::FindIntersectionsTriShapeFastPath(class NiPoint3 *Origin, class NiPoint3 *Dir, class NiPick *Pick, class BSTriShape *Shape)
{
	// Pretend this is a regular BSTriShape when using BSMultiIndexTriShape
	uint8_t& type = *(uint8_t *)((uintptr_t)Shape + 0x150);
	uint8_t oldType = type;

	if (type == 7)
		type = 3;

	bool result = ((bool(__thiscall *)(void *, void *, void *, void *, void *))(g_ModuleBase + 0x2E17BE0))(this, Origin, Dir, Pick, Shape);

	if (oldType == 7)
		type = 7;

	return result;
}

void QuitHandler()
{
	TerminateProcess(GetCurrentProcess(), 0);
}

void hk_sub_141047AB2(__int64 FileHandle, __int64 *Value)
{
	// The pointer is converted to a form id, but the top 32 bits are never cleared correctly (ui32/ui64 union)
	*Value &= 0x00000000FFFFFFFF;

	((void(__fastcall *)(__int64, __int64 *))(g_ModuleBase + 0x15C88D0))(FileHandle, Value);
}

bool hk_BGSPerkRankArray_sub_14168DF70(PerkRankEntry *Entry, uint32_t *FormId, __int64 UnknownArray)
{
	AutoFunc(__int64(*)(__int64, __int64), sub_1416B8A20, 0x16B8A20);
	AutoFunc(__int64(*)(uint32_t *, __int64), sub_1416C05D0, 0x16C05D0);
	AutoFunc(__int64(*)(void *, uint32_t *), sub_14168E790, 0x168E790);

	AssertMsg(Entry->Rank == 0, "NPC perk loading code is still broken somewhere, rank shouldn't be set");

	// Bugfix: Always zero the form pointer union, even if the form id was null
	*FormId = Entry->FormId;
	Entry->FormIdOrPointer = 0;

	if (*FormId && UnknownArray)
	{
		sub_1416C05D0(FormId, sub_1416B8A20(UnknownArray, 0xFFFFFFFFi64));
		Entry->FormIdOrPointer = sub_14168E790(nullptr, FormId);

		if (Entry->FormIdOrPointer)
		{
			(*(void(__fastcall **)(__int64, __int64))(*(__int64 *)Entry->FormIdOrPointer + 520i64))(Entry->FormIdOrPointer, UnknownArray);
			return true;
		}

		return false;
	}

	return true;
}

void hk_BGSPerkRankArray_sub_14168EAE0(__int64 ArrayHandle, PerkRankEntry *&Entry)
{
	//
	// This function does two things:
	// - Correct the "rank" value which otherwise defaults to 1 and zero the remaining bits
	// - Prevent insertion of the perk in the array if it's null
	//
	AssertMsg(Entry->NewRank == 1, "Unexpected default for new rank member conversion");

	Entry->NewRank = Entry->Rank;
	Entry->FormIdOrPointer &= 0x00000000FFFFFFFF;

	if (Entry->FormId != 0)
		((void(__fastcall *)(__int64, PerkRankEntry *&))(g_ModuleBase + 0x168EAE0))(ArrayHandle, Entry);
	else
		EditorUI_Warning(13, "Null perk found while loading a PerkRankArray. Entry will be discarded.");
}

void FaceGenOverflowWarning(__int64 Texture)
{
	const char *texName = ((const char *(__fastcall *)(__int64))(g_ModuleBase + 0x14BE2E0))(*(__int64 *)Texture);

	EditorUI_Warning(23, "Exceeded limit of 16 tint masks. Skipping texture: %s", texName);
}

void ExportFaceGenForSelectedNPCs(__int64 a1, __int64 a2)
{
	AutoFunc(bool(*)(), sub_1418F5210, 0x18F5210);
	AutoFunc(void(*)(), sub_1418F5320, 0x18F5320);
	AutoFunc(__int64(*)(HWND, __int64), sub_1413BAAC0, 0x13BAAC0);
	AutoFunc(bool(*)(__int64, __int64), sub_1418F5260, 0x18F5260);
	AutoFunc(void(*)(__int64), sub_141617680, 0x1617680);

	// Display confirmation message box first
	if (!sub_1418F5210())
		return;

	// Nop the call to reload the loose file tables
	XUtil::PatchMemory(g_ModuleBase + 0x18F4D4A, (PBYTE)"\x90\x90\x90\x90\x90", 5);

	HWND listHandle = *(HWND *)(a1 + 16);
	int itemIndex = ListView_GetNextItem(listHandle, -1, LVNI_SELECTED);
	int itemCount = 0;

	for (bool flag = true; itemIndex >= 0 && flag; itemCount++)
	{
		flag = sub_1418F5260(a2, sub_1413BAAC0(listHandle, itemIndex));

		if (flag)
		{
			int oldIndex = itemIndex;
			itemIndex = ListView_GetNextItem(listHandle, itemIndex, LVNI_SELECTED);

			if (itemIndex == oldIndex)
				itemIndex = -1;
		}
	}

	// Reload loose file paths manually
	EditorUI_Log("Exported FaceGen for %d NPCs. Reloading loose file paths...", itemCount);
	sub_141617680(*(__int64 *)(g_ModuleBase + 0x3AFB930));

	// Done => unpatch it
	XUtil::PatchMemory(g_ModuleBase + 0x18F4D4A, (PBYTE)"\xE8\x98\xDA\x6F\xFF", 5);
	sub_1418F5320();
}

void hk_call_141C68FA6(class TESForm *DialogForm, __int64 Unused)
{
	auto *waterRoot = TESWaterRoot::Singleton();

	for (uint32_t i = 0; i < waterRoot->m_WaterObjects.QSize(); i++)
	{
		uint32_t currentFormId = *(uint32_t *)((uintptr_t)DialogForm + 0x14);
		uint32_t baseFormId = *(uint32_t *)((uintptr_t)waterRoot->m_WaterObjects[i]->m_BaseWaterForm + 0x14);

		if (currentFormId == baseFormId)
			((void(__fastcall *)(class TESForm *, class BSShaderMaterial *))(g_ModuleBase + 0x1C62AA0))(DialogForm, waterRoot->m_WaterObjects[i]->m_TriShape->QShaderProperty()->pMaterial);
	}
}

void *hk_call_141C26F3A(void *a1)
{
	if (!a1)
		return nullptr;

	return ((void *(__fastcall *)(void *))(g_ModuleBase + 0x1BA5C60))(a1);
}

uint32_t hk_sub_140FEC464(__int64 a1, uint32_t a2, bool a3)
{
	// a1 is NiPick::Results (NiTPrimitiveArray<NiPick::Record>)
	for (uint32_t i = a2; i < *(uint32_t *)(a1 + 0x18); i++)
	{
		uint32_t recordIndex = ((uint32_t(__fastcall *)(__int64, uint32_t, bool))(g_ModuleBase + 0x12E4740))(a1, i, a3);

		if (recordIndex == 0xFFFFFFFF)
			continue;

		__int64 record = ((__int64(__fastcall *)(__int64, uint32_t))(g_ModuleBase + 0x12E21E0))(a1, recordIndex);
		NiObject *object = *(NiObject **)record;

		// If possible, avoiding picking any object of type BSDynamicTriShape
		if (object->IsExactKindOf(NiRTTI::ms_BSDynamicTriShape))
			continue;

		return recordIndex;
	}

	// Fallback if BSDynamicTriShape can't be avoided
	return ((uint32_t(__fastcall *)(__int64, uint32_t, bool))(g_ModuleBase + 0x12E4740))(a1, a2, a3);
}

void hk_sub_141032ED7(__int64 a1, __int64 a2, __int64 a3)
{
	// Draw objects in the render window normally
	((void(__fastcall *)(__int64, __int64, __int64))(g_ModuleBase + 0x2DAAC80))(a1, a2, a3);

	// Then do post-process SAO (Fog) ("Draw WorldRoot")
	AutoPtr(bool, byte_144F05728, 0x4F05728);
	AutoPtr(uintptr_t, qword_145A11B28, 0x5A11B28);
	AutoPtr(uintptr_t, qword_145A11B38, 0x5A11B38);

	if (byte_144F05728)
	{
		if (!qword_145A11B28)
			qword_145A11B28 = (uintptr_t)MemoryManager::Alloc(nullptr, 4096, 8, true);// Fake BSFadeNode

		if (!qword_145A11B38)
			qword_145A11B38 = (uintptr_t)MemoryManager::Alloc(nullptr, 4096, 8, true);// Fake SceneGraph

		((void(__fastcall *)())(g_ModuleBase + 0x2E2EEB0))();
	}
}

void *hk_call_1417E42BF(void *a1)
{
	// Patch flowchartx64.dll every time - it's a COM dll and I have no idea if it gets reloaded
	uintptr_t flowchartBase = (uintptr_t)GetModuleHandleA("flowchartx64.dll");

	if (flowchartBase)
	{
		AssertMsg(*(uint8_t *)(flowchartBase + 0x5FF89) == 0x48 && *(uint8_t *)(flowchartBase + 0x5FF8A) == 0x8B, "Unknown FlowChartX64.dll version");

		// Prevent the XML element <Tag Type="14">16745094784</Tag> from being written
		XUtil::PatchMemoryNop(flowchartBase + 0x5FF9A, 60);
	}

	// Return null so the branch directly after is never taken
	return nullptr;
}

HRESULT LoadTextureDataFromFile(__int64 a1, __int64 a2, __int64 a3, __int64 a4, unsigned int a5, int a6)
{
	// Modified LoadTextureDataFromFile from DDSTextureLoader (DirectXTex)
	HRESULT hr = ((HRESULT(__fastcall *)(__int64, __int64, __int64, __int64, unsigned int, int))(g_ModuleBase + 0x2D266E0))(a1, a2, a3, a4, a5, a6);

	if (FAILED(hr))
	{
		// NiBinaryStream->BSResourceStream->File name
		const char *fileName = *(const char **)(*(__int64 *)(a2 + 0x20) + 0x20);

		AssertMsgVa(SUCCEEDED(hr),
			"Fatal error while trying to load texture \"%s\" due to an incompatible file format. This "
			"indicates a problem with your mod or game files. Note that B5G6R5 and B5G5R5A1 texture "
			"formats are not supported on Windows 7. HR = 0x%08X.",
			fileName, hr);
	}

	// This return value is ignored. If it fails it returns a null pointer (a3) and crashes later on.
	return hr;
}

void hk_call_141C410A1(__int64 a1, BSShaderProperty *Property)
{
	if (Property)
	{
		Property->ulFlags |= (1ull << BSShaderProperty::BSSP_FLAG_TWO_SIDED);		// Sphere is only 1 sided
		Property->ulFlags &= ~(1ull << BSShaderProperty::BSSP_FLAG_ZBUFFER_WRITE);	// Transparency is used

		// Fix material alpha. A copy must be made because it uses a global pointer by default.
		auto oldShaderMaterial = static_cast<BSEffectShaderMaterial *>(Property->pMaterial);
		auto newShaderMaterial = static_cast<BSEffectShaderMaterial *>(oldShaderMaterial->CreateNew());

		newShaderMaterial->CopyMembers(oldShaderMaterial);
		newShaderMaterial->kBaseColor.a = 0.5f;

		((void(__fastcall *)(BSShaderProperty *, BSEffectShaderMaterial *, bool))(g_ModuleBase + 0x2D511E0))(Property, newShaderMaterial, false);
	}

	((void(__fastcall *)(__int64, BSShaderProperty *))(g_ModuleBase + 0x12A4D20))(a1 + 0x128, Property);
}

void hk_sub_141B08540(__int64 DiskCRDT, __int64 SourceCRDT)
{
	// Convert in-memory CRDT to on-disk CRDT data, but do it properly this time
	memset((void *)DiskCRDT, 0, 24);

	*(uint16_t *)DiskCRDT = *(uint16_t *)(SourceCRDT + 0x10);		// Damage
	*(uint32_t *)(DiskCRDT + 0x4) = *(uint32_t *)SourceCRDT;		// Percentage multiplier
	*(uint8_t *)(DiskCRDT + 0x8) = *(uint8_t *)(SourceCRDT + 0x12);	// Flags

	uintptr_t effectForm = *(uintptr_t *)(SourceCRDT + 0x8);

	if (effectForm)
		*(uint64_t *)(DiskCRDT + 0x10) = *(uint32_t *)(effectForm + 0x14);// Effect form id
}

void hk_call_141B037B2(__int64 TESFile, __int64 SourceCRDT)
{
	((bool(__fastcall *)(__int64, __int64))(g_ModuleBase + 0x1B07AA0))(TESFile, SourceCRDT);

	// Apply struct fixup after reading SkyrimLE data
	uint32_t chunkVersion = ((uint32_t(__fastcall *)(__int64))(g_ModuleBase + 0x1222200))(TESFile);

	if (chunkVersion < 44)
		*(uint32_t *)(SourceCRDT + 0x10) = *(uint32_t *)(SourceCRDT + 0xC);
}

const char *hk_call_1417F4A04(int ActorValueIndex)
{
	__int64 actorValue = ((__int64(__fastcall *)(int))(g_ModuleBase + 0x14B8030))(ActorValueIndex);

	if (!actorValue)
		return nullptr;

	return *(const char **)(actorValue + 0x90);
}

thread_local WIN32_FIND_DATAA CachedFileInfo;
thread_local char CachedBasePath[MAX_PATH];
thread_local bool CachedFileInfoValid;

uint32_t sub_142647AC0(__int64 a1, bool *IterationFinished)
{
	HANDLE& findHandle = *(HANDLE *)a1;
	LPWIN32_FIND_DATAA findData = (LPWIN32_FIND_DATAA)(a1 + 8);
	uint32_t& status = *(uint32_t *)(a1 + 0x24C);

	CachedFileInfoValid = true;

	if (findHandle == INVALID_HANDLE_VALUE)
	{
		// Attempting to iterate directory on an already invalid handle
		status = 6;
	}
	else if (findHandle)
	{
		*IterationFinished = FindNextFileA(findHandle, findData) == FALSE;
		memcpy(&CachedFileInfo, findData, sizeof(WIN32_FIND_DATAA));

		if (*IterationFinished)
			CachedFileInfoValid = false;
	}
	else
	{
		findHandle = FindFirstFileExA((LPCSTR)(a1 + 0x148), FindExInfoStandard, findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
		memcpy(&CachedFileInfo, findData, sizeof(WIN32_FIND_DATAA));

		strcpy_s(CachedBasePath, (LPCSTR)(a1 + 0x148));
		*strrchr(CachedBasePath, '\\') = '\0';

		// status = x ? EC_INVALID_PARAM : EC_NONE;
		status = (findHandle == INVALID_HANDLE_VALUE) ? 6 : 0;
	}

	if (status != 0)
		CachedFileInfoValid = false;

	return status;
}

bool sub_142676020(const char *File, uint32_t *FileSize)
{
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	fileInfo.dwFileAttributes = INVALID_FILE_ATTRIBUTES;

	if (CachedFileInfoValid)
	{
		const static uint32_t cwdLength = GetCurrentDirectoryA(0, nullptr);
		const size_t filePathLen = strlen(File);
		const size_t basePathLen = strlen(CachedBasePath);

		if (filePathLen > cwdLength)
		{
			// Anything under "<CWD>\\Data\\data\\" will never be valid, so discard all calls with it
			if (!_strnicmp(File + cwdLength, "Data\\data\\", 10))
				return false;

			// Check if this file path matches the query from FindFirstFileExA
			if (filePathLen > basePathLen && !_stricmp(File + basePathLen, CachedFileInfo.cFileName))
			{
				fileInfo.dwFileAttributes = CachedFileInfo.dwFileAttributes;
				fileInfo.nFileSizeLow = CachedFileInfo.nFileSizeLow;
				fileInfo.nFileSizeHigh = CachedFileInfo.nFileSizeHigh;
			}
		}
	}

	if (fileInfo.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		// Cache miss
		if (!GetFileAttributesExA(File, GetFileExInfoStandard, &fileInfo))
			return false;
	}

	if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return false;

	AssertMsgVa(fileInfo.nFileSizeHigh <= 0, "Need to move to 64-bit file sizes. '%s' exceeds 4GB.", File);

	*FileSize = fileInfo.nFileSizeLow;
	return true;
}

void hk_call_1412DD706(HWND WindowHandle, uint32_t *ControlId)
{
	__int64 previewControl = ((__int64(__fastcall *)(HWND, uint32_t *))(g_ModuleBase + 0x1486C50))(WindowHandle, ControlId);

	if (previewControl)
		((void(__fastcall *)(__int64))(g_ModuleBase + 0x14AD7F0))(previewControl);
}

int sub_141BBF320(__int64 a1, __int64 a2)
{
	// Create a copy of the cell's persistent ref hashmap and increase the ref count for all elements
	std::vector<__int64> temporaryCellRefList;
	{
		struct
		{
			uintptr_t unk1;
			uintptr_t unk2;
			uint32_t unk3;
		} currIter, endIter;

		((void(__fastcall *)(__int64, void *))(g_ModuleBase + 0x12D4700))(a1, &currIter);
		((void(__fastcall *)(__int64, void *))(g_ModuleBase + 0x12D4FD0))(a1, &endIter);

		while (((bool(__fastcall *)(void *, void *))(g_ModuleBase + 0x12D32B0))(&currIter, &endIter))
		{
			// Increase refcount via BSHandleRefObject::IncRefCount
			__int64 refr;

			((__int64(__fastcall *)(__int64 *, __int64))(g_ModuleBase + 0x1348900))(&refr, currIter.unk1);
			temporaryCellRefList.push_back(refr);

			// Move to next element
			((void(__fastcall *)(void *))(g_ModuleBase + 0x12D3AC0))(&currIter);
		}
	}

	// Now parse the entire list separately - allow InitItem() to modify the cell's hashmap without invalidating any iterators
	int status = 1;

	for (__int64 refr : temporaryCellRefList)
	{
		if (status != 1)
			break;

		// Automatically decrements ref count
		status = ((int(__fastcall *)(__int64, __int64 *))(g_ModuleBase + 0x1BC8B00))(*(__int64 *)a2, &refr);
	}

	return status;
}

thread_local __int64 bullshit;

__int64 hk_call_141B7FFB4(__int64 a1)
{
	bullshit = *(__int64 *)a1;
	return *(__int64 *)a1;
}

void *hk_call_141B816A7()
{
	// IRendererResourceManager::CreateTrishape
	void *rendererData = ((void *(__fastcall *)(__int64, __int64, uint32_t, __int64, __int64))(g_ModuleBase + 0x2D0AA90))(g_ModuleBase + 0x56B73A0, bullshit, 11560, 0xBB0080765040A, *(__int64 *)(g_ModuleBase + 0x4F00CB0));

	//
	// Construct a normal BSTriShape rather than a dynamic one. Also force a crash if any of the uninitialized
	// pointers are accessed.
	//
	// sizeof(BSTriShape) = 0x160, sizeof(BSDynamicTriShape) = 0x180
	//
	uintptr_t shape = ((uintptr_t(__fastcall *)(size_t))(g_ModuleBase + 0x1219450))(0x180);
	((void *(__fastcall *)(uintptr_t, void *, __int64, __int16, __int16))(g_ModuleBase + 0x268D480))(shape, rendererData, 0xBB0080765040A, 289, 512);

	*(uintptr_t *)(shape + 0x160) = 0xFEFEDEADC0DEBADD;

	return (void *)shape;
}