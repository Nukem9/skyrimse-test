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