#include "../../common.h"
#include "BSPointerHandleManager.h"

template class BSPointerHandleManager<>;
template class BSPointerHandleManagerInterface<>;

template<typename HandleType>
void BSPointerHandleManager<HandleType>::InitSDM()
{
	FreeListHead = 0;
	HandleEntries.resize(HandleType::MAX_HANDLE_COUNT);

	for (uint32_t i = 0; i < HandleType::MAX_HANDLE_COUNT; i++)
	{
		if ((i + 1) >= HandleType::MAX_HANDLE_COUNT)
			HandleEntries[i].SetNextFreeEntry(i);
		else
			HandleEntries[i].SetNextFreeEntry(i + 1);
	}

	FreeListTail = HandleType::MAX_HANDLE_COUNT - 1;
}

template<typename HandleType>
void BSPointerHandleManager<HandleType>::KillSDM()
{
	HandleManagerLock.LockForWrite();

	for (uint32_t i = 0; i < HandleType::MAX_HANDLE_COUNT; i++)
	{
		auto& arrayHandle = HandleEntries[i];

		if (!arrayHandle.IsInUse())
			continue;

		if (arrayHandle.GetPointer())
			arrayHandle.GetPointer()->ClearHandleEntryIndex();

		arrayHandle.SetPointer(nullptr);
		arrayHandle.SetNotInUse();

		if (FreeListTail == 0xFFFFFFFF)
			FreeListHead = i;
		else
			HandleEntries[FreeListTail].SetNextFreeEntry(i);

		arrayHandle.SetNextFreeEntry(i);
		FreeListTail = i;
	}

	HandleManagerLock.UnlockWrite();
}

void HandleManager::KillSDM()
{
	BSPointerHandleManager::KillSDM();
}

void HandleManager::WarnForUndestroyedHandles()
{
	AssertMsg(false, "Unimplemented");
}

template<typename ObjectType, typename Manager>
inline BSUntypedPointerHandle<> BSPointerHandleManagerInterface<ObjectType, Manager>::GetCurrentHandle(ObjectType * Refr)
{
	BSUntypedPointerHandle<> untypedHandle;

	if (Refr && Refr->IsHandleValid())
	{
		auto& handle = HandleEntries[Refr->QHandleEntryIndex()];
		untypedHandle.Set(Refr->QHandleEntryIndex(), handle.QAge());

		AssertMsg(untypedHandle.QAge() == handle.QAge(), "BSPointerHandleManagerInterface::GetCurrentHandle - Handle already exists but has wrong age!");
	}

	return untypedHandle;
}

template<typename ObjectType, typename Manager>
inline BSUntypedPointerHandle<> BSPointerHandleManagerInterface<ObjectType, Manager>::CreateHandle(ObjectType * Refr)
{
	BSUntypedPointerHandle<> untypedHandle;

	if (!Refr)
		return untypedHandle;

	// Shortcut: Check if the handle is already valid
	untypedHandle = GetCurrentHandle(Refr);

	if (untypedHandle != NullHandle)
		return untypedHandle;

	// Wasn't present. Acquire lock and add it (unless someone else inserted it in the meantime)
	HandleManagerLock.LockForWrite();
	{
		untypedHandle = GetCurrentHandle(Refr);

		if (untypedHandle == NullHandle)
		{
			if (FreeListHead == 0xFFFFFFFF)
			{
				untypedHandle.SetBitwiseNull();
				AssertMsgVa(false, "OUT OF HANDLE ARRAY ENTRIES. Null handle created for pointer 0x%p.", Refr);
			}
			else
			{
				auto& newHandle = HandleEntries[FreeListHead];
				newHandle.IncrementAge();
				newHandle.SetInUse();

				untypedHandle.Set(newHandle.QNextFreeEntry(), newHandle.QAge());
				newHandle.SetPointer(Refr);

				Refr->SetHandleEntryIndex(FreeListHead);
				Assert(Refr->QHandleEntryIndex() == FreeListHead);

				if (newHandle.QNextFreeEntry() == FreeListHead)
				{
					// Table reached the maximum count
					Assert(FreeListHead == FreeListTail);

					FreeListHead = 0xFFFFFFFF;
					FreeListTail = 0xFFFFFFFF;
				}
				else
				{
					Assert(FreeListHead != FreeListTail);
					FreeListHead = newHandle.QNextFreeEntry();
				}
			}
		}
	}
	HandleManagerLock.UnlockWrite();

	return untypedHandle;
}

template<typename ObjectType, typename Manager>
inline void BSPointerHandleManagerInterface<ObjectType, Manager>::Destroy1(const BSUntypedPointerHandle<>& Handle)
{
	if (Handle.IsBitwiseNull())
		return;

	HandleManagerLock.LockForWrite();
	{
		const uint32_t handleIndex = Handle.QIndex();
		auto& arrayHandle = HandleEntries[handleIndex];

		if (arrayHandle.IsValid(Handle.QAge()))
		{
			arrayHandle.GetPointer()->ClearHandleEntryIndex();
			arrayHandle.SetPointer(nullptr);
			arrayHandle.SetNotInUse();

			if (FreeListTail == 0xFFFFFFFF)
				FreeListHead = handleIndex;
			else
				HandleEntries[FreeListTail].SetNextFreeEntry(handleIndex);

			arrayHandle.SetNextFreeEntry(handleIndex);
			FreeListTail = handleIndex;
		}
	}
	HandleManagerLock.UnlockWrite();
}

template<typename ObjectType, typename Manager>
inline void BSPointerHandleManagerInterface<ObjectType, Manager>::Destroy2(BSUntypedPointerHandle<>& Handle)
{
	if (Handle.IsBitwiseNull())
		return;

	HandleManagerLock.LockForWrite();
	{
		const uint32_t handleIndex = Handle.QIndex();
		auto& arrayHandle = HandleEntries[handleIndex];

		if (arrayHandle.IsValid(Handle.QAge()))
		{
			arrayHandle.GetPointer()->ClearHandleEntryIndex();
			arrayHandle.SetPointer(nullptr);
			arrayHandle.SetNotInUse();

			if (FreeListTail == 0xFFFFFFFF)
				FreeListHead = handleIndex;
			else
				HandleEntries[FreeListTail].SetNextFreeEntry(handleIndex);

			arrayHandle.SetNextFreeEntry(handleIndex);
			FreeListTail = handleIndex;
		}

		// Identical to Destroy1 except for this Handle.SetBitwiseNull();
		Handle.SetBitwiseNull();
	}
	HandleManagerLock.UnlockWrite();
}

template<typename ObjectType, typename Manager>
inline bool BSPointerHandleManagerInterface<ObjectType, Manager>::GetSmartPointer1(const BSUntypedPointerHandle<>& Handle, NiPointer<ObjectType>& Out)
{
	if (Handle.IsBitwiseNull())
	{
		Out = nullptr;
		return false;
	}

	const uint32_t handleIndex = Handle.QIndex();
	auto& arrayHandle = HandleEntries[handleIndex];

	Out = static_cast<TESObjectREFR_CK *>(arrayHandle.GetPointer());

	if (!arrayHandle.IsValid(Handle.QAge()) || Out->QHandleEntryIndex() != handleIndex)
		Out = nullptr;

	return Out != nullptr;
}

template<typename ObjectType, typename Manager>
inline bool BSPointerHandleManagerInterface<ObjectType, Manager>::GetSmartPointer2(BSUntypedPointerHandle<>& Handle, NiPointer<ObjectType>& Out)
{
	if (Handle.IsBitwiseNull())
	{
		Out = nullptr;
		return false;
	}

	const uint32_t handleIndex = Handle.QIndex();
	auto& arrayHandle = HandleEntries[handleIndex];

	Out = static_cast<TESObjectREFR_CK *>(arrayHandle.GetPointer());

	if (!arrayHandle.IsValid(Handle.QAge()) || Out->QHandleEntryIndex() != handleIndex)
	{
		// Identical to GetSmartPointer1 except for this Handle.SetBitwiseNull();
		Handle.SetBitwiseNull();
		Out = nullptr;
	}

	return Out != nullptr;
}

template<typename ObjectType, typename Manager>
inline bool BSPointerHandleManagerInterface<ObjectType, Manager>::IsValid(const BSUntypedPointerHandle<>& Handle)
{
	const uint32_t handleIndex = Handle.QIndex();
	auto& arrayHandle = HandleEntries[handleIndex];

	// Handle.IsBitwiseNull(); -- This if() is optimized away because the result is irrelevant

	if (!arrayHandle.IsValid(Handle.QAge()))
		return false;

	return arrayHandle.GetPointer()->QHandleEntryIndex() == handleIndex;
}