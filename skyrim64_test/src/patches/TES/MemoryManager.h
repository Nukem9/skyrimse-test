#pragma once

class MemoryManager
{
private:
	MemoryManager() = default;
	~MemoryManager() = default;

public:
	static void *Allocate(MemoryManager *Manager, size_t Size, uint32_t Alignment, bool Aligned);
	static void Deallocate(MemoryManager *Manager, void *Memory, bool Aligned);
	static size_t Size(MemoryManager *Manager, void *Memory);
};

class ScrapHeap
{
private:
	ScrapHeap() = default;
	~ScrapHeap() = default;

public:
	const static uint32_t MAX_ALLOC_SIZE = 0x4000000;

	void *Allocate(size_t Size, uint32_t Alignment);
	void Deallocate(void *Memory);
};