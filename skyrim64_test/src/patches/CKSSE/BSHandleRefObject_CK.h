#pragma once

#include "../TES/NiMain/NiRefObject.h"

class BSHandleRefObject : public NiRefObject
{
private:
	//
	// 31             10       9          0
	// |--------------|---------|----------|
	// | Handle Index | Active  |Ref Count |
	// |--------------|---------|----------|
	//
	constexpr static uint32_t ACTIVE_BIT_INDEX = 9;
	constexpr static uint32_t HANDLE_BIT_INDEX = 10;
	constexpr static uint32_t REF_COUNT_MASK = (1u << 10) - 1u;

public:
	BSHandleRefObject()
	{
		ClearHandleEntryIndex();
	}

	virtual ~BSHandleRefObject()
	{
		ClearHandleEntryIndex();
	}

	uint32_t IncRefCount()
	{
		AssertMsgDebug(QRefCount() < REF_COUNT_MASK, "BSHandleRefObject - IncRefCount is about to cause refcount wraparound to 0.");

		return InterlockedIncrement(&m_uiRefCount);
	}

	uint32_t DecRefCount()
	{
		AssertMsgDebug(QRefCount() != 0, "BSHandleRefObject - DecRefCount called with refcount already 0.");

		uint32_t count = InterlockedDecrement(&m_uiRefCount) & REF_COUNT_MASK;

		if (count <= 0)
			DeleteThis();

		return count;
	}

	uint32_t QRefCount() const
	{
		return m_uiRefCount & REF_COUNT_MASK;
	}

	void SetHandleEntryIndex(uint32_t HandleIndex)
	{
		m_uiRefCount = (HandleIndex << HANDLE_BIT_INDEX) | (1u << ACTIVE_BIT_INDEX) | QRefCount();
	}

	uint32_t QHandleEntryIndex() const
	{
		return m_uiRefCount >> HANDLE_BIT_INDEX;
	}

	void ClearHandleEntryIndex()
	{
		m_uiRefCount &= REF_COUNT_MASK;
	}

	bool IsHandleValid() const
	{
		return (m_uiRefCount & (1u << ACTIVE_BIT_INDEX)) != 0;
	}
};
static_assert(sizeof(BSHandleRefObject) == 0x10);