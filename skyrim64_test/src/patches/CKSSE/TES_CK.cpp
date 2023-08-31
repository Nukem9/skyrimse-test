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
#include "BSString.h"
#include "TESForm_CK.h"
#include "LogWindow.h"
#include "BSResourceArchive.h"
#include "BSArchiveManager.h"

uint32_t TES_CK::GetActivePluginMinFormID()
{
	auto Instance = GetInstance();
	AssertMsg(Instance, "The main data store has not been created.");
	
	auto ActivePlugin = Instance->GetActiveMod();
	AssertMsg(ActivePlugin, "The plugin is not selected as active.");

	auto UserMinID = std::min((int)g_INI.GetInteger("CreationKit", "CompactStartFormID", 0x800), 0x800);
	if (UserMinID <= 0) UserMinID = 1;
	auto MinFormID = (ActivePlugin->GetIndexLoader() << 24) | UserMinID;
	return MinFormID;
}

uint32_t TES_CK::GetActivePluginMaxFormID() {
	return ((uint32_t(__fastcall*)(const TES_CK*))OFFSET(0x159A1D0, 16438))(GetInstance());
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
	auto ActivePlugin = Instance->GetActiveMod();
	if (ActivePlugin) {
		LogWindow::Log("	Active plugin: %s.", ActivePlugin->GetFileName().c_str());

		// Result XX000FFF
		auto MaxFormID = Instance->GetActivePluginMaxFormID();
		// Result XX000001
		auto UserMinID = std::min((int)g_INI.GetInteger("CreationKit", "CompactStartFormID", 0x800), 0x800);
		if (UserMinID <= 0) UserMinID = 1;
		auto MinFormID = (ActivePlugin->GetIndexLoader() << 24) | UserMinID;

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

void TES_CK::SetLoaderIdByForm(const TESFile_CK* load_file)
{
	// My attempts to force the code to open compact plugins ignoring the 0x800 limit.
	// The key difference from Fallout 4 is that this function corrects in the file buffer, 
	// after reading the dependency index bits of the form.

	// What exactly is the problem: The problem is that any form that has an index less 
	// than 0x800 is reserved for the engine, well, like the form of a player, money, etc.
	// Therefore, when opening a plugin with such indexes, it just dumped out a ton of errors, 
	// and spoiled the index of the forms of the active plugin, making them 00.

	auto IdPtr = (uint32_t*)((uintptr_t)load_file + 0x290);
	//auto OriginalId = *(byte*)((uintptr_t)load_file + 0x460);
	auto DependObjs = (void*)((uintptr_t)load_file + 0x468);
	//auto LoadedId = *(byte*)((uintptr_t)load_file + 0x478);
	auto DependId = *IdPtr >> 24;

	if (DependId >= 0xFE)
	{
		// Usually there is a situation when opening a list of plugins

		*IdPtr &= 0xFFFFFF;
		return;
	}

	if (!DependObjs && DependId)
		LogWindow::Log("Form(%08X) in non-dependent file contains a form with file index bits.", *IdPtr);

	/*if (OriginalId)
	{
		if (OriginalId != DependId)
			LogWindow::Log("Form(%08X) contains different data on the dependency index bits.", *IdPtr);

		*IdPtr |= (LoadedId << 24);
	}
	else
	{*/
	//	if ((*IdPtr - 1) < 0x7FF)
	//		*IdPtr &= 0xFFFFFF;
	//}

	// As a result, i do'nt allow to spoil the index of the form in the buffer, 
	// while not correcting its loading index.
}

std::vector<const TESFile_CK*> g_SelectedFilesArray;

VOID FIXAPI AttachBSAFile(LPCSTR _filename) {
	if (BSArchiveManager::IsAvailableForLoad(_filename))
		goto attach_ba2;

	return;

attach_ba2:
	BSResourceArchive::LoadArchive(_filename);
}

void TES_CK::LoadTesFile(const TESFile_CK* load_file)
{
	// Sometimes duplicated
	if (std::find(g_SelectedFilesArray.begin(), g_SelectedFilesArray.end(), load_file) == 
		g_SelectedFilesArray.end()) {
		if (load_file->IsActive()) {
			_MESSAGE_FMT("Load active file %s...", load_file->GetFileName().c_str());
			//#if FALLOUT4_DEVELOPER_MODE
			//			File->Dump();
			//#endif // !FALLOUT4_DEVELOPER_MODE
		}
		else if (load_file->IsMaster() || load_file->IsSmallMaster())
			_MESSAGE_FMT("Load master file %s...", load_file->GetFileName().c_str());
		else
			_MESSAGE_FMT("Load file %s...", load_file->GetFileName().c_str());

		g_SelectedFilesArray.push_back(load_file);
	}

	if (g_OwnArchiveLoader)
	{
		auto sname = load_file->GetFileName();
		sname.Copy(0, sname.FindLastOf('.'));

		AttachBSAFile(*(sname + ".bsa"));
		AttachBSAFile(*(sname + " - Textures.bsa"));
	}

	((void(__fastcall*)(const TESFile_CK*))OFFSET(0x1664CC0, 1530))(load_file);
}

void TES_CK::LoadTesFileFinal()
{
	g_SelectedFilesArray.clear();
}