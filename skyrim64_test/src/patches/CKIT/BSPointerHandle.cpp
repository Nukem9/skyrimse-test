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

void BSPointerHandleManagerInterface::Initialize()
{
	NextPointerHandleIndex = 0;
	HandleTable.resize(MAX_HANDLE_COUNT);

	for (uint32_t i = 0; i < MAX_HANDLE_COUNT; i++)
	{
		if ((i + 1) >= MAX_HANDLE_COUNT)
			HandleTable[i].SetIndex(i);
		else
			HandleTable[i].SetIndex(i + 1);
	}

	LastPointerHandleIndex = MAX_HANDLE_COUNT - 1;
}

BSUntypedPointerHandle<> BSPointerHandleManagerInterface::GetCurrentHandle(TESObjectREFR *Refr)
{
	BSUntypedPointerHandle<> untypedHandle;

	if (Refr && Refr->IsHandleActive())
	{
		auto& handle = HandleTable[Refr->GetHandleIndex()];
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
	HandleTableLock.LockForWrite();
	{
		untypedHandle = GetCurrentHandle(Refr);

		if (untypedHandle == EmptyHandle)
		{
			if (NextPointerHandleIndex == 0xFFFFFFFF)
			{
				untypedHandle.Clear();
				AssertMsgVa(false, "OUT OF HANDLE ARRAY ENTRIES. Null handle created for pointer 0x%p.", Refr);
			}
			else
			{
				auto& newHandle = HandleTable[NextPointerHandleIndex];
				newHandle.ResetAge();
				newHandle.SetActive();

				untypedHandle.Set(newHandle.GetIndex(), newHandle.GetRawAge());
				newHandle.SetPtr(Refr);

				Refr->AssignHandle(NextPointerHandleIndex);
				Assert(Refr->GetHandleIndex() == NextPointerHandleIndex);

				if (newHandle.GetIndex() == NextPointerHandleIndex)
				{
					// Table reached the maximum count
					Assert(NextPointerHandleIndex == LastPointerHandleIndex);

					NextPointerHandleIndex = 0xFFFFFFFF;
					LastPointerHandleIndex = 0xFFFFFFFF;
				}
				else
				{
					Assert(NextPointerHandleIndex != LastPointerHandleIndex);
					NextPointerHandleIndex = newHandle.GetIndex();
				}
			}
		}
	}
	HandleTableLock.UnlockWrite();

	return untypedHandle;
}

void BSPointerHandleManagerInterface::ReleaseHandle(const BSUntypedPointerHandle<>& Handle)
{
	if (Handle.IsEmpty())
		return;

	HandleTableLock.LockForWrite();
	{
		const uint32_t handleIndex = Handle.GetIndex();
		auto& arrayHandle = HandleTable[handleIndex];

		if (arrayHandle.AgeMatches(Handle.GetRawAge()))
		{
			arrayHandle.ClearActive();
			arrayHandle.GetPtr()->ClearHandle();
			arrayHandle.SetPtr(nullptr);

			if (LastPointerHandleIndex == 0xFFFFFFFF)
				NextPointerHandleIndex = handleIndex;
			else
				HandleTable[LastPointerHandleIndex].SetIndex(handleIndex);

			arrayHandle.SetIndex(handleIndex);
			LastPointerHandleIndex = handleIndex;
		}

	}
	HandleTableLock.UnlockWrite();
}

void BSPointerHandleManagerInterface::ReleaseHandleAndClear(BSUntypedPointerHandle<>& Handle)
{
	if (Handle.IsEmpty())
		return;

	HandleTableLock.LockForWrite();
	{
		const uint32_t handleIndex = Handle.GetIndex();
		auto& arrayHandle = HandleTable[handleIndex];

		if (arrayHandle.AgeMatches(Handle.GetRawAge()))
		{
			// I don't know why these functions are in a different order than ReleaseHandle()
			arrayHandle.GetPtr()->ClearHandle();
			arrayHandle.SetPtr(nullptr);
			arrayHandle.ClearActive();

			if (LastPointerHandleIndex == 0xFFFFFFFF)
				NextPointerHandleIndex = handleIndex;
			else
				HandleTable[LastPointerHandleIndex].SetIndex(handleIndex);

			arrayHandle.SetIndex(handleIndex);
			LastPointerHandleIndex = handleIndex;
		}

		Handle.Clear();
	}
	HandleTableLock.UnlockWrite();
}

void BSPointerHandleManagerInterface::CheckForLeaks()
{
	AssertMsg(false, "Unimplemented");
}

void BSPointerHandleManagerInterface::ClearActiveHandles()
{
	for (uint32_t i = 0; i < MAX_HANDLE_COUNT; i++)
	{
		auto& arrayHandle = HandleTable[i];

		if (!arrayHandle.IsActive())
			continue;

		arrayHandle.GetPtr()->ClearHandle();
		arrayHandle.SetPtr(nullptr);
		arrayHandle.ClearActive();

		if (LastPointerHandleIndex == 0xFFFFFFFF)
			NextPointerHandleIndex = i;
		else
			HandleTable[LastPointerHandleIndex].SetIndex(i);

		arrayHandle.SetIndex(i);
		LastPointerHandleIndex = i;
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
	auto& arrayHandle = HandleTable[handleIndex];

	// Possible nullptr deref hazard
	Out = static_cast<TESObjectREFR *>(arrayHandle.GetPtr());

	if (!arrayHandle.AgeMatches(Handle.GetRawAge()) || Out->GetHandleIndex() != handleIndex)
		Out = nullptr;

	return Out != nullptr;
}

bool BSPointerHandleManagerInterface::sub_1412E25B0(BSUntypedPointerHandle<>& Handle, NiPointer<TESObjectREFR>& Out)
{
	// Identical to sub_141293870 except for the Handle.Clear();
	if (Handle.IsEmpty())
	{
		Out = nullptr;
		return false;
	}

	const uint32_t handleIndex = Handle.GetIndex();
	auto& arrayHandle = HandleTable[handleIndex];

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
	auto& arrayHandle = HandleTable[handleIndex];

	Handle.IsEmpty();// This if() got optimized out or something?...

	if (arrayHandle.AgeMatches(Handle.GetRawAge()))
		return arrayHandle.GetPtr()->GetHandleIndex() == handleIndex;

	return false;
}
