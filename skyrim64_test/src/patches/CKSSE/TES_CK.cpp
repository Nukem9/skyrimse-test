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

#include "TES_CK.h"
#include "..\offsets.h"
#include "..\..\xutil.h"
#include "..\TES\BSTArray.h"
#include "TESForm_CK.h"
#include "LogWindow.h"

TESFile_CK* TES_CK::GetActivePlugin() const {
	return m_ActivePlugin;
}

uint32_t TES_CK::GetActivePluginMaxFormID() const {
	return ((uint32_t(__fastcall*)(const TES_CK*))OFFSET(0x159A1D0, 16438))(this);
}

TES_CK* TES_CK::GetInstance() {
	return ((TES_CK*(__fastcall*)())OFFSET(0x117E270, 16438))();
}

uint32_t TES_CK::CompactActivePlugin() {
	uint32_t MaxCurrentFormID = 0;
	auto data = *((BSTArray<TESForm_CK*>**)(OFFSET(0x3982A18, 16438)));
	if (!data) return MaxCurrentFormID;

	LogWindow::Log("Starting the compact of the active plugin...");

	auto Instance = GetInstance();
	auto ActivePlugin = Instance->GetActivePlugin();
	if (ActivePlugin) {
		LogWindow::Log("	Active plugin: %s.", ActivePlugin->m_FileName);

		// Result XX000FFF
		auto MaxFormID = Instance->GetActivePluginMaxFormID();
		// Result XX000001
		auto UserMinID = std::min((int)g_INI.GetInteger("CreationKit", "CompactStartFormID", 0x800), 0x800);
		if (UserMinID <= 0) UserMinID = 1;
		auto MinFormID = (ActivePlugin->GetIndexLoader() << 24) + UserMinID;

		LogWindow::Log("	FormID (min/max): %08X/%08X.", MinFormID, MaxFormID);
		LogWindow::Log("	Total forms: %u.", data->QSize());

		auto StartFormID = MinFormID;

		for (uint32_t i = 0; i < data->QSize(); i++) {
			auto form = data->at(i);
			auto formid = form->GetFormID();
			if (form->GetActive() && (formid >= MinFormID)) {
				if (form->GetMarkedDelete())
					LogWindow::Log("		The form %08X is marked for deletion, but a new id will still be given to it.", formid);

				if (formid != StartFormID)
					form->SetNewFormID(StartFormID);

				MaxCurrentFormID = StartFormID;
				StartFormID++;
			}
		}

		LogWindow::Log("	Done. Maximum form ID in active file is now (%08X).", MaxCurrentFormID);
	}
	else
		LogWindow::Log("	Active plugin not found.");

	LogWindow::Log("Stoping the compact of the active plugin.");

	return MaxCurrentFormID;
}