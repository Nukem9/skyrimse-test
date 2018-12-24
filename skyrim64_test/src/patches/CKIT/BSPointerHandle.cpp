#include "../../common.h"
#include "BSPointerHandle.h"

#define REFR_TO_REF(x) ((BSHandleRefObject *)((uintptr_t)(x) + 0x30))
#define REF_TO_REFR(x) ((TESObjectREFR *)((uintptr_t)(x) - 0x30))

class NiRefObjectA
{
public:
	virtual ~NiRefObjectA();
	virtual void DeleteThis();
};

class BSHandleRefObject : public NiRefObjectA
{
public:
	virtual ~BSHandleRefObject();
	virtual void DeleteThis();

	uint32_t m_uiRefCount;

	BSHandleRefObject() : NiRefObjectA()
	{
		Reset();
	}

	void Reset()
	{
		m_uiRefCount &= 0x3FF;
	}

	void AssignHandleIndex(uint32_t HandleIndex)
	{
		Reset();
		m_uiRefCount |= ((HandleIndex << 11) | 0x400);
	}

	uint32_t GetIndex() const
	{
		return m_uiRefCount >> 11;
	}

	bool GetActive() const
	{
		return (m_uiRefCount & 0x400) != 0;
	}
};

class TESObjectREFR
{
public:
	virtual ~TESObjectREFR();

	char _pad[0x28];
	BSHandleRefObject ref;
};

static_assert(offsetof(TESObjectREFR, ref) == 0x30);

void BSPointerHandleManagerInterface::acquire_lock()
{
}

void BSPointerHandleManagerInterface::release_lock()
{
}

void BSPointerHandleManagerInterface::Initialize()
{
	g_NextPointerHandleIndex = 0;
	memset(m_HandleTable, 0, sizeof(m_HandleTable));

	for (uint32_t i = 0; i < ARRAYSIZE(m_HandleTable); i++)
	{
		if ((i + 1) >= ARRAYSIZE(m_HandleTable))
			m_HandleTable[i].SetIndex(i);
		else
			m_HandleTable[i].SetIndex(i + 1);
	}

	g_LastPointerHandleIndex = ARRAYSIZE(m_HandleTable) - 1;
}

BSUntypedPointerHandle<> BSPointerHandleManagerInterface::GetCurrentHandle(TESObjectREFR *Refr)
{
	BSUntypedPointerHandle<> untypedHandle;

	if (Refr && REFR_TO_REF(Refr)->GetActive())
	{
		auto& handle = m_HandleTable[REFR_TO_REF(Refr)->GetIndex()];
		untypedHandle.SetRaw(REFR_TO_REF(Refr)->GetIndex(), handle.GetRawAge());

		AssertMsg(untypedHandle.GetRawAge() == handle.GetRawAge(), "BSPointerHandleManagerInterface::GetCurrentHandle - Handle already exists but has wrong age!");
	}

	return untypedHandle;
}

BSUntypedPointerHandle<> BSPointerHandleManagerInterface::CreateHandle(TESObjectREFR *Refr)
{
	BSUntypedPointerHandle<> untypedHandle;

	if (!Refr)
		return untypedHandle;

	// Shortcut: Check if the handle is already valid
	untypedHandle = GetCurrentHandle(Refr);

	if (untypedHandle != EmptyHandle)
		return untypedHandle;

	// Wasn't present. Acquire lock and add it (unless someone else inserted it in the meantime)
	acquire_lock();
	{
		untypedHandle = GetCurrentHandle(Refr);

		if (untypedHandle == EmptyHandle)
		{
			if (g_NextPointerHandleIndex == 0xFFFFFFFF)
			{
				untypedHandle.Clear();
				AssertMsgVa(false, "OUT OF HANDLE ARRAY ENTRIES. Null handle created for pointer 0x%p.", Refr);
			}
			else
			{
				auto& newHandle = m_HandleTable[g_NextPointerHandleIndex];
				newHandle.ResetAge();
				newHandle.SetActive();

				untypedHandle.SetRaw(newHandle.GetIndex(), newHandle.GetRawAge());
				newHandle.AssignObject(REFR_TO_REF(Refr));

				REFR_TO_REF(Refr)->AssignHandleIndex(g_NextPointerHandleIndex);
				Assert(REFR_TO_REF(Refr)->GetIndex() == g_NextPointerHandleIndex);

				if (newHandle.GetIndex() == g_NextPointerHandleIndex)
				{
					// Table reached the maximum count
					Assert(g_NextPointerHandleIndex == g_LastPointerHandleIndex);

					g_NextPointerHandleIndex = 0xFFFFFFFF;
					g_LastPointerHandleIndex = 0xFFFFFFFF;
				}
				else
				{
					Assert(g_NextPointerHandleIndex != g_LastPointerHandleIndex);
					g_NextPointerHandleIndex = newHandle.GetIndex();
				}
			}
		}
	}
	release_lock();

	return untypedHandle;
}

void BSPointerHandleManagerInterface::ReleaseHandle(const BSUntypedPointerHandle<>& Handle)
{
	if (Handle.IsEmpty())
		return;

	acquire_lock();
	{
		const uint32_t handleIndex = Handle.GetIndex();
		auto& arrayHandle = m_HandleTable[handleIndex];

		if (arrayHandle.AgeMatches(Handle.GetRawAge()))
		{
			arrayHandle.ClearActive();
			arrayHandle.GetPtr()->Reset();
			arrayHandle.AssignObject(nullptr);

			if (g_LastPointerHandleIndex == 0xFFFFFFFF)
				g_NextPointerHandleIndex = handleIndex;
			else
				m_HandleTable[g_LastPointerHandleIndex].SetIndex(handleIndex);

			arrayHandle.SetIndex(handleIndex);
			g_LastPointerHandleIndex = handleIndex;
		}

	}
	release_lock();
}

void BSPointerHandleManagerInterface::ReleaseHandleAndClear(BSUntypedPointerHandle<>& Handle)
{
	if (Handle.IsEmpty())
		return;

	acquire_lock();
	{
		const uint32_t handleIndex = Handle.GetIndex();
		auto& arrayHandle = m_HandleTable[handleIndex];

		if (arrayHandle.AgeMatches(Handle.GetRawAge()))
		{
			// I don't know why these functions are in a different order than ReleaseHandle()
			arrayHandle.GetPtr()->Reset();
			arrayHandle.AssignObject(nullptr);
			arrayHandle.ClearActive();

			if (g_LastPointerHandleIndex == 0xFFFFFFFF)
				g_NextPointerHandleIndex = handleIndex;
			else
				m_HandleTable[g_LastPointerHandleIndex].SetIndex(handleIndex);

			arrayHandle.SetIndex(handleIndex);
			g_LastPointerHandleIndex = handleIndex;
		}

		Handle.Clear();
	}
	release_lock();
}

void BSPointerHandleManagerInterface::CheckForLeaks()
{
	AssertMsg(false, "Unimplemneted");
}

void BSPointerHandleManagerInterface::ClearActiveHandles()
{
	for (int i = 0; i < ARRAYSIZE(m_HandleTable); i++)
	{
		auto& arrayHandle = m_HandleTable[i];

		if (!arrayHandle.IsActive())
			continue;

		arrayHandle.GetPtr()->Reset();
		arrayHandle.AssignObject(nullptr);
		arrayHandle.ClearActive();

		if (g_LastPointerHandleIndex == 0xFFFFFFFF)
			g_NextPointerHandleIndex = i;
		else
			m_HandleTable[g_LastPointerHandleIndex].SetIndex(i);

		arrayHandle.SetIndex(i);
		g_LastPointerHandleIndex = i;
	}
}

bool BSPointerHandleManagerInterface::sub_141293870(BSUntypedPointerHandle<>& Handle, __int64 a2)
{
	AutoFunc(void(*)(__int64, void *), sub_140FB461D, 0xFB461D);
	AutoFunc(bool(*)(__int64, void *), sub_140FEEAB1, 0xFEEAB1);
	AutoFunc(void *(*)(__int64), sub_140FFFBFE, 0xFFFBFE);

	// a2 is possibly NiPointer<TESObjectREFR>?
	if (Handle.IsEmpty())
	{
		sub_140FB461D(a2, nullptr);
	}
	else
	{
		const uint32_t handleIndex = Handle.GetIndex();
		auto& arrayHandle = m_HandleTable[handleIndex];

		if (arrayHandle.GetPtr())
			sub_140FB461D(a2, REF_TO_REFR(arrayHandle.GetPtr()));
		else
			sub_140FB461D(a2, nullptr);

		void *v2 = sub_140FFFBFE(a2);

		if (!arrayHandle.AgeMatches(Handle.GetRawAge()) || REFR_TO_REF(v2)->GetIndex() != handleIndex)
			sub_140FB461D(a2, nullptr);
	}

	return sub_140FEEAB1(a2, nullptr);
}

bool BSPointerHandleManagerInterface::sub_1412E25B0(BSUntypedPointerHandle<>& Handle, __int64 a2)
{
	AutoFunc(void(*)(__int64, void *), sub_140FB461D, 0xFB461D);
	AutoFunc(bool(*)(__int64, void *), sub_140FEEAB1, 0xFEEAB1);
	AutoFunc(void *(*)(__int64), sub_140FFFBFE, 0xFFFBFE);

	// a2 is possibly NiPointer<TESObjectREFR>?
	if (Handle.IsEmpty())
	{
		sub_140FB461D(a2, nullptr);
	}
	else
	{
		const uint32_t handleIndex = Handle.GetIndex();
		auto& arrayHandle = m_HandleTable[handleIndex];

		if (arrayHandle.GetPtr())
			sub_140FB461D(a2, REF_TO_REFR(arrayHandle.GetPtr()));
		else
			sub_140FB461D(a2, nullptr);

		void *v2 = sub_140FFFBFE(a2);

		if (!arrayHandle.AgeMatches(Handle.GetRawAge()) || REFR_TO_REF(v2)->GetIndex() != handleIndex)
		{
			Handle.Clear();
			sub_140FB461D(a2, nullptr);
		}
	}

	return sub_140FEEAB1(a2, nullptr);
}

bool BSPointerHandleManagerInterface::sub_1414C52B0(BSUntypedPointerHandle<>& Handle)
{
	const uint32_t handleIndex = Handle.GetIndex();
	auto& arrayHandle = m_HandleTable[handleIndex];

	Handle.IsEmpty();// This if() got optimized out or something?...

	if (arrayHandle.AgeMatches(Handle.GetRawAge()))
		return arrayHandle.GetPtr()->GetIndex() == handleIndex;

	return false;
}
