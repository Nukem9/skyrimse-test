#pragma once

#define OFFSET(RelOffset, Version) (ResolveOffset(RelOffset, Version))
#define OFFSET_ENTRY(RelOffset, Version, Signature, SigAdjustment, TranslatedAddress) { RelOffset, Version, Signature, SigAdjustment, TranslatedAddress },

struct OffsetEntry
{
	uint32_t RelOffset;
	uint32_t Version;
	const char *Signature;
	int SigAdjustment;
	uintptr_t TranslatedAddress;
};

extern const OffsetEntry OffsetEntryList1530[];
extern const OffsetEntry OffsetEntryList1573[];

uintptr_t ResolveOffset(uint32_t RelOffset, uint32_t Version);
void BuildTableForVersion(uint32_t Version);
void DumpTable();