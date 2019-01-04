#pragma once

#include "../TES/NiMain/NiRefObject.h"
#include "../TES/NiMain/NiPointer.h"
#include "../TES/BSReadWriteLock.h"

class TESObjectREFR;

class BSHandleRefObject : public NiRefObject
{
private:
	//
	// 31             11       10          0
	// |--------------|--------|-----------|
	// | Handle Index | Active | Ref Count |
	// |--------------|--------|-----------|
	//
	const static uint32_t ACTIVE_BIT_INDEX	= 10;
	const static uint32_t HANDLE_BIT_INDEX	= 11;
	const static uint32_t REF_COUNT_MASK	= (1u << 10) - 1u;

public:
	BSHandleRefObject() : NiRefObject()
	{
		ClearHandle();
	}

	virtual ~BSHandleRefObject()
	{
		ClearHandle();
	}

	uint32_t IncRefCount()
	{
		AssertMsgDebug(GetRefCount() < REF_COUNT_MASK, "BSHandleRefObject - IncRefCount is about to cause refcount wraparound to 0.");

		return InterlockedIncrement(&m_uiRefCount);
	}

	uint32_t DecRefCount()
	{
		AssertMsgDebug(GetRefCount() != 0, "BSHandleRefObject - DecRefCount called with refcount already 0.");

		uint32_t count = InterlockedDecrement(&m_uiRefCount) & REF_COUNT_MASK;

		if (count <= 0)
			DeleteThis();

		return count;
	}

	uint32_t GetRefCount() const
	{
		return m_uiRefCount & REF_COUNT_MASK;
	}

	void AssignHandle(uint32_t HandleIndex)
	{
		m_uiRefCount = (HandleIndex << HANDLE_BIT_INDEX) | (1u << ACTIVE_BIT_INDEX) | GetRefCount();
	}

	void ClearHandle()
	{
		m_uiRefCount &= REF_COUNT_MASK;
	}

	uint32_t GetHandleIndex() const
	{
		return m_uiRefCount >> HANDLE_BIT_INDEX;
	}

	bool IsHandleActive() const
	{
		return (m_uiRefCount & (1u << ACTIVE_BIT_INDEX)) != 0;
	}
};
static_assert(sizeof(BSHandleRefObject) == 0x10);

template<int IndexBits = 21, int AgeCountBits = 6>
class BSUntypedPointerHandle
{
protected:
	//
	// NOTE: Handle index bits increased from 20 (vanilla) to 21 (limit doubled)
	//
	// 31       28       27    21             0
	// |--------|--------|-----|--------------|
	// | Unused | Active | Age | Handle Index |
	// |--------|--------|-----|--------------|
	//
	const static uint32_t UNUSED_BIT_START	= IndexBits + AgeCountBits;					// 26 in vanilla
	const static uint32_t INDEX_MASK		= (1u << IndexBits) - 1u;					// 0x00FFFFF
	const static uint32_t AGE_MASK			= ((1u << AgeCountBits) - 1u) << IndexBits;	// 0x3F00000
	const static uint32_t ACTIVE_BIT_MASK	= (1u << UNUSED_BIT_START);					// 0x4000000

	uint32_t m_Bits;

public:
	BSUntypedPointerHandle() : m_Bits(0)
	{
	}

	void Clear()
	{
		m_Bits = 0;
	}

	bool IsEmpty() const
	{
		return m_Bits == 0;
	}

	void Set(uint32_t Index, uint32_t Age)
	{
		AssertMsg(Index < (1u << IndexBits), "BSUntypedPointerHandle::Set - parameter Index is too large");

		m_Bits = Index | Age;
	}

	void SetIndex(uint32_t Index)
	{
		m_Bits = (Index & INDEX_MASK) | (m_Bits & ~INDEX_MASK);
	}

	uint32_t GetIndex() const
	{
		return m_Bits & INDEX_MASK;
	}

	void ResetAge()
	{
		// Don't know what this is doing
		m_Bits = ((m_Bits << IndexBits) & AGE_MASK) | (m_Bits & ~AGE_MASK);
	}

	bool AgeMatches(uint32_t RawAge) const
	{
		return IsActive() && GetRawAge() == RawAge;
	}

	uint32_t GetRawAge() const
	{
		// Bits are not shifted here
		return m_Bits & AGE_MASK;
	}

	void SetActive()
	{
		m_Bits |= ACTIVE_BIT_MASK;
	}

	void ClearActive()
	{
		m_Bits &= ~ACTIVE_BIT_MASK;
	}

	bool IsActive() const
	{
		return (m_Bits & ACTIVE_BIT_MASK) != 0;
	}

	BSUntypedPointerHandle<> operator=(const BSUntypedPointerHandle<>& Other)
	{
		m_Bits = Other.m_Bits;
		return *this;
	}

	bool operator==(const BSUntypedPointerHandle<>& Other) const
	{
		return m_Bits == Other.m_Bits;
	}

	bool operator!=(const BSUntypedPointerHandle<>& Other) const
	{
		return m_Bits != Other.m_Bits;
	}
};
static_assert(sizeof(BSUntypedPointerHandle<>) == 0x4);

template<typename ObjectType, typename HandleType = BSUntypedPointerHandle<>>
class BSPointerHandle : public HandleType, protected NiPointer<BSHandleRefObject>
{
public:
	BSPointerHandle() : HandleType(), NiPointer()
	{
	}
	
	void SetPtr(BSHandleRefObject *Object)
	{
		NiPointer::operator=(Object);
	}

	BSHandleRefObject *GetPtr() const
	{
		return NiPointer::operator BSHandleRefObject *();
	}
};
static_assert(sizeof(BSPointerHandle<TESObjectREFR>) == 0x10);

class BSPointerHandleManagerInterface
{
public:
	const static uint32_t MAX_HANDLE_COUNT = 1 << 21;

	inline static BSReadWriteLock HandleTableLock;
	inline static std::vector<BSPointerHandle<TESObjectREFR>> HandleTable;
	inline const static BSUntypedPointerHandle<> EmptyHandle;

	inline static uint32_t NextPointerHandleIndex;
	inline static uint32_t LastPointerHandleIndex;

	static void Initialize();
	static BSUntypedPointerHandle<> GetCurrentHandle(TESObjectREFR *Refr);
	static BSUntypedPointerHandle<> CreateHandle(TESObjectREFR *Refr);
	static void ReleaseHandle(const BSUntypedPointerHandle<>& Handle);
	static void ReleaseHandleAndClear(BSUntypedPointerHandle<>& Handle);
	static void CheckForLeaks();
	static void ClearActiveHandles();
	static bool sub_141293870(const BSUntypedPointerHandle<>& Handle, NiPointer<TESObjectREFR>& Out);
	static bool sub_1412E25B0(BSUntypedPointerHandle<>& Handle, NiPointer<TESObjectREFR>& Out);
	static bool sub_1414C52B0(const BSUntypedPointerHandle<>& Handle);
};