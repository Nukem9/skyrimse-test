#include "../common.h"

// clang-format off
struct
{
	const char *BytesToFind;
	const char *FindMask;
	const char *BytePatch;
	int PatchSize;
	int AddressModifier;
} const Patches[] =
{
	// SkyrimSE 1.5
	{
		"\x48\x83\xEC\x28\xC6\x44\x24\x38\x00\x84\xD2",
		"xxxxxxxxxxx",
		"\xB0\x00\xC3",
		3,
		0x0
	},

	// SkyrimSE 1.2, 1.3, 1.4
	// https://github.com/Sumwunn/AchievementsModsEnabler
	{
		"\xC3\x40\x32\xFF\x48\x89\x5C\x24\x40\x48\x89\x6C\x24\x48",
		"xxxxxxxxxxxxxx",
		"\xB0\x00\xC3",
		3,
		-0x30
	},

	// SkyrimSE 1.1 or lower
	// https://github.com/Sumwunn/AchievementsModsEnabler
	{
		"\xC3\x48\x89\x5C\x24\x40\x48\x89\x6C\x24\x48\x8B\xA9\x70\x0D\x00\x00",
		"xxxxxxxxxxxxxxxxx",
		"\xB0\x00\xC3",
		3,
		-0x35
	},
};
// clang-format on

void PatchAchievements()
{
    // Loop through each fix and exit on the first found
    for (auto &patch : Patches)
    {
		uintptr_t addr = FindPatternSimple(g_CodeBase, g_CodeSize, (uint8_t *)patch.BytesToFind, patch.FindMask);

        if (!addr)
            continue;

        PatchMemory((addr + patch.AddressModifier), (PBYTE)patch.BytePatch, patch.PatchSize);
        return;
    }

    MessageBoxA(nullptr, "Failed to find at least one achievement patch address", "???", 0);
}
