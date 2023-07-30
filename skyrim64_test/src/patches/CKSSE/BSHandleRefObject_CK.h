#pragma once

#include "../TES/NiMain/NiRefObject.h"

class BSHandleRefObject : public NiRefObject
{
private:
	//
	// 63             11       10          0
	// |--------------|--------|-----------|
	// | Handle Index | Active | Ref Count |
	// |--------------|--------|-----------|
	//
	constexpr static uint64_t ACTIVE_BIT_INDEX = 10;
	constexpr static uint64_t HANDLE_BIT_INDEX = ACTIVE_BIT_INDEX + 1;
	constexpr static uint64_t REF_COUNT_MASK = (1ull << ACTIVE_BIT_INDEX) - 1ull;

public:
	BSHandleRefObject()
	{
		ClearHandleEntryIndex();
	}

	virtual ~BSHandleRefObject()
	{
		ClearHandleEntryIndex();
	}

	uint64_t IncRefCount()
	{
		AssertMsgDebug(GetRefCount() < REF_COUNT_MASK, 
			"BSHandleRefObject - IncRefCount is about to cause refcount wraparound to 0.");

		return ((NiRefObject*)this)->IncRefCount();
	}

	uint64_t DecRefCount()
	{
		AssertMsgDebug(GetRefCount() != 0, 
			"BSHandleRefObject - DecRefCount called with refcount already 0.");

		return ((NiRefObject*)this)->DecRefCount();
	}

	uint64_t GetRefCount() const
	{
		return ((uint64_t)m_uiRefCount) & REF_COUNT_MASK;
	}

	void SetHandleEntryIndex(uint64_t HandleIndex)
	{
		m_uiRefCount = (LONGLONG)((HandleIndex << HANDLE_BIT_INDEX) | (1ull << ACTIVE_BIT_INDEX) | GetRefCount());
	}

	uint64_t GetHandleEntryIndex() const
	{
		return ((uint64_t)m_uiRefCount) >> HANDLE_BIT_INDEX;
	}

	void ClearHandleEntryIndex()
	{
		m_uiRefCount &= REF_COUNT_MASK;
	}

	bool IsHandleValid() const
	{
		return (m_uiRefCount & (1ull << ACTIVE_BIT_INDEX)) != 0;
	}
};
static_assert(sizeof(BSHandleRefObject) == 0x10);