#pragma once

void *Scaleform_HeapAlloc(void *Heap, size_t Size, unsigned int Alignment, bool Aligned);
void Scaleform_HeapFree(void *Heap, void *Memory, bool Aligned);

void PatchMemory();