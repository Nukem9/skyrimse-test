#include "../../common.h"
#include "BSPointerHandle.h"

class TempTESForm
{
public:
	virtual ~TempTESForm();

	char _data[0x20];
};

class TESChildCell
{
public:
	virtual ~TESChildCell();
};

class TESObjectREFR : public TempTESForm, public TESChildCell, public BSHandleRefObject
{
public:
	virtual ~TESObjectREFR();
	virtual void OtherTestFunction2();
};

static_assert_offset(TESObjectREFR, m_uiRefCount, 0x38);

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

	if (Refr && Refr->IsHandleActive())
	{
		auto& handle = m_HandleTable[Refr->GetHandleIndex()];
		untypedHandle.Set(Refr->GetHandleIndex(), handle.GetRawAge());

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

				untypedHandle.Set(newHandle.GetIndex(), newHandle.GetRawAge());
				newHandle.SetPtr(Refr);

				Refr->AssignHandle(g_NextPointerHandleIndex);
				Assert(Refr->GetHandleIndex() == g_NextPointerHandleIndex);

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
			arrayHandle.GetPtr()->ClearHandle();
			arrayHandle.SetPtr(nullptr);

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
			arrayHandle.GetPtr()->ClearHandle();
			arrayHandle.SetPtr(nullptr);
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

		arrayHandle.GetPtr()->ClearHandle();
		arrayHandle.SetPtr(nullptr);
		arrayHandle.ClearActive();

		if (g_LastPointerHandleIndex == 0xFFFFFFFF)
			g_NextPointerHandleIndex = i;
		else
			m_HandleTable[g_LastPointerHandleIndex].SetIndex(i);

		arrayHandle.SetIndex(i);
		g_LastPointerHandleIndex = i;
	}
}

bool BSPointerHandleManagerInterface::sub_141293870(const BSUntypedPointerHandle<>& Handle, NiPointer<TESObjectREFR>& Out)
{
	if (Handle.IsEmpty())
	{
		Out = nullptr;
		return false;
	}

	const uint32_t handleIndex = Handle.GetIndex();
	auto& arrayHandle = m_HandleTable[handleIndex];

	// Possible nullptr deref hazard
	Out = static_cast<TESObjectREFR *>(arrayHandle.GetPtr());

	if (!arrayHandle.AgeMatches(Handle.GetRawAge()) || Out->GetHandleIndex() != handleIndex)
		Out = nullptr;

	return Out != nullptr;
}

bool BSPointerHandleManagerInterface::sub_1412E25B0(BSUntypedPointerHandle<>& Handle, NiPointer<TESObjectREFR>& Out)
{
	if (Handle.IsEmpty())
	{
		Out = nullptr;
		return false;
	}

	const uint32_t handleIndex = Handle.GetIndex();
	auto& arrayHandle = m_HandleTable[handleIndex];

	// Possible nullptr deref hazard
	Out = static_cast<TESObjectREFR *>(arrayHandle.GetPtr());

	if (!arrayHandle.AgeMatches(Handle.GetRawAge()) || Out->GetHandleIndex() != handleIndex)
	{
		Handle.Clear();
		Out = nullptr;
	}

	return Out != nullptr;
}

bool BSPointerHandleManagerInterface::sub_1414C52B0(const BSUntypedPointerHandle<>& Handle)
{
	const uint32_t handleIndex = Handle.GetIndex();
	auto& arrayHandle = m_HandleTable[handleIndex];

	Handle.IsEmpty();// This if() got optimized out or something?...

	if (arrayHandle.AgeMatches(Handle.GetRawAge()))
		return arrayHandle.GetPtr()->GetHandleIndex() == handleIndex;

	return false;
}
