// MIT License
//
// Copyright(c) 2023 Perchik71 <Perchik71@Outlook.com>
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

#pragma once

#include "..\..\common.h"
#include "TESFile_CK.h"
#include "..\TES\BSTArray.h"
#include "..\TES\BSTList.h"
#include "TESForm_CK.h"
#include "..\TES\NiMain\NiArray.h"

class TES_CK
{
public:
	using UnkArray = BSTArray<TESForm_CK*>;
	using TESFileList = BSSimpleList<TESFile_CK>;
	using TESFileArray = BSTArray<TESFile_CK*>;

	struct ModList {
		TESFileList modInfoList;		// 00
		TESFileArray loadedMods;		// 10
	};
public:
	uint32_t GetActivePluginMaxFormID() const;

	inline const TESFileList* GetMods(VOID) const { return &modList.modInfoList; }
	inline const TESFileArray* GetLoadedMods(VOID) const { return &modList.loadedMods; }
	inline TESFile_CK* GetActiveMod(VOID) const { return m_ActivePlugin; }
	inline bool IsActiveMod(VOID) const { return m_ActivePlugin != nullptr; }
public:
	static TES_CK* GetInstance();
	static uint32_t CompactActivePlugin();
	static void SetLoaderIdByForm(const TESFile_CK* load_file);
private:
	char pad0[0x68];
	UnkArray arrForm[0x8A];							// All array forms by type
	LPVOID UnkD58;
	NiArray<TESForm_CK*> cellList;
	NiArray<TESForm_CK*> addonNodes;
	char pad1[0x20];
	TESFile_CK* m_ActivePlugin;						// Active plugin
	ModList modList;
};