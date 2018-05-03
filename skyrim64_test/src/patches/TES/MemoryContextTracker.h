#pragma once

class MemoryContextTracker
{
private:
	uint32_t& m_Id;
	uint32_t m_OldId;

public:
	inline MemoryContextTracker(uint32_t Id, const char *File) : m_Id(GAME_TLS(uint32_t, 0x768))
	{
		m_OldId = m_Id;
		m_Id = Id;
	}

	inline ~MemoryContextTracker()
	{
		m_Id = m_OldId;
	}
};