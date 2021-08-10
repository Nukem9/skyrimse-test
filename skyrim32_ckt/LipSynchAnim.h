#pragma once

#include <stdio.h>

struct LipHeader
{
	enum LipHeaderFlags : int
	{
		Compressed = 1,
		BigEndian = 2,
		HasGestures = 4,
		VariableTargets = 8,
	};

	int Version;
	int Size;
	LipHeaderFlags Flags;
};
static_assert(sizeof(LipHeader) == 0xC);

class LipSynchAnim
{
public:
	static LipSynchAnim *Generate(const char *WavPath, const char *ResamplePath, const char *DialogueText, void *FaceFXPhonemeData);
	bool SaveToFile(const char *Path, bool Compress = true, int NumTargets = 16, bool FacegenDefault = true);
	void Free();

	static int hk_call_00587816(FILE *File, void *Data, int Size);
};