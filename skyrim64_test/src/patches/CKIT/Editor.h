#pragma once

#include "../../common.h"

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

uint32_t GetESLMasterCount();
const char *GetESLMasterName(uint32_t Index);
bool IsESLMaster(const char *Name);

bool sub_141477DA0_SSE41(__int64 a1);
bool sub_141477DA0(__int64 a1);
void UpdateLoadProgressBar();
void UpdateObjectWindowTreeView(void *Thisptr, HWND ControlHandle);

void InsertComboBoxItem(HWND ComboBoxHandle, const char *DisplayText, const char *Value, bool AllowResize);
void InsertListViewItem(HWND ListViewHandle, void *Parameter, bool UseImage, int ItemIndex);

void PatchTemplatedFormIterator();
LRESULT CSScript_PickScriptsToCompileDlg_WindowMessage(void *Thisptr, UINT Message, WPARAM WParam, LPARAM LParam);
void DialogueInfoSort(__int64 TESDataHandler, uint32_t FormType, void *SortFunction);

class IRendererResourceManager
{
};

class BSShaderResourceManager : public IRendererResourceManager
{
public:
	bool FindIntersectionsTriShapeFastPath(class NiPoint3 *P1, class NiPoint3 *P2, class NiPick *Pick, class BSTriShape *Shape);
};

void QuitHandler();