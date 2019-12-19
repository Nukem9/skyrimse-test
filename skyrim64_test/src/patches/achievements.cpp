#include "../common.h"

// clang-format off
struct
{
	const char *BytesToFind;
	const char *BytePatch;
	int PatchSize;
	int AddressModifier;
} const Patches[] =
{
	// SkyrimSE 1.5
	{
		"48 83 EC 28 C6 44 24 38 00 84 D2",
		"\xB0\x00\xC3",
		3,
		0x0
	},

	// SkyrimSE 1.2, 1.3, 1.4
	// https://github.com/Sumwunn/AchievementsModsEnabler
	{
		"C3 40 32 FF 48 89 5C 24 40 48 89 6C 24 48",
		"\xB0\x00\xC3",
		3,
		-0x30
	},

	// SkyrimSE 1.1 or lower
	// https://github.com/Sumwunn/AchievementsModsEnabler
	{
		"C3 48 89 5C 24 40 48 89 6C 24 48 8B A9 70 0D 00 00",
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
		uintptr_t addr = XUtil::FindPattern(g_CodeBase, g_CodeEnd - g_CodeBase, patch.BytesToFind);

        if (!addr)
            continue;

        XUtil::PatchMemory((addr + patch.AddressModifier), (PBYTE)patch.BytePatch, patch.PatchSize);
        return;
    }

    MessageBoxA(nullptr, "Failed to find at least one achievement patch address", "???", 0);
}
