#include "../../common.h"
#include "TESForm_CK.h"

#define FORM_REFERENCE_KEY(x) (((x & 3) == 0) ? (x >> 2) : x)

std::unordered_map<uint64_t, void *> m_Entries;

void FormReferenceMap_RemoveAllEntries()
{
	for (auto[k, v] : m_Entries)
	{
		if (v)
			((void(__fastcall *)(void *, int))(g_ModuleBase + 0x149F560))(v, 1);
	}

	m_Entries.clear();
}

void *FormReferenceMap_FindOrCreate(uint64_t Key, bool Create)
{
	auto itr = m_Entries.find(FORM_REFERENCE_KEY(Key));

	if (itr != m_Entries.end() && itr->second)
		return itr->second;

	if (Create)
	{
		void *ptr = ((void *(__fastcall *)(size_t))(g_ModuleBase + 0x1219450))(24);

		if (ptr)
			ptr = ((void *(__fastcall *)(void *))(g_ModuleBase + 0x1397CD0))(ptr);

		m_Entries.insert_or_assign(FORM_REFERENCE_KEY(Key), ptr);
		return ptr;
	}

	return nullptr;
}

void FormReferenceMap_RemoveEntry(uint64_t Key)
{
	auto itr = m_Entries.find(FORM_REFERENCE_KEY(Key));

	if (itr != m_Entries.end() && itr->second)
	{
		void *ptr = itr->second;
		m_Entries.erase(itr);

		((void(__fastcall *)(void *, int))(g_ModuleBase + 0x149F560))(ptr, 1);
	}
}

bool FormReferenceMap_Get(uint64_t Unused, uint64_t Key, void **Value)
{
	// Function doesn't care if entry is nullptr, only if it exists
	auto itr = m_Entries.find(FORM_REFERENCE_KEY(Key));

	if (itr != m_Entries.end())
	{
		*Value = itr->second;
		return true;
	}

	return false;
}