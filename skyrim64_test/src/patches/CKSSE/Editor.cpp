#include "../../common.h"
#include <libdeflate/libdeflate.h>
#include <mutex>
#include <smmintrin.h>
#include <CommCtrl.h>
#include <Shlobj.h>
#include "../TES/MemoryManager.h"
#include "../TES/NiMain/NiColor.h"
#include "../TES/NiMain/NiPointer.h"
#include "../TES/NiMain/NiSourceTexture.h"
#include "../TES/NiMain/BSTriShape.h"
#include "../TES/BSShader/BSShaderProperty.h"
#include "../TES/BSShader/Shaders/BSEffectShaderMaterial.h"
#include "Editor.h"
#include "EditorUI.h"
#include "EditorUIDarkMode.h"
#include "TESWater.h"
#include "LogWindow.h"

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

	// Override certain default dialogs to use this DLL's resources
	switch ((uintptr_t)lpTemplateName)
	{
	case 0x7A:// "Object Window"
	case 0x8D:// "Reference"
	case 0xA2:// "Data"
	case 0xAF:// "Cell View"
	case 0xDC:// "Use Report"
		hInstance = (HINSTANCE)&__ImageBase;
		break;
	}

	return CreateDialogParamA(hInstance, lpTemplateName, hWndParent, DialogFuncOverride, dwInitParam);
}

INT_PTR WINAPI hk_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	// EndDialog MUST be used
	DlgData.DialogFunc = lpDialogFunc;
	DlgData.IsDialog = true;

	// Override certain default dialogs to use this DLL's resources
	switch ((uintptr_t)lpTemplateName)
	{
	case 0x7A:// "Object Window"
	case 0x8D:// "Reference"
	case 0xA2:// "Data"
	case 0xAF:// "Cell View"
	case 0xDC:// "Use Report"
		hInstance = (HINSTANCE)&__ImageBase;
		break;
	}

	return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, DialogFuncOverride, dwInitParam);
}

BOOL WINAPI hk_EndDialog(HWND hDlg, INT_PTR nResult)
{
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
		OFFSET(0x14824B0, 1530))(ParentWindow, BasePath, filter, title, extension, nullptr, false, true, Buffer, BufferSize, Directory, nullptr);
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

	auto procInfo = (PROCESS_INFORMATION *)(a1 + 0x228);
	memset(procInfo, 0, sizeof(PROCESS_INFORMATION));

	STARTUPINFOA startupInfo
	{
		.cb = sizeof(STARTUPINFOA)
	};

	if (LogWindow::GetStdoutListenerPipe())
	{
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdError = LogWindow::GetStdoutListenerPipe();
		startupInfo.hStdOutput = LogWindow::GetStdoutListenerPipe();
		startupInfo.hStdInput = nullptr;
	}

	if (!CreateProcessA(procToolPath, nullptr, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startupInfo, procInfo))
		LogWindow::Log("FaceFXWrapper could not be started (%d). LIP generation will be disabled.\n", GetLastError());
	else
		LogWindow::Log("FaceFXWrapper background process started.\n");
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

	LogWindow::LogWarning(6, "Moving temporary LIP file to '%s' (%s)", destDir, status ? "Succeeded" : "Failed");
	return status;
}

int IsWavDataPresent(const char *Path, __int64 a2, __int64 a3, __int64 a4)
{
	return ((int(__fastcall *)(const char *, __int64, __int64, __int64))OFFSET(0x264D120, 1530))("Sound\\Voice\\Temp.wav", a2, a3, a4);
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

uint32_t BSGameDataSystemUtility__GetCCFileCount()
{
	ParseCreationClubContentFile();

	return (uint32_t)g_CCEslNames.size();
}

const char *BSGameDataSystemUtility__GetCCFile(uint32_t Index)
{
	ParseCreationClubContentFile();

	if (Index < g_CCEslNames.size())
		return g_CCEslNames[Index].c_str();

	return nullptr;
}

bool BSGameDataSystemUtility__IsCCFile(const char *Name)
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
	uint32_t index = StartIndex;
	__int64 *data = (__int64 *)Array.QBuffer();

	const uint32_t comparesPerIter = 4;
	const uint32_t vectorizedIterations = (Array.QSize() - index) / comparesPerIter;

	//
	// Compare 4 pointers per iteration - use SIMD instructions to generate a bit mask. Set
	// bit 0 if 'array[i + 0]'=='target', set bit 1 if 'array[i + 1]'=='target', set bit X...
	//
	// AVX: mask = _mm256_movemask_pd(_mm256_castsi256_pd(_mm256_cmpeq_epi64(targets, _mm256_loadu_si256((__m256i *)&data[i]))));
	//
	const __m128i targets = _mm_set1_epi64x((__int64)Target);

	for (uint32_t iter = 0; iter < vectorizedIterations; iter++)
	{
		__m128i test1 = _mm_cmpeq_epi64(targets, _mm_loadu_si128((__m128i *)&data[index + 0]));
		__m128i test2 = _mm_cmpeq_epi64(targets, _mm_loadu_si128((__m128i *)&data[index + 2]));

		int mask = _mm_movemask_pd(_mm_castsi128_pd(_mm_or_si128(test1, test2)));

		// if (target pointer found) { break into the remainder loop to get the index }
		if (mask != 0)
			break;

		index += comparesPerIter;
	}

	// Scan the rest 1-by-1
	for (; index < Array.QSize(); index++)
	{
		if (data[index] == (__int64)Target)
			return index;
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
	float newPercent = ((float)*(uint32_t *)OFFSET(0x3BBDA8C, 1530) / (float)*(uint32_t *)OFFSET(0x3BBDA88, 1530)) * 100.0f;

	if (abs(lastPercent - newPercent) <= 0.25f)
		return;

	((void(__fastcall *)())OFFSET(0x13A4640, 1530))();
	lastPercent = newPercent;
}

void UpdateObjectWindowTreeView(void *Thisptr, HWND ControlHandle)
{
	SendMessage(ControlHandle, WM_SETREDRAW, FALSE, 0);
	((void(__fastcall *)(void *, HWND))OFFSET(0x12D8710, 1530))(Thisptr, ControlHandle);
	SendMessage(ControlHandle, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(ControlHandle, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
}

void UpdateCellViewCellList(void *Thisptr, HWND ControlHandle, __int64 Unknown)
{
	SendMessage(ControlHandle, WM_SETREDRAW, FALSE, 0);
	((void(__fastcall *)(void *, HWND, __int64))OFFSET(0x147FA70, 1530))(Thisptr, ControlHandle, Unknown);
	SendMessage(ControlHandle, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(ControlHandle, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_NOCHILDREN);
}

void UpdateCellViewObjectList(void *Thisptr, HWND *ControlHandle)
{
	SendMessage(*ControlHandle, WM_SETREDRAW, FALSE, 0);
	((void(__fastcall *)(void *, HWND *))OFFSET(0x13E0CE0, 1530))(Thisptr, ControlHandle);
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
			std::array<int, UCHAR_MAX + 1> fontWidths;
			std::array<ABC, UCHAR_MAX + 1> trueTypeFontWidths;

			if (!GetCharABCWidthsA(hdc, 0, trueTypeFontWidths.size() - 1, trueTypeFontWidths.data()))
			{
				BOOL result = GetCharWidthA(hdc, 0, fontWidths.size() - 1, fontWidths.data());
				AssertMsg(result, "Failed to determine any font widths");
			}
			else
			{
				for (int i = 0; i < fontWidths.size(); i++)
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

				finalWidth = std::max(finalWidth, lineSize);

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

	LVITEMA item
	{
		.mask = LVIF_PARAM | LVIF_TEXT,
		.iItem = ItemIndex,
		.pszText = LPSTR_TEXTCALLBACK,
		.lParam = (LPARAM)Parameter
	};

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

void SortDialogueInfo(__int64 TESDataHandler, uint32_t FormType, int(*SortFunction)(const void *, const void *))
{
	static std::unordered_map<BSTArray<TESForm_CK *> *, std::pair<void *, uint32_t>> arrayCache;

	auto formArray = &((BSTArray<TESForm_CK *> *)(TESDataHandler + 104))[FormType];
	auto itr = arrayCache.find(formArray);

	// If not previously found or any counters changed...
	if (itr == arrayCache.end() || itr->second.first != formArray->QBuffer() || itr->second.second != formArray->QSize())
	{
		// Update and resort the array
		ArrayQuickSortRecursive(*formArray, SortFunction);

		arrayCache[formArray] = std::make_pair(formArray->QBuffer(), formArray->QSize());
	}
}

void QuitHandler()
{
	TerminateProcess(GetCurrentProcess(), 0);
}

void hk_sub_141047AB2(__int64 FileHandle, __int64 *Value)
{
	// The pointer is converted to a form id, but the top 32 bits are never cleared correctly (ui32/ui64 union)
	*Value &= 0x00000000FFFFFFFF;

	((void(__fastcall *)(__int64, __int64 *))OFFSET(0x15C88D0, 1530))(FileHandle, Value);
}

void hk_call_14158589F(__int64 Buffer, __int64 *Value)
{
	// The Value pointer can't be modified since it's used immediately after. Garbage data is still written to the plugin.
	__int64 newValue = *Value & 0x00000000FFFFFFFF;

	((void(__fastcall *)(__int64, __int64 *))OFFSET(0x158D2F0, 1530))(Buffer, &newValue);
}

bool InitItemPerkRankDataVisitor(PerkRankEntry *Entry, uint32_t *FormId, __int64 UnknownArray)
{
	auto sub_1416B8A20 = (__int64(*)(__int64, __int64))OFFSET(0x16B8A20, 1530);
	auto sub_1416C05D0 = (__int64(*)(uint32_t *, __int64))OFFSET(0x16C05D0, 1530);
	auto sub_14168E790 = (__int64(*)(void *, uint32_t *))OFFSET(0x168E790, 1530);

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

void PerkRankData__LoadFrom(__int64 ArrayHandle, PerkRankEntry *&Entry)
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
		((void(__fastcall *)(__int64, PerkRankEntry *&))OFFSET(0x168EAE0, 1530))(ArrayHandle, Entry);
	else
		LogWindow::LogWarning(13, "Null perk found while loading a PerkRankArray. Entry will be discarded.");
}

void FaceGenOverflowWarning(__int64 Texture)
{
	const char *texName = ((const char *(__fastcall *)(__int64))OFFSET(0x14BE2E0, 1530))(*(__int64 *)Texture);

	LogWindow::LogWarning(23, "Exceeded limit of 16 tint masks. Skipping texture: %s", texName);
}

void ExportFaceGenForSelectedNPCs(__int64 a1, __int64 a2)
{
	auto sub_1418F5210 = (bool(*)())OFFSET(0x18F5210, 1530);
	auto sub_1418F5320 = (void(*)())OFFSET(0x18F5320, 1530);
	auto sub_1413BAAC0 = (__int64(*)(HWND, __int64))OFFSET(0x13BAAC0, 1530);
	auto sub_1418F5260 = (bool(*)(__int64, __int64))OFFSET(0x18F5260, 1530);
	auto sub_141617680 = (void(*)(__int64))OFFSET(0x1617680, 1530);

	// Display confirmation message box first
	if (!sub_1418F5210())
		return;

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

	// Reload loose file paths manually since it's patched out
	LogWindow::Log("Exported FaceGen for %d NPCs. Reloading loose file paths...", itemCount);
	sub_141617680(*(__int64 *)OFFSET(0x3AFB930, 1530));

	sub_1418F5320();
}

void hk_call_141C68FA6(TESForm_CK *DialogForm, __int64 Unused)
{
	auto waterRoot = TESWaterRoot::Singleton();

	for (uint32_t i = 0; i < waterRoot->m_WaterObjects.QSize(); i++)
	{
		if (DialogForm->GetFormID() == waterRoot->m_WaterObjects[i]->m_BaseWaterForm->GetFormID())
			((void(__fastcall *)(TESForm_CK *, class BSShaderMaterial *))OFFSET(0x1C62AA0, 1530))(DialogForm, waterRoot->m_WaterObjects[i]->m_TriShape->QShaderProperty()->pMaterial);
	}
}

void *hk_call_141C26F3A(void *a1)
{
	if (!a1)
		return nullptr;

	return ((void *(__fastcall *)(void *))OFFSET(0x1BA5C60, 1530))(a1);
}

void hk_sub_141032ED7(__int64 a1, __int64 a2, __int64 a3)
{
	// Draw objects in the render window normally
	((void(__fastcall *)(__int64, __int64, __int64))OFFSET(0x2DAAC80, 1530))(a1, a2, a3);

	// Then do post-process SAO (Fog) ("Draw WorldRoot")
	auto& byte_144F05728 = *(bool *)OFFSET(0x4F05728, 1530);
	auto& qword_145A11B28 = *(uintptr_t *)OFFSET(0x5A11B28, 1530);
	auto& qword_145A11B38 = *(uintptr_t *)OFFSET(0x5A11B38, 1530);

	if (byte_144F05728)
	{
		if (!qword_145A11B28)
			qword_145A11B28 = (uintptr_t)MemoryManager::Allocate(nullptr, 4096, 8, true);// Fake BSFadeNode

		if (!qword_145A11B38)
			qword_145A11B38 = (uintptr_t)MemoryManager::Allocate(nullptr, 4096, 8, true);// Fake SceneGraph

		((void(__fastcall *)())OFFSET(0x2E2EEB0, 1530))();
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

HRESULT DirectX__LoadFromDDSFile(__int64 a1, __int64 a2, __int64 a3, __int64 a4, unsigned int a5, int a6)
{
	// Modified DirectX::LoadFromDDSFile from DDSTextureLoader (DirectXTex)
	HRESULT hr = ((HRESULT(__fastcall *)(__int64, __int64, __int64, __int64, unsigned int, int))OFFSET(0x2D266E0, 1530))(a1, a2, a3, a4, a5, a6);

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

		((void(__fastcall *)(BSShaderProperty *, BSEffectShaderMaterial *, bool))OFFSET(0x2D511E0, 1530))(Property, newShaderMaterial, false);
	}

	((void(__fastcall *)(__int64, BSShaderProperty *))OFFSET(0x12A4D20, 1530))(a1 + 0x128, Property);
}

void TESObjectWEAP__Data__ConvertCriticalData(__int64 DiskCRDT, __int64 SourceCRDT)
{
	// Convert in-memory CRDT to on-disk CRDT data, but do it properly this time
	memset((void *)DiskCRDT, 0, 24);

	*(uint16_t *)DiskCRDT = *(uint16_t *)(SourceCRDT + 0x10);		// Damage
	*(uint32_t *)(DiskCRDT + 0x4) = *(uint32_t *)SourceCRDT;		// Percentage multiplier
	*(uint8_t *)(DiskCRDT + 0x8) = *(uint8_t *)(SourceCRDT + 0x12);	// Flags

	TESForm_CK *effectForm = *(TESForm_CK **)(SourceCRDT + 0x8);

	if (effectForm)
		*(uint64_t *)(DiskCRDT + 0x10) = effectForm->GetFormID();
}

void TESObjectWEAP__Data__LoadCriticalData(__int64 TESFile, __int64 SourceCRDT)
{
	((bool(__fastcall *)(__int64, __int64))OFFSET(0x1B07AA0, 1530))(TESFile, SourceCRDT);

	// Apply struct fixup after reading SkyrimLE data
	uint32_t chunkVersion = ((uint32_t(__fastcall *)(__int64))OFFSET(0x1222200, 1530))(TESFile);

	if (chunkVersion < 44)
		*(uint32_t *)(SourceCRDT + 0x10) = *(uint32_t *)(SourceCRDT + 0xC);
}

const char *hk_call_1417F4A04(int ActorValueIndex)
{
	__int64 actorValue = ((__int64(__fastcall *)(int))OFFSET(0x14B8030, 1530))(ActorValueIndex);

	if (!actorValue)
		return nullptr;

	return *(const char **)(actorValue + 0x90);
}

thread_local WIN32_FIND_DATAA CachedFileInfo;
thread_local char CachedBasePath[MAX_PATH];
thread_local bool CachedFileInfoValid;

uint32_t BSSystemDir__NextEntry(__int64 a1, bool *IsComplete)
{
	auto& findHandle = *(HANDLE *)a1;
	auto findData = (LPWIN32_FIND_DATAA)(a1 + 8);
	auto& status = *(uint32_t *)(a1 + 0x24C);

	CachedFileInfoValid = true;

	if (findHandle == INVALID_HANDLE_VALUE)
	{
		// Attempting to iterate directory on an already invalid handle
		status = 6;
	}
	else if (findHandle)
	{
		*IsComplete = FindNextFileA(findHandle, findData) == FALSE;
		memcpy(&CachedFileInfo, findData, sizeof(WIN32_FIND_DATAA));

		if (*IsComplete)
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

bool BSResource__LooseFileLocation__FileExists(const char *CanonicalFullPath, uint32_t *TotalSize)
{
	WIN32_FILE_ATTRIBUTE_DATA fileInfo
	{
		.dwFileAttributes = INVALID_FILE_ATTRIBUTES
	};

	if (CachedFileInfoValid)
	{
		const static uint32_t cwdLength = GetCurrentDirectoryA(0, nullptr);
		const size_t filePathLen = strlen(CanonicalFullPath);
		const size_t basePathLen = strlen(CachedBasePath);

		if (filePathLen > cwdLength)
		{
			// Anything under "<CWD>\\Data\\data\\" will never be valid, so discard all calls with it
			if (!_strnicmp(CanonicalFullPath + cwdLength, "Data\\data\\", 10))
				return false;

			// Check if this file path matches the query from FindFirstFileExA
			if (filePathLen > basePathLen && !_stricmp(CanonicalFullPath + basePathLen, CachedFileInfo.cFileName))
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
		if (!GetFileAttributesExA(CanonicalFullPath, GetFileExInfoStandard, &fileInfo))
			return false;
	}

	if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return false;

	// The game doesn't support files larger than 4GB
	if (fileInfo.nFileSizeHigh > 0)
	{
		LogWindow::LogWarning(0, "Need to move to 64-bit file sizes. '%s' exceeds 4GB.", CanonicalFullPath);
		return false;
	}

	*TotalSize = fileInfo.nFileSizeLow;
	return true;
}

void hk_call_1412DD706(HWND WindowHandle, uint32_t *ControlId)
{
	__int64 previewControl = ((__int64(__fastcall *)(HWND, uint32_t *))OFFSET(0x1486C50, 1530))(WindowHandle, ControlId);

	if (previewControl)
		((void(__fastcall *)(__int64))OFFSET(0x14AD7F0, 1530))(previewControl);
}

std::vector<TESObjectREFR_CK *> CreateCellPersistentMapCopy(__int64 List)
{
	// Create a copy of the cell's persistent ref hashmap and increase the ref count for all elements
	std::vector<TESObjectREFR_CK *> temporaryCellRefList;

	struct
	{
		uintptr_t unk1;
		uintptr_t unk2;
		uint32_t unk3;
	} currIter, endIter;

	((void(__fastcall *)(__int64, void *))OFFSET(0x12D4700, 1530))(List, &currIter);
	((void(__fastcall *)(__int64, void *))OFFSET(0x12D4FD0, 1530))(List, &endIter);

	while (((bool(__fastcall *)(void *, void *))OFFSET(0x12D32B0, 1530))(&currIter, &endIter))
	{
		// Increase refcount via BSHandleRefObject::IncRefCount
		TESObjectREFR_CK *refr;

		((__int64(__fastcall *)(TESObjectREFR_CK **, __int64))OFFSET(0x1348900, 1530))(&refr, currIter.unk1);
		temporaryCellRefList.push_back(refr);

		// Move to next element
		((void(__fastcall *)(void *))OFFSET(0x12D3AC0, 1530))(&currIter);
	}

	return temporaryCellRefList;
}

int sub_141BBF220(__int64 a1, __int64 a2)
{
	auto cellRefList = CreateCellPersistentMapCopy(a1);
	int status = 1;

	// Unknown init function
	for (TESObjectREFR_CK *refr : cellRefList)
	{
		if (status != 1)
			break;

		// Automatically decrements ref count
		status = ((int(__fastcall *)(__int64, TESObjectREFR_CK **))OFFSET(0x1BC88B0, 1530))(*(__int64 *)a2, &refr);
	}

	return status;
}

int sub_141BBF320(__int64 a1, __int64 a2)
{
	auto cellRefList = CreateCellPersistentMapCopy(a1);
	int status = 1;

	// Now parse the entire list separately - allow InitItem() to modify the cell's hashmap without invalidating any iterators
	for (TESObjectREFR_CK *refr : cellRefList)
	{
		if (status != 1)
			break;

		// Automatically decrements ref count
		status = ((int(__fastcall *)(__int64, TESObjectREFR_CK **))OFFSET(0x1BC8B00, 1530))(*(__int64 *)a2, &refr);
	}

	return status;
}

void hk_call_141CF03C9(__int64 a1, bool Enable)
{
	// Modify the global setting itself then update UI to match
	((void(__fastcall *)(__int64, bool))OFFSET(0x1390C30, 1530))(a1, Enable);

	CheckMenuItem(GetMenu(EditorUI::GetWindow()), UI_EDITOR_TOGGLEFOG, Enable ? MF_CHECKED : MF_UNCHECKED);
}

void hk_call_141CE8269(__int64 a1)
{
	if (*(__int64 *)(a1 + 0x58))
		((void(__fastcall *)(__int64))OFFSET(0x1CEB510, 1530))(a1);
}

const char *hk_call_1416B849E(__int64 a1)
{
	const char *formEDID = (*(const char *(__fastcall **)(__int64))(*(__int64 *)a1 + 0x1E8))(a1);

	if (!formEDID || strlen(formEDID) <= 0)
		return "UNNAMED_FORM";

	return formEDID;
}

void hk_call_14135CDD3(__int64 RenderWindowInstance, uint32_t *UntypedPointerHandle, bool Select)
{
	// The caller of this function already holds a reference to the pointer
	__int64 parentRefr = ((__int64(__fastcall *)(__int64))OFFSET(0x1C0D8F0, 1530))(*(__int64 *)(RenderWindowInstance + 0xB8));

	__int64 childRefr;
	((void(__fastcall *)(__int64 *, uint32_t *))OFFSET(0x12E0DF0, 1530))(&childRefr, UntypedPointerHandle);

	if (childRefr)
	{
		// Only select child forms if they are in the same parent cell
		if (*(__int64 *)(childRefr + 0x70) == *(__int64 *)(parentRefr + 0x70))
			((void(__fastcall *)(__int64, uint32_t *, bool))OFFSET(0x13059D0, 1530))(RenderWindowInstance, UntypedPointerHandle, Select);
		else
			LogWindow::Log("Not selecting child refr 0x%X because parent cells don't match (%p != %p)\n", *(uint32_t *)(childRefr + 0x14), *(__int64 *)(childRefr + 0x70), *(__int64 *)(parentRefr + 0x70));
	}

	((void(__fastcall *)(__int64 *))OFFSET(0x128BCF0, 1530))(&childRefr);
}

int hk_call_1412D1541(__int64 ObjectListInsertData, TESForm_CK *Form)
{
	const __int64 objectWindowInstance = *(__int64 *)(ObjectListInsertData + 0x8) - 0x28;
	const HWND objectWindowHandle = *(HWND *)(objectWindowInstance);

	bool allowInsert = true;
	SendMessageA(objectWindowHandle, UI_OBJECT_WINDOW_ADD_ITEM, (WPARAM)Form, (LPARAM)&allowInsert);

	if (!allowInsert)
		return 1;

	return ((int(__fastcall *)(__int64, TESForm_CK *))OFFSET(0x12D3BD0, 1530))(ObjectListInsertData, Form);
}

void hk_call_14147FB57(HWND ListViewHandle, TESForm_CK *Form, bool UseImage, int ItemIndex)
{
	bool allowInsert = true;
	SendMessageA(GetParent(ListViewHandle), UI_CELL_VIEW_ADD_CELL_ITEM, (WPARAM)Form, (LPARAM)&allowInsert);

	if (!allowInsert)
		return;

	((void(__fastcall *)(HWND, TESForm_CK *, bool, int))OFFSET(0x13BA4D0, 1530))(ListViewHandle, Form, UseImage, ItemIndex);
}

float hk_call_14202E0E8(float Delta)
{
	return abs(Delta) / 100.0f;
}

void BSUtilities__SetLocalAppDataPath(const char *Path)
{
	char appDataPath[MAX_PATH];
	HRESULT hr = SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath);

	AssertMsg(SUCCEEDED(hr), "Failed to get user AppData path for plugins.txt");

	sprintf_s((char *)OFFSET(0x54CD060, 1530), MAX_PATH, "%s\\%s\\", appDataPath, "Skyrim Special Edition");
}

void hk_call_14130F9E8(uintptr_t a1, bool a2)
{
	memset((void *)a1, 0, 0x1C);

	// Unknown camera distance multiplier
	*(bool *)(a1 + 0x18) = a2;
	*(float *)(a1 + 0x10) = 1000.0f;
}

void hk_call_14267B359(__int64 SourceNode, __int64 DestNode, __int64 CloningProcess)
{
	const char *sourceNodeName = *(const char **)(SourceNode + 0x10);
	__int64 sourceNodeParent = *(__int64 *)(SourceNode + 0x30);

	if (*(const char **)(SourceNode + 0x40) && !sourceNodeName && sourceNodeParent)
		LogWindow::LogWarning(10, "Cloning a child node with collision and no name present. Parent is \"%s\".", *(const char **)(sourceNodeParent + 0x10));

	((void(__fastcall *)(__int64, __int64, __int64))OFFSET(0x267B390, 1530))(SourceNode, DestNode, CloningProcess);
}

size_t hk_call_141A0808C(const char *String)
{
	return String ? strlen(String) : 0;
}

void TESClass__InitializeEditDialog(TESForm_CK *Class, HWND WindowHandle)
{
	// Hide the unused "Recharge" checkbox
	ShowWindow(GetDlgItem(WindowHandle, 1543), SW_HIDE);

	// If (max training level > 0) update "Training"
	CheckDlgButton(WindowHandle, 1542, (*(uint8_t *)((__int64)Class + 0x95) > 0) ? BST_CHECKED : BST_UNCHECKED);

	((void(__fastcall *)(TESForm_CK *, HWND))OFFSET(0x18AE640, 1530))(Class, WindowHandle);
}

void hk_call_142D12196()
{
	AssertMsg(false, "Creation Kit renderer initialization failed because your graphics card doesn't support D3D11 Feature Level 11 (FL11_0). Updating your drivers may fix this.");
}

void hk_sub_141481390(HWND ControlHandle)
{
	auto sub_141481030 = (void(*)(HWND, uint8_t, bool, __int64, __int64, __int64, __int64, __int64))OFFSET(0x1481030, 1530);

	sub_141481030(ControlHandle, 18, true, 0, 0, 0, 0, 0);	// MGEF Magic Effect
	sub_141481030(ControlHandle, 21, false, 0, 0, 0, 0, 0);	// ENCH Enchantment
	sub_141481030(ControlHandle, 118, false, 0, 0, 0, 0, 0);// WOOP Word of Power
}

void hk_call_141434458(__int64 a1, uint32_t Color)
{
	// Blue and green channels are swapped
	uint32_t newColor = Color & 0xFF;
	newColor |= ((Color >> 16) & 0xFF) << 8;
	newColor |= ((Color >> 8) & 0xFF) << 16;

	((void(__fastcall *)(__int64, uint32_t))OFFSET(0x1478E60, 1530))(a1, newColor);
}

BOOL hk_call_1420AD5C9(HWND RichEditControl, const char *Text)
{
	SendMessageA(RichEditControl, EM_LIMITTEXT, 500000, 0);
	return SetWindowTextA(RichEditControl, Text);
}

void NiSkinInstance__LinkObject(__int64 SkinInstance, __int64 Stream)
{
	((void(__fastcall *)(__int64, __int64))OFFSET(0x26B9300, 1530))(SkinInstance, Stream);

	// SkinInstance->RootParent can't be null
	__int64 rootParent = *(__int64 *)(SkinInstance + 0x20);
	const char *nifPath = (const char *)(Stream + 0x108);

	AssertMsgVa(rootParent, "A mesh's NiSkinInstance is missing a skeleton root node. This is a fatal error. NIF path is \"%s\".", nifPath);
}

void hk_call_141299CF5()
{
	MessageBoxA(nullptr, "Could not get the FlowChart class object. Note that the Creation Kit needs to be run as administrator at least one time to register FlowChartX.", "Error", 0);
}