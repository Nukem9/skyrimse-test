#pragma once

#include "../TES/BSTArray.h"

void FormReferenceMap_RemoveAllEntries();
void *FormReferenceMap_FindOrCreate(uint64_t Key, bool Create);
void FormReferenceMap_RemoveEntry(uint64_t Key);
bool FormReferenceMap_Get(uint64_t Unused, uint64_t Key, void **Value);

void *AlteredFormList_Create(BSTArray<class TESForm *> *Array, uint32_t Unknown);
void AlteredFormList_RemoveAllEntries(BSTArray<class TESForm *> *Array, bool Unknown);
void AlteredFormList_Insert(BSTArray<class TESForm *> *Array, class TESForm *&Entry);
void AlteredFormList_RemoveEntry(BSTArray<class TESForm *> *Array, uint32_t Index, uint32_t Unknown);
bool AlteredFormList_ElementExists(BSTArray<class TESForm *> *Array, class TESForm *&Entry);