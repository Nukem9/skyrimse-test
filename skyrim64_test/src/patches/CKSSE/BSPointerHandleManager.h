#pragma once

#include "../TES/NiMain/NiPointer.h"
#include "../TES/BSReadWriteLock.h"
#include "TESForm_CK.h"

template<int IndexBits = 21, int AgeCountBits = 6>
class BSUntypedPointerHandle
{
public:
	//
	// NOTE: Handle index bits increased from 20 (vanilla) to 21 (limit doubled)
	//
	// 31       28       27    21             0
	// |--------|--------|-----|--------------|
	// | Unused | Active | Age | Handle Index |
	// |--------|--------|-----|--------------|
	//
	constexpr static uint32_t INDEX_BITS		= IndexBits;
	constexpr static uint32_t AGE_BITS			= AgeCountBits;
	constexpr static uint32_t UNUSED_BIT_START	= INDEX_BITS + AGE_BITS;				// 26 in vanilla

	constexpr static uint32_t INDEX_MASK		= (1u << INDEX_BITS) - 1u;				// 0x00FFFFF
	constexpr static uint32_t AGE_MASK			= ((1u << AGE_BITS) - 1u) << INDEX_BITS;// 0x3F00000
	constexpr static uint32_t ACTIVE_BIT_MASK	= 1u << UNUSED_BIT_START;				// 0x4000000

	constexpr static uint32_t MAX_HANDLE_COUNT	= 1u << INDEX_BITS;

private:
	uint32_t m_Bits = 0;

public:
	void SetBitwiseNull()
	{
		m_Bits = 0;
	}

	bool IsBitwiseNull() const
	{
		return m_Bits == 0;
	}

	void Set(uint32_t Index, uint32_t Age)
	{
		AssertMsg(Index < MAX_HANDLE_COUNT, "BSUntypedPointerHandle::Set - parameter Index is too large");

		m_Bits = Index | Age;
	}

	uint32_t QIndex() const
	{
		return m_Bits & INDEX_MASK;
	}

	uint32_t QAge() const
	{
		return m_Bits & AGE_MASK;
	}

	BSUntypedPointerHandle& operator=(const BSUntypedPointerHandle<>& Other)
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
class BSPointerHandle : public HandleType
{
};
static_assert(sizeof(BSPointerHandle<TESObjectREFR_CK>) == 0x4);

template<typename HandleType = BSUntypedPointerHandle<>>
class BSPointerHandleManager
{
protected:
	class Entry
	{
	private:
		uint32_t m_EntryBits = 0;
		NiPointer<BSHandleRefObject> m_Pointer;

	public:
		void SetInUse()
		{
			m_EntryBits |= HandleType::ACTIVE_BIT_MASK;
		}

		void SetNotInUse()
		{
			m_EntryBits &= ~HandleType::ACTIVE_BIT_MASK;
		}

		bool IsInUse() const
		{
			return (m_EntryBits & HandleType::ACTIVE_BIT_MASK) != 0;
		}

		void SetNextFreeEntry(uint32_t Index)
		{
			m_EntryBits = (Index & HandleType::INDEX_MASK) | (m_EntryBits & ~HandleType::INDEX_MASK);
		}

		uint32_t QNextFreeEntry() const
		{
			return m_EntryBits & HandleType::INDEX_MASK;
		}

		uint32_t QAge() const
		{
			return m_EntryBits & HandleType::AGE_MASK;
		}

		void SetPointer(NiPointer<BSHandleRefObject> Pointer)
		{
			m_Pointer = Pointer;
		}

		BSHandleRefObject *GetPointer() const
		{
			return m_Pointer.operator BSHandleRefObject *();
		}

		bool IsValid(uint32_t Age) const
		{
			return IsInUse() && QAge() == Age;
		}

		void IncrementAge()
		{
			m_EntryBits = ((m_EntryBits << HandleType::INDEX_BITS) & HandleType::AGE_MASK) | (m_EntryBits & ~HandleType::AGE_MASK);
		}
	};
	static_assert(sizeof(Entry) == 0x10);

	inline static uint32_t FreeListHead;
	inline static uint32_t FreeListTail;
	inline static BSReadWriteLock HandleManagerLock;
	inline static std::vector<Entry> HandleEntries;
	inline const static BSUntypedPointerHandle<> NullHandle;

public:
	static void InitSDM();
	static void KillSDM();
};

class HandleManager : public BSPointerHandleManager<>
{
public:
	static void KillSDM();
	static void WarnForUndestroyedHandles();
};

template<typename ObjectType = TESObjectREFR_CK, typename Manager = HandleManager>
class BSPointerHandleManagerInterface : public Manager
{
public:
	static BSUntypedPointerHandle<> GetCurrentHandle(ObjectType *Refr);
	static BSUntypedPointerHandle<> CreateHandle(ObjectType *Refr);
	static void Destroy1(const BSUntypedPointerHandle<>& Handle);
	static void Destroy2(BSUntypedPointerHandle<>& Handle);
	static bool GetSmartPointer1(const BSUntypedPointerHandle<>& Handle, NiPointer<ObjectType>& Out);
	static bool GetSmartPointer2(BSUntypedPointerHandle<>& Handle, NiPointer<ObjectType>& Out);
	static bool IsValid(const BSUntypedPointerHandle<>& Handle);
};