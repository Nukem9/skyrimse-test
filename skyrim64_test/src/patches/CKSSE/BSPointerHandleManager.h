#pragma once

#include "../TES/NiMain/NiPointer.h"
#include "../TES/BSReadWriteLock.h"
#include "TESForm_CK.h"

template<typename _Ty, int IndexBits = 20, int AgeCountBits = 6>
class IBSUntypedPointerHandle
{
public:
	//
	// NOTE: Handle index bits increased from 20 (vanilla) to 21 (limit doubled) or to 23 (limit doubled)
	//
	// 31       27       26    20             0
	// |--------|--------|-----|--------------|
	// | Unused | Active | Age | Handle Index |
	// |--------|--------|-----|--------------|
	//
	constexpr static _Ty INDEX_BITS = IndexBits;
	constexpr static _Ty AGE_BITS = AgeCountBits;
	constexpr static _Ty UNUSED_BIT_START = INDEX_BITS + AGE_BITS;							// 26 in vanilla

	constexpr static _Ty INDEX_MASK = (((_Ty)1) << INDEX_BITS) - ((_Ty)1);					// 0x00FFFFF
	constexpr static _Ty AGE_MASK = ((((_Ty)1) << AGE_BITS) - ((_Ty)1)) << INDEX_BITS;		// 0x3F00000
	constexpr static _Ty ACTIVE_BIT_MASK = ((_Ty)1) << UNUSED_BIT_START;					// 0x4000000

	constexpr static _Ty MAX_HANDLE_COUNT = ((_Ty)1) << INDEX_BITS;

protected:
	_Ty m_Bits;
public:
	inline void SetBitwiseNull() { m_Bits = 0; }
	inline bool IsBitwiseNull() const { return !m_Bits; }
	inline _Ty GetIndex() const { return m_Bits & INDEX_MASK; }
	inline _Ty GetAge() const { return m_Bits & AGE_MASK; }
	inline void SetInUse() { m_Bits |= ACTIVE_BIT_MASK; }
	inline void SetNotInUse() { m_Bits &= ~ACTIVE_BIT_MASK; }
	inline bool IsInUse() const { return (m_Bits & ACTIVE_BIT_MASK) != 0; }
	inline void IncrementAge() {
		m_Bits = ((m_Bits << INDEX_BITS) & AGE_MASK) | (m_Bits & ~AGE_MASK);
	}

	void Set(_Ty Index, _Ty Age)
	{
		AssertMsg(Index < MAX_HANDLE_COUNT, "BSUntypedPointerHandle::Set - parameter Index is too large");

		m_Bits = Index | Age;
	}

	void SetIndex(_Ty Index)
	{
		AssertMsg(Index < MAX_HANDLE_COUNT, "BSUntypedPointerHandle::Set - parameter Index is too large");

		m_Bits = (Index & INDEX_MASK) | (m_Bits & ~INDEX_MASK);
	}
	

	IBSUntypedPointerHandle& operator=(const IBSUntypedPointerHandle& Other)
	{
		m_Bits = Other.m_Bits;
		return *this;
	}

	bool operator==(const IBSUntypedPointerHandle& Other) const
	{
		return m_Bits == Other.m_Bits;
	}

	bool operator!=(const IBSUntypedPointerHandle& Other) const
	{
		return m_Bits != Other.m_Bits;
	}

	IBSUntypedPointerHandle() : m_Bits(0) {}
};

#if SKYRIM64_USE_64BIT_REFOBJS
typedef IBSUntypedPointerHandle<uint32_t, 25, 6> BSUntypedPointerHandle;
static_assert(sizeof(BSUntypedPointerHandle) == 0x4);
#elif SKYRIM64_PRE_HANDLE32_REF
typedef IBSUntypedPointerHandle<uint32_t, 23, 6> BSUntypedPointerHandle;
static_assert(sizeof(BSUntypedPointerHandle) == 0x4);
#else
typedef IBSUntypedPointerHandle<uint32_t, 21, 6> BSUntypedPointerHandle;
static_assert(sizeof(BSUntypedPointerHandle) == 0x4);
#endif

template<typename ObjectType, typename HandleType = BSUntypedPointerHandle>
class BSPointerHandle : public HandleType
{
};

static_assert(sizeof(BSPointerHandle<TESObjectREFR_CK>) == 0x4);

template<typename _Ty, typename HandleType = BSUntypedPointerHandle>
class IBSPointerHandleManager
{
protected:
	class Entry : public BSUntypedPointerHandle
	{
	private:
		NiPointer<BSHandleRefObject> m_Pointer;
	public:
		void SetNextFreeEntry(_Ty Index)
		{
			m_Bits = (Index & HandleType::INDEX_MASK) | (m_Bits & ~HandleType::INDEX_MASK);
		}

		_Ty GetNextFreeEntry() const
		{
			return m_Bits & HandleType::INDEX_MASK;
		}

		void SetPointer(BSHandleRefObject* Pointer)
		{
			m_Pointer = Pointer;
		}

		BSHandleRefObject* GetPointer() const
		{
			return m_Pointer.operator BSHandleRefObject * ();
		}

		bool IsValid(_Ty Age) const
		{
			return IsInUse() && GetAge() == Age;
		}
	};
	static_assert(sizeof(Entry) == 0x10);

	inline static _Ty FreeListHead;
	inline static _Ty FreeListTail;
	inline static BSReadWriteLock HandleManagerLock;
	inline static std::vector<Entry> HandleEntries;
	inline const static HandleType NullHandle;
public:
	constexpr static _Ty INVALID_INDEX = (_Ty)-1;
public:
	inline static _Ty GetHead() { return FreeListHead; }
	inline static _Ty GetTail() { return FreeListTail; }

	static void InitSDM()
	{
		FreeListHead = 0;
		HandleEntries.resize(HandleType::MAX_HANDLE_COUNT);

		for (_Ty i = 0; i < HandleType::MAX_HANDLE_COUNT; i++)
		{
			if ((i + 1) >= HandleType::MAX_HANDLE_COUNT)
				HandleEntries[i].SetNextFreeEntry(i);
			else
				HandleEntries[i].SetNextFreeEntry(i + 1);
		}

		FreeListTail = HandleType::MAX_HANDLE_COUNT - 1;
	}
	static void KillSDM()
	{
		HandleManagerLock.LockForWrite();

		for (_Ty i = 0; i < HandleType::MAX_HANDLE_COUNT; i++)
		{
			auto& arrayHandle = HandleEntries[i];

			if (!arrayHandle.IsInUse())
				continue;

			if (arrayHandle.GetPointer())
				arrayHandle.GetPointer()->ClearHandleEntryIndex();

			arrayHandle.SetPointer(nullptr);
			arrayHandle.SetNotInUse();

			if (FreeListTail == INVALID_INDEX)
				FreeListHead = i;
			else
				HandleEntries[FreeListTail].SetNextFreeEntry(i);

			arrayHandle.SetNextFreeEntry(i);
			FreeListTail = i;
		}

		HandleManagerLock.UnlockWrite();
	}
};

#if SKYRIM64_USE_64BIT_REFOBJS
typedef IBSPointerHandleManager<uint64_t> BSPointerHandleManager;
#else
typedef IBSPointerHandleManager<uint32_t> BSPointerHandleManager;
#endif

class HandleManager : public BSPointerHandleManager
{
public:
	static void KillSDM();
	static void WarnForUndestroyedHandles();
};

template<typename ObjectType = TESObjectREFR_CK, typename Manager = HandleManager>
class BSPointerHandleManagerInterface : public Manager
{
public:
	static BSUntypedPointerHandle GetCurrentHandle(ObjectType *Refr);
	static BSUntypedPointerHandle CreateHandle(ObjectType *Refr);
	static void Destroy1(const BSUntypedPointerHandle& Handle);
	static void Destroy2(BSUntypedPointerHandle& Handle);
	static bool GetSmartPointer1(const BSUntypedPointerHandle& Handle, NiPointer<ObjectType>& Out);
	static bool GetSmartPointer2(BSUntypedPointerHandle& Handle, NiPointer<ObjectType>& Out);
	static bool IsValid(const BSUntypedPointerHandle& Handle);
};