#include <libdeflate/libdeflate.h>
#include <mutex>
#include <set>
#include <atomic>
#include "../../common.h"

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

std::recursive_mutex dialogMutex;
std::set<HWND> g_ParentCreateDialogHwnds;
std::set<HWND> g_ParentDialogHwnds;
std::atomic<uint64_t> g_OpenDialogCount;

HWND WINAPI hk_CreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	// NOTE: This is NOT an actual dialog. It uses CreateWindowExA internally.
	dialogMutex.lock();
	g_ParentCreateDialogHwnds.insert(hWndParent);
	dialogMutex.unlock();

	return CreateDialogParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR WINAPI hk_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	dialogMutex.lock();
	Assert(IsWindow(hWndParent));
	Assert(g_ParentDialogHwnds.count(hWndParent) <= 0);

	g_ParentDialogHwnds.insert(hWndParent);
	dialogMutex.unlock();

	g_OpenDialogCount++;
	return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

BOOL WINAPI hk_EndDialog(HWND hDlg, INT_PTR nResult)
{
	Assert(hDlg);
	HWND parent = GetParent(hDlg);

	dialogMutex.lock();
	{
		if (g_OpenDialogCount <= 0)
		{
			// Fix for the CK calling EndDialog on a CreateDialogParamA window
			if (g_ParentCreateDialogHwnds.count(parent) > 0)
			{
				g_ParentCreateDialogHwnds.erase(parent);
				DestroyWindow(hDlg);
			}

			dialogMutex.unlock();
			return FALSE;
		}

		g_ParentDialogHwnds.erase(parent);
	}
	dialogMutex.unlock();

	g_OpenDialogCount--;
	return EndDialog(hDlg, nResult);
}

LRESULT WINAPI hk_SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (hWnd && Msg == WM_DESTROY)
	{
		dialogMutex.lock();
		bool found = g_ParentDialogHwnds.count(GetParent(hWnd)) > 0;
		dialogMutex.unlock();

		if (found)
		{
			// This is a dialog, we can't call DestroyWindow on it
			return 0;
		}

		DestroyWindow(hWnd);
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