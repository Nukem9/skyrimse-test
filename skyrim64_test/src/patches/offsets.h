#pragma once

#include <vector>

#define OFFSET(RelOffset, Version) (Offsets::Resolve(RelOffset, Version))
#define OFFSET_ENTRY_KEY(RelOffset, Version) (((uint64_t)(Version) << 32) | (RelOffset))
#define OFFSET_ENTRY(RelOffset, Version, Signature, SigAdjustment, TranslatedOffset) { RelOffset, Version, Signature, SigAdjustment, (uint32_t)(TranslatedOffset) },

namespace Offsets
{
	struct OffsetEntry
	{
		uint32_t RelOffset;
		uint32_t Version;
		const char *Signature;
		int SigAdjustment;
		uint32_t TranslatedOffset;
	};

	extern const std::vector<OffsetEntry> EntryListCK1530;
	extern const std::vector<OffsetEntry> EntryListCK1573;
	extern const std::vector<OffsetEntry> EntryListCK16438;

	uintptr_t Resolve(uint32_t RelOffset, uint32_t Version);
	bool CanResolve(uint32_t RelOffset, uint32_t Version);

	bool IsCKVersion16438(void);
	bool IsCKVersion16438OrNewer(void);
	bool IsCKVersion1573(void);
	bool IsCKVersion1573OrNewer(void);

	void BuildTableForCKSSEVersion(uint32_t Version);
	void BuildTableForGameVersion(uint32_t Version);
	void BuildTable(const std::vector<OffsetEntry>& Table, bool CurrentVersion);
	void ValidateTable(const std::vector<OffsetEntry>& Table);
	void DumpLoadedTable(const char *FilePath);
}