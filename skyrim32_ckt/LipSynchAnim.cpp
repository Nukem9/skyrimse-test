#include "CreationKit.h"
#include "LipSynchAnim.h"

LipSynchAnim *LipSynchAnim::Generate(const char *WavPath, const char *ResamplePath, const char *DialogueText, void *FaceFXPhonemeData)
{
	return ((LipSynchAnim * (__cdecl *)(const char *, const char *, const char *, const char *, void *))(0x46ACD0))
		(WavPath, ResamplePath, "", DialogueText, FaceFXPhonemeData);
}

bool LipSynchAnim::SaveToFile(const char *Path, bool Compress, int NumTargets, bool FacegenDefault)
{
	if (FILE *f; fopen_s(&f, Path, "wb") == 0)
	{
		// Warning: sub_587730 has been manually patched to take a FILE handle
		bool result = ((bool(__thiscall *)(LipSynchAnim *, FILE *, bool, int, int))(0x587730))(this, f, Compress, NumTargets, FacegenDefault);
		fclose(f);

		return result;
	}

	return false;
}

void LipSynchAnim::Free()
{
	((void(__thiscall *)(LipSynchAnim *))(0x586A40))(this);
	CreationKit::MemoryManager_Free(nullptr, nullptr, this, false);
}

int LipSynchAnim::hk_call_00587816(FILE *File, void *Data, int Size)
{
	return fwrite(Data, 1, Size, File);
}