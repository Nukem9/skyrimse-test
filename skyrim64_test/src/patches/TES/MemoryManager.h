#pragma once

class MemoryManager
{
private:
	MemoryManager() = default;
	~MemoryManager() = default;

public:
	static void *Alloc(MemoryManager *Manager, size_t Size, uint32_t Alignment, bool Aligned);
	static void Free(MemoryManager *Manager, void *Memory, bool Aligned);
};

class ScrapHeap
{
private:
	ScrapHeap() = default;
	~ScrapHeap() = default;

public:
	const static uint32_t MAX_ALLOC_SIZE = 0x4000000;

	static void *Alloc(ScrapHeap *Heap, size_t Size, uint32_t Alignment);
	static void Free(ScrapHeap *Heap, void *Memory);
};