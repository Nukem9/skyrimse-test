#pragma once

void FormReferenceMap_RemoveAllEntries();
void *FormReferenceMap_FindOrCreate(uint64_t Key, bool Create);
void FormReferenceMap_RemoveEntry(uint64_t Key);
bool FormReferenceMap_Get(uint64_t Unused, uint64_t Key, void **Value);