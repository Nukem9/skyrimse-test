#pragma once

class MemoryContextTracker
{
private:
	uint32_t m_OldId;

public:
	inline MemoryContextTracker(uint32_t Id, const char *File)
	{
		m_OldId = GAME_TLS(uint32_t, 0x768);
		GAME_TLS(uint32_t, 0x768) = Id;
	}

	inline ~MemoryContextTracker()
	{
		GAME_TLS(uint32_t, 0x768) = m_OldId;
	}
};