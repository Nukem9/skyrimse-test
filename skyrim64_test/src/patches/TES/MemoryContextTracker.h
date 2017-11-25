#pragma once

class MemoryContextTracker
{
private:
	uint32_t *m_IdPtr;
	uint32_t m_OldId;
	uint32_t m_Id;

public:
	MemoryContextTracker(uint32_t Id, const char *File)
	{
//		m_IdPtr = (unsigned int *)(*(uintptr_t *)(__readgsqword(0x58u) + 8i64 * TlsIndex) + 1896i64);
//		m_OldId = *m_IdPtr;
//		*m_IdPtr = Id;
	}

	~MemoryContextTracker()
	{
//		*m_IdPtr = m_OldId;
	}
};