#pragma once

class MemoryManager
{
private:
	MemoryManager() {}
	~MemoryManager() {}

public:
	void *Alloc(size_t Size, uint32_t Alignment, bool Aligned);
	void Free(void *Memory, bool Aligned);
};