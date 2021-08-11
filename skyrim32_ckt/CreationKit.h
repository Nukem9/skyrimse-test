#pragma once

#include "Loader.h"

namespace CreationKit
{
	struct LipGenTunnel
	{
		// Tunnel data is offset by 0x4 for some reason
		// +0x8 = int32 status value 0,1,2,3
		// +0xC = int32 number of floating point values
		// +0x10 = int32 number of expressions (text)
		char unknown[0x20];
		char InputWAVPath[MAX_PATH];
		char ResampleTempWAVPath[MAX_PATH];
		char DialogueText[MAX_PATH];
		char FonixDataPath[MAX_PATH];
		char Language[MAX_PATH];
		char data[0x2F21];					// Floating point/expressions/custom phoneme data?
		bool UnknownStatus;
	};
	static_assert(offsetof(LipGenTunnel, InputWAVPath) == 0x20);
	static_assert(offsetof(LipGenTunnel, ResampleTempWAVPath) == 0x124);
	static_assert(offsetof(LipGenTunnel, DialogueText) == 0x228);
	static_assert(offsetof(LipGenTunnel, FonixDataPath) == 0x32C);
	static_assert(offsetof(LipGenTunnel, Language) == 0x430);
	static_assert(offsetof(LipGenTunnel, UnknownStatus) == 0x3455);

	void SetFaceFXDataPath(const char *Path);
	void SetFaceFXLanguage(const char *Language);
	void SetFaceFXAutoResampling(bool Resample);

	void FaceFXLogCallback(const char *Text, int Type);
	void LogCallback(int Type, const char *Format, ...);

	void *__fastcall MemoryManager_Alloc(void *Thisptr, void *_EDX, uint32_t Size, uint32_t Alignment, bool Aligned);
	void __fastcall MemoryManager_Free(void *Thisptr, void *_EDX, void *Ptr, bool Aligned);
	void *ScrapHeap_Alloc(uint32_t Size);
	void ScrapHeap_Free(void *Ptr);
}