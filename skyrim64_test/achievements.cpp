#include "stdafx.h"

struct
{
	const char *BytesToFind;
	const char *FindMask;
	const char *BytePatch;
	int PatchSize;
	int AddressModifier;
} Patches[] =
{
	// https://github.com/Sumwunn/AchievementsModsEnabler

	// SkyrimSE 1.2
	{
		"\xC3\x40\x32\xFF\x48\x89\x5C\x24\x40\x48\x89\x6C\x24\x48",
		"xxxxxxxxxxxxxx",
		"\xB0\x00\xC3",
		3,
		-0x30
	},

	// SkyrimSE 1.1 or lower
	{
		"\xC3\x48\x89\x5C\x24\x40\x48\x89\x6C\x24\x48\x8B\xA9\x70\x0D\x00\x00",
		"xxxxxxxxxxxxxxxxx",
		"\xB0\x00\xC3",
		3,
		-0x35
	},
};

void PatchAchievements()
{
	// Loop through each fix and break on the first found
	for (int i = 0; i < ARRAYSIZE(Patches); i++)
	{
		auto patch = &Patches[i];

		ULONG_PTR addr = FindPattern(
			std::vector<unsigned char>((PUCHAR)g_CodeBase, (PUCHAR)(g_CodeBase + g_CodeSize)),
			g_CodeBase,
			(PUCHAR)patch->BytesToFind,
			patch->FindMask,
			patch->AddressModifier,
			0);

		if (!addr)
		{
			MessageBoxA(nullptr, "Failed to find achievement patch address", "???", 0);
			continue;
		}

		PatchMemory(addr, (PBYTE)patch->BytePatch, patch->PatchSize);

		// Patch applied, we're done here
		break;
	}
}