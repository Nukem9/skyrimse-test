#pragma once

#include "../../common.h"
#include "../TES/BSTArray.h"

struct z_stream_s;

HWND WINAPI hk_CreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
INT_PTR WINAPI hk_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
BOOL WINAPI hk_EndDialog(HWND hDlg, INT_PTR nResult);
LRESULT WINAPI hk_SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

int hk_inflateInit(z_stream_s *Stream, const char *Version, int Mode);
int hk_inflate(z_stream_s *Stream, int Flush);
bool OpenPluginSaveDialog(HWND ParentWindow, const char *BasePath, bool IsESM, char *Buffer, uint32_t BufferSize, const char *Directory);
bool IsBSAVersionCurrent(class BSFile *File);
bool IsLipDataPresent(void *Thisptr);
bool WriteLipData(void *Thisptr, const char *Path, int Unkown1, bool Unknown2, bool Unknown3);
int IsWavDataPresent(const char *Path, __int64 a2, __int64 a3, __int64 a4);
INT_PTR CALLBACK LipRecordDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

uint32_t GetESLMasterCount();
const char *GetESLMasterName(uint32_t Index);
bool IsESLMaster(const char *Name);

bool sub_141477DA0_SSE41(__int64 a1);
bool sub_141477DA0(__int64 a1);
uint32_t sub_1414974E0(BSTArray<void *>& Array, const void *&Target, uint32_t StartIndex, __int64 Unused);

void UpdateLoadProgressBar();
void UpdateObjectWindowTreeView(void *Thisptr, HWND ControlHandle);

void InsertComboBoxItem(HWND ComboBoxHandle, const char *DisplayText, void *Value, bool AllowResize);
void InsertListViewItem(HWND ListViewHandle, void *Parameter, bool UseImage, int ItemIndex);

void PatchTemplatedFormIterator();
LRESULT CSScript_PickScriptsToCompileDlg_WindowMessage(void *Thisptr, UINT Message, WPARAM WParam, LPARAM LParam);

void SortFormArray(BSTArray<class TESForm *> *Array, int(*SortFunction)(const void *, const void *));
void SortDialogueInfo(__int64 TESDataHandler, uint32_t FormType, int(*SortFunction)(const void *, const void *));

class IRendererResourceManager
{
};

class BSShaderResourceManager : public IRendererResourceManager
{
public:
	bool FindIntersectionsTriShapeFastPath(class NiPoint3 *P1, class NiPoint3 *P2, class NiPick *Pick, class BSTriShape *Shape);
};

struct PerkRankEntry
{
	union
	{
		struct
		{
			uint32_t FormId;	// 0x0
			uint8_t Rank;		// 0x4
		};

		struct
		{
			uint64_t FormIdOrPointer;	// 0x0
			uint8_t NewRank;			// 0x8
		};
	};
};
static_assert(sizeof(PerkRankEntry) == 0x10);

void QuitHandler();

void ListViewUnselectItem(HWND ListViewHandle, void *Parameter);
void ListViewSelectItem(HWND ListViewHandle, int ItemIndex, bool KeepOtherSelections);
void ListViewFindAndSelectItem(HWND ListViewHandle, void *Parameter, bool KeepOtherSelections);

void hk_sub_141047AB2(__int64 FileHandle, __int64 *Value);
bool hk_BGSPerkRankArray_sub_14168DF70(PerkRankEntry *Entry, uint32_t *FormId, __int64 UnknownArray);
void hk_BGSPerkRankArray_sub_14168EAE0(__int64 ArrayHandle, PerkRankEntry *&Entry);

void FaceGenOverflowWarning(__int64 Texture);