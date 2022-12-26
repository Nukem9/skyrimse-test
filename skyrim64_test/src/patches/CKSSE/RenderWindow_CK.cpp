// MIT License
//
// Copyright(c) 2022 Perchik71 <Perchik71@Outlook.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "RenderWindow_CK.h"

namespace RenderWindow {
	DLGPROC OldRenderWndProc;
	static HWND RenderWndHandle;
	static bool blockInputMessage;
	static bool loadCell;

	INT_PTR CALLBACK RenderWndProc(HWND DialogHwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		if (Message == WM_INITDIALOG) {
			RenderWndHandle = DialogHwnd;
			blockInputMessage = true;
			loadCell = false;
		}
		else if (blockInputMessage) {
			switch (Message) {
			case WM_KEYUP:
			case WM_KEYDOWN:
			case WM_SYSCHAR:
			case WM_SYSKEYUP:
			case WM_SYSKEYDOWN:
			case WM_MOUSEMOVE:
			case WM_MOUSEWHEEL:
			case WM_LBUTTONUP:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONDBLCLK:
				// block messages
				return 0;
			default:
				break;
			}
		}
		
		return OldRenderWndProc(DialogHwnd, Message, wParam, lParam);
	}

	//void unlockInputMessages() {
	//	blockInputMessage = false;
	//}

	void setFlagLoadCell() {
		blockInputMessage = false;
		loadCell = true;
	}
}