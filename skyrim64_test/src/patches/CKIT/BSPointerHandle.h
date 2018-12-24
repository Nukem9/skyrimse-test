#pragma once

class BSHandleRefObject;
class TESObjectREFR;

template<int IndexBits = 21, int AgeCountBits = 6>
class BSUntypedPointerHandle
{
protected:
	uint32_t m_Bits;

	const static uint32_t UNUSED_BIT_START	= IndexBits + AgeCountBits;					// 26 in vanilla
	const static uint32_t INDEX_MASK		= (1u << IndexBits) - 1u;					// 0x00FFFFF
	const static uint32_t AGE_MASK			= ((1u << AgeCountBits) - 1u) << IndexBits;	// 0x3F00000
	const static uint32_t ACTIVE_BIT_MASK	= (1u << UNUSED_BIT_START);					// 0x4000000

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

	void SetRaw(uint32_t Index, uint32_t Age)
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

template<typename ObjectType, typename HandleType = BSUntypedPointerHandle<>>
class BSPointerHandle : public HandleType
{
protected:
	BSHandleRefObject *m_Object;

public:
	BSPointerHandle() : m_Object(nullptr)
	{
	}

	~BSPointerHandle()
	{
		// Calls DecRef() on object if it's not a nullptr
	}

	void AssignObject(BSHandleRefObject *Object)
	{
		if (m_Object != Object)
		{
			BSHandleRefObject *temp = m_Object;

			m_Object = ((BSHandleRefObject *(__fastcall *)(void *, void *))(g_ModuleBase + 0x128F860))(this, Object);
			((void(__fastcall *)(void *, void *))(g_ModuleBase + 0x1295EA0))(this, temp);
		}
	}

	BSHandleRefObject *GetPtr()
	{
		return m_Object;
	}
};

class BSPointerHandleManagerInterface
{
public:
	inline static BSPointerHandle<TESObjectREFR, BSUntypedPointerHandle<>> m_HandleTable[2 * 1024 * 1024];
	inline static BSUntypedPointerHandle<> EmptyHandle;
	inline static uint32_t g_NextPointerHandleIndex;
	inline static uint32_t g_LastPointerHandleIndex;

	static_assert(sizeof(m_HandleTable[0]) == 0x10);

	static void acquire_lock();
	static void release_lock();

	static void Initialize();
	static BSUntypedPointerHandle<> GetCurrentHandle(TESObjectREFR *Refr);
	static BSUntypedPointerHandle<> CreateHandle(TESObjectREFR *Refr);
	static void ReleaseHandle(const BSUntypedPointerHandle<>& Handle);
	static void ReleaseHandleAndClear(BSUntypedPointerHandle<>& Handle);
	static void CheckForLeaks();
	static void ClearActiveHandles();

	static bool sub_141293870(BSUntypedPointerHandle<>& Handle, __int64 a2);
	// Identical to above except for the Handle.Clear();
	static bool sub_1412E25B0(BSUntypedPointerHandle<>& Handle, __int64 a2);
	static bool sub_1414C52B0(BSUntypedPointerHandle<>& Handle);
};