#pragma once

void *MemoryManager_Alloc(void *Heap, size_t Size, unsigned int Alignment, bool Aligned);
void MemoryManager_Free(void *Heap, void *Memory, bool Aligned);

void PatchMemory();