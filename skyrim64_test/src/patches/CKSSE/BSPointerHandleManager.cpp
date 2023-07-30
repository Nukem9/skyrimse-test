#include "../../common.h"
#include "BSPointerHandleManager.h"

template class BSPointerHandleManagerInterface<>;

void HandleManager::KillSDM()
{
	BSPointerHandleManager::KillSDM();
}

void HandleManager::WarnForUndestroyedHandles()
{
	AssertMsg(false, "Unimplemented");
}

template<typename ObjectType, typename Manager>
BSUntypedPointerHandle BSPointerHandleManagerInterface<ObjectType, Manager>::GetCurrentHandle(ObjectType *Refr)
{
	BSUntypedPointerHandle untypedHandle;

	if (Refr && Refr->IsHandleValid())
	{
		auto& handle = Manager::HandleEntries[Refr->GetHandleEntryIndex()];
		untypedHandle.Set(Refr->GetHandleEntryIndex(), handle.GetAge());

		AssertMsg(untypedHandle.GetAge() == handle.GetAge(), 
			"BSPointerHandleManagerInterface::GetCurrentHandle - Handle already exists but has wrong age!");
	}

	return untypedHandle;
}

template<typename ObjectType, typename Manager>
BSUntypedPointerHandle BSPointerHandleManagerInterface<ObjectType, Manager>::CreateHandle(ObjectType *Refr)
{
	BSUntypedPointerHandle untypedHandle;

	if (!Refr)
		return untypedHandle;

	// Shortcut: Check if the handle is already valid
	untypedHandle = GetCurrentHandle(Refr);

	if (untypedHandle != Manager::NullHandle)
		return untypedHandle;

	// Wasn't present. Acquire lock and add it (unless someone else inserted it in the meantime)
	Manager::HandleManagerLock.LockForWrite();
	{
		untypedHandle = GetCurrentHandle(Refr);

		if (untypedHandle == Manager::NullHandle)
		{
			if (Manager::FreeListHead == Manager::INVALID_INDEX)
			{
				untypedHandle.SetBitwiseNull();
				AssertMsgVa(false, "OUT OF HANDLE ARRAY ENTRIES. Null handle created for pointer 0x%p.", Refr);
			}
			else
			{
				auto& newHandle = Manager::HandleEntries[Manager::FreeListHead];
				newHandle.IncrementAge();
				newHandle.SetInUse();

				untypedHandle.Set(newHandle.GetNextFreeEntry(), newHandle.GetAge());
				newHandle.SetPointer(Refr);

				Refr->SetHandleEntryIndex(Manager::FreeListHead);
				Assert(Refr->GetHandleEntryIndex() == Manager::FreeListHead);

				if (newHandle.GetNextFreeEntry() == Manager::FreeListHead)
				{
					// Table reached the maximum count
					Assert(Manager::FreeListHead == Manager::FreeListTail);

					Manager::FreeListHead = Manager::INVALID_INDEX;
					Manager::FreeListTail = Manager::INVALID_INDEX;
				}
				else
				{
					Assert(Manager::FreeListHead != Manager::FreeListTail);
					Manager::FreeListHead = newHandle.GetNextFreeEntry();
				}
			}
		}
	}
	Manager::HandleManagerLock.UnlockWrite();

	return untypedHandle;
}

template<typename ObjectType, typename Manager>
void BSPointerHandleManagerInterface<ObjectType, Manager>::Destroy1(const BSUntypedPointerHandle& Handle)
{
	if (Handle.IsBitwiseNull())
		return;

	Manager::HandleManagerLock.LockForWrite();
	{
		const auto handleIndex = Handle.GetIndex();
		auto& arrayHandle = Manager::HandleEntries[handleIndex];

		if (arrayHandle.IsValid(Handle.GetAge()))
		{
			arrayHandle.GetPointer()->ClearHandleEntryIndex();
			arrayHandle.SetPointer(nullptr);
			arrayHandle.SetNotInUse();

			if (Manager::FreeListTail == Manager::INVALID_INDEX)
				Manager::FreeListHead = handleIndex;
			else
				Manager::HandleEntries[Manager::FreeListTail].SetNextFreeEntry(handleIndex);

			arrayHandle.SetNextFreeEntry(handleIndex);
			Manager::FreeListTail = handleIndex;
		}
	}
	Manager::HandleManagerLock.UnlockWrite();
}

template<typename ObjectType, typename Manager>
void BSPointerHandleManagerInterface<ObjectType, Manager>::Destroy2(BSUntypedPointerHandle& Handle)
{
	if (Handle.IsBitwiseNull())
		return;

	Manager::HandleManagerLock.LockForWrite();
	{
		const auto handleIndex = Handle.GetIndex();
		auto& arrayHandle = Manager::HandleEntries[handleIndex];

		if (arrayHandle.IsValid(Handle.GetAge()))
		{
			arrayHandle.GetPointer()->ClearHandleEntryIndex();
			arrayHandle.SetPointer(nullptr);
			arrayHandle.SetNotInUse();

			if (Manager::FreeListTail == Manager::INVALID_INDEX)
				Manager::FreeListHead = handleIndex;
			else
				Manager::HandleEntries[Manager::FreeListTail].SetNextFreeEntry(handleIndex);

			arrayHandle.SetNextFreeEntry(handleIndex);
			Manager::FreeListTail = handleIndex;
		}

		// Identical to Destroy1 except for this Handle.SetBitwiseNull();
		Handle.SetBitwiseNull();
	}
	Manager::HandleManagerLock.UnlockWrite();
}

template<typename ObjectType, typename Manager>
bool BSPointerHandleManagerInterface<ObjectType, Manager>::GetSmartPointer1(const BSUntypedPointerHandle& Handle, NiPointer<ObjectType>& Out)
{
	if (Handle.IsBitwiseNull())
	{
		Out = nullptr;
		return false;
	}

	const auto handleIndex = Handle.GetIndex();
	auto& arrayHandle = Manager::HandleEntries[handleIndex];

	Out = static_cast<TESObjectREFR_CK*>(arrayHandle.GetPointer());

	if (!arrayHandle.IsValid(Handle.GetAge()) || Out->GetHandleEntryIndex() != handleIndex)
		Out = nullptr;

	return Out != nullptr;
}

template<typename ObjectType, typename Manager>
bool BSPointerHandleManagerInterface<ObjectType, Manager>::GetSmartPointer2(BSUntypedPointerHandle& Handle, NiPointer<ObjectType>& Out)
{
	if (Handle.IsBitwiseNull())
	{
		Out = nullptr;
		return false;
	}

	const auto handleIndex = Handle.GetIndex();
	auto& arrayHandle = Manager::HandleEntries[handleIndex];

	Out = static_cast<TESObjectREFR_CK*>(arrayHandle.GetPointer());

//#if !SKYRIM64_USE_64BIT_REFOBJS
	if (!arrayHandle.IsValid(Handle.GetAge()) || Out->GetHandleEntryIndex() != handleIndex)
	{
		// Identical to GetSmartPointer1 except for this Handle.SetBitwiseNull();
		Handle.SetBitwiseNull();
		Out = nullptr;
	}
//#endif

	return Out != nullptr;
}

template<typename ObjectType, typename Manager>
bool BSPointerHandleManagerInterface<ObjectType, Manager>::IsValid(const BSUntypedPointerHandle& Handle)
{
	const auto handleIndex = Handle.GetIndex();
	auto& arrayHandle = Manager::HandleEntries[handleIndex];

	// Handle.IsBitwiseNull(); -- This if() is optimized away because the result is irrelevant

	if (!arrayHandle.IsValid(Handle.GetAge()))
		return false;

	return arrayHandle.GetPointer()->GetHandleEntryIndex() == handleIndex;
}