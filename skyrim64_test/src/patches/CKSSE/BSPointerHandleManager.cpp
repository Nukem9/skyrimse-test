#include "../../common.h"
#include "BSPointerHandleManager.h"
#include "LogWindow.h"

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

	if (!arrayHandle.IsValid(Handle.GetAge()) || Out->GetHandleEntryIndex() != handleIndex)
	{
		// Identical to GetSmartPointer1 except for this Handle.SetBitwiseNull();
		Handle.SetBitwiseNull();
		Out = nullptr;
	}

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

void PatchPointerHandleManager()
{
	// Unfortunately, it will not be possible to make an extension, 
	// since there is not an alignment to 0x8 everywhere in memory.

#if 0

	unsigned char szBuf[40];
	size_t all_count = 0, dec_count = 0, inc_count = 0;

	DWORD protect;
	VirtualProtect((void*)g_CodeBase, g_CodeEnd - g_CodeBase, PAGE_READWRITE, &protect);

	// 
	// mov qword ptr ss:[rsp+8],rcx
	// mov rax, qword ptr ss:[rsp + 8]
	// mov ecx, FFFFFFFF
	// lock xadd dword ptr ds:[rax], ecx
	// dec ecx
	// mov eax, ecx
	// ret
	//

	auto patterns = XUtil::FindPatterns(g_CodeBase, g_CodeEnd - g_CodeBase,
		"48 89 4C 24 08 48 8B 44 24 08 B9 FF FF FF FF F0 0F C1 08 FF C9 8B C1 C3");
	all_count += patterns.size();

	for (auto pattern : patterns)
	{
		auto start = (unsigned char*)(pattern);

		start[10] = 0x48;
		start[11] = 0x83;
		start[12] = 0xC9;
		start[14] = 0xF0;
		start[15] = 0x48;

		start[19] = 0x48;
		start[20] = 0xFF;
		start[21] = 0xC9;
		start[22] = 0x48;
		start[23] = 0x89;
		start[24] = 0xC8;
		start[25] = 0xC3;

		dec_count++;
	}

	//
	// mov qword ptr ss:[rsp+8],rcx
	// mov rax, qword ptr ss:[rsp + 8]
	// mov ecx, 1
	// lock xadd dword ptr ds:[rax], ecx
	// inc ecx
	// mov eax, ecx
	// ret
	//

	patterns = XUtil::FindPatterns(g_CodeBase, g_CodeEnd - g_CodeBase,
		"48 89 4C 24 08 48 8B 44 24 08 B9 01 00 00 00 F0 0F C1 08 FF C1 8B C1 C3");
	all_count += patterns.size();

	for (auto pattern : patterns)
	{
		auto start = (unsigned char*)(pattern);

		start[10] = 0x31;
		start[11] = 0xC9;
		start[12] = 0xFF;
		start[13] = 0xC1;

		start[14] = 0xF0;
		start[15] = 0x48;

		start[19] = 0x48;
		start[20] = 0xFF;
		start[21] = 0xC1;
		start[22] = 0x48;
		start[23] = 0x89;
		start[24] = 0xC8;
		start[25] = 0xC3;

		inc_count++;
	}

	// lock xadd dword ptr ds:[rcx+8], e??
	patterns = XUtil::FindPatterns(g_CodeBase, g_CodeEnd - g_CodeBase, "F0 0F C1 ?? 08");
	all_count += patterns.size();

	uintptr_t func01 = OFFSET(0xF9FC80, 16438);
	memcpy((void*)(func01), (void*)"\x48\x83\xC8\xFF\xF0\x48\x0F\xC1\x41\x08\x48\xC3", 12);

	uintptr_t func01_rdx = func01 + 0x10;
	memcpy((void*)(func01_rdx), (void*)"\x48\x83\xC8\xFF\xF0\x48\x0F\xC1\x42\x08\x48\xC3", 12);

	uintptr_t func01_rbx = func01 + 0x20;
	memcpy((void*)(func01_rbx), (void*)"\x48\x83\xC8\xFF\xF0\x48\x0F\xC1\x43\x08\x48\xC3", 12);

	uintptr_t func01_rsi = func01 + 0x30;
	memcpy((void*)(func01_rsi), (void*)"\x48\x83\xC8\xFF\xF0\x48\x0F\xC1\x46\x08\x48\xC3", 12);

	uintptr_t func01_rdi = func01 + 0x40;
	memcpy((void*)(func01_rdi), (void*)"\x48\x83\xC8\xFF\xF0\x48\x0F\xC1\x47\x08\x48\xC3", 12);

	uintptr_t func01_rax = func01 + 0x50;
	memcpy((void*)(func01_rax), (void*)"\x48\x83\xC9\xFF\xF0\x48\x0F\xC1\x48\x08\x48\xC3", 12);

	auto sub_lockdec_rXX_offset_08 = [&func01, &dec_count](unsigned char* start, 
		uintptr_t pattern, const char* before_hexcode, int before_hexcode_size, int nop_size = 13)
	{
		// lock xadd dword ptr ds:[r??+8], e??

		memset(start, 0x90, nop_size);

		auto rip = start;
		auto rel32 = (uint32_t)(func01 - (pattern + 2 + before_hexcode_size));

		*(rip++) = 0x50;	// push rax
		*(rip++) = 0x51;	// push rcx
		memcpy((void*)rip, (void*)before_hexcode, before_hexcode_size);
		rip += before_hexcode_size;
		*(rip++) = 0xE8;	// call
		memcpy(rip, &rel32, sizeof(uint32_t));
		rip += sizeof(uint32_t);
		*(rip++) = 0x59;	// pop rcx
		*(rip++) = 0x58;	// pop rax

		dec_count++;
	};

	auto sub_lockdec_rXX_offset_08_short = [&dec_count](unsigned char* start,
		uintptr_t pattern, uintptr_t func, int num_reg = 0, int nop_size = 8)
	{
		// lock xadd dword ptr ds:[r??+8], e??

		memset(start, 0x90, nop_size);

		auto rip = start;
		auto rel32 = (uint32_t)(func - (pattern + 5));

		*(rip++) = 0x50 + num_reg;	// push rax
		*(rip++) = 0xE8;			// call
		memcpy(rip, &rel32, sizeof(uint32_t));
		rip += sizeof(uint32_t);
		*(rip++) = 0x58 + num_reg;	// pop rax

		dec_count++;
	};

	for (auto pattern : patterns)
	{
		auto start = (unsigned char*)(pattern - 5);

		if ((*(start + 1) == 0xFF) && (*(start + 2) == 0xFF) &&
			(*(start + 3) == 0xFF) && (*(start + 4) == 0xFF))
		{
			// mov e??, FFFFFFFF
			// lock xadd dword ptr ds:[r??+8],e??

			//LogWindow::Log("%X", *(start + 10));

			if ((*(start + 10) == 0x83) && (*(start + 12) == 0x01))
			{
				// Comparison by 1, in "op dec" is used to destroy an object

				// cmp e??, 1

				if ((*(start + 8) == 0x41) || (*(start + 8) == 0x51) || 
					(*(start + 8) == 0x59) || (*(start + 8) == 0x79))
				{
					// lock xadd dword ptr ds:[rcx+8], e??

					memset(start, 0x90, 13);

					auto rel32 = (uint32_t)(func01 - (pattern + 1));

					*(start) = 0x50;		// push rax
					*(start + 1) = 0xE8;	// call
					memcpy(start + 2, &rel32, sizeof(uint32_t));
					*(start + 6) = 0x58;	// pop rax

					dec_count++;
				}
				else if (*(start + 8) == 0x42)
					// lock xadd dword ptr ds:[rdx+8], e??
					// "\x48\x89\xD9" -> mov rcx, rdx
					sub_lockdec_rXX_offset_08(start, pattern, "\x48\x89\xD1", 3);
				else if ((*(start + 8) == 0x43) || (*(start + 8) == 0x53) || 
					(*(start + 8) == 0x73) || (*(start + 8) == 0x4B))
					// lock xadd dword ptr ds:[rbx+8], e??
					// "\x48\x89\xD9" -> mov rcx, rbx
					sub_lockdec_rXX_offset_08(start, pattern, "\x48\x89\xD9", 3);
				else if (*(start + 8) == 0x46)
					// lock xadd dword ptr ds:[rsi+8], e??
					// "\x48\x89\xF1" -> mov rcx, rsi
					sub_lockdec_rXX_offset_08(start, pattern, "\x48\x89\xF1", 3);
				else if ((*(start + 8) == 0x47) || (*(start + 8) == 0x57))
					// lock xadd dword ptr ds:[rdi+8], e??
					// "\x48\x89\xF9" -> mov rcx, rdi
					sub_lockdec_rXX_offset_08(start, pattern, "\x48\x89\xF9", 3);
				else if ((*(start + 8) == 0x48) || (*(start + 8) == 0x50))
					// lock xadd dword ptr ds:[rax+8], e??
					// "\x48\x89\xF9" -> mov rcx, rax
					sub_lockdec_rXX_offset_08(start, pattern, "\x48\x89\xC1", 3);
				/*else
					LogWindow::Log("%p %X", start, *(start + 8));*/
			}
			else
			{
				// As a rule, here is the code where the compiler played a trick and 
				// added the code that is needed later and not exactly.

				if (Offsets::IsCKVersion16438())
				{
					if (((uintptr_t)start == 0x1426DA491) || ((uintptr_t)start == 0x142B8C2DC))
					{
						// mov eax, FFFFFFFF
						// lock xadd dword ptr ds:[rdi+8], eax 
						// cmp eax, r15d

						// r15d always = 1
						sub_lockdec_rXX_offset_08(start, pattern, "\x48\x89\xF9", 3);
						continue;
					}
					else if ((uintptr_t)start == 0x142B13135)
					{
						// mov eax, FFFFFFFF
						// lock xadd dword ptr ds:[rbx+8], eax
						// mov edi, 1
						// cmp eax, edi

						// edi always = 1
						sub_lockdec_rXX_offset_08(start, pattern, "\x48\x89\xD9", 3, 0x11);
						continue;
					}
					else if ((uintptr_t)start == 0x142668E6C)
					{
						// mov eax, FFFFFFFF
						// lock xadd dword ptr ds:[rbx+8], eax
						// mov rbx, qword ptr ss:[rsp+68]
						// sub eax, 1

						memcpy(szBuf, start + 10, 5);
						sub_lockdec_rXX_offset_08(start, pattern, "\x48\x89\xD9", 3, 0x12);
						memcpy(start + 12, szBuf, 5);

						continue;
					}
					//else
					//	LogWindow::Log("%p %X", start, *(start + 10));
				}
			}
		}
		else if ((*(start + 1) == 0x01) && (*(start + 2) == 0x00) &&
				 (*(start + 3) == 0x00) && (*(start + 4) == 0x00))
		{
			// mov e??, 1
			// lock xadd dword ptr ds:[rdx+8], eax
			// --- to ---
			// xor e??, e??
			// inc e??
			// lock xadd qword ptr ds:[rdx+8], rax

			auto reg_num = (*start - 0xB8);

			memcpy(start, "\x31\xC0\xFF\xC0\xF0\x48", 6);

			*(start + 1) += reg_num;
			*(start + 3) += reg_num;

			inc_count++;
		}
		else 
		{
			start = (unsigned char*)(pattern);

			if ((*(start + 5) == 0x83) && (*(start + 7) == 0x01))
			{
				// lock xadd dword ptr ds:[r??+8], e??
				// cmp e??, 1

				// This is definitely a dec, since there is a comparison by 1

				if ((*(start + 3) == 0x41) || (*(start + 3) == 0x51) ||
					(*(start + 3) == 0x59) || (*(start + 3) == 0x79) || 
					(*(start + 3) == 0x71) || (*(start + 3) == 0x69))
					// lock xadd dword ptr ds:[rcx+8], e??
					sub_lockdec_rXX_offset_08_short(start, pattern, func01);
				else if (*(start + 3) == 0x42)
					// lock xadd dword ptr ds:[rdx+8], e??
					sub_lockdec_rXX_offset_08_short(start, pattern, func01_rdx);
				else if ((*(start + 3) == 0x43) || (*(start + 3) == 0x53) ||
					     (*(start + 3) == 0x73) || (*(start + 3) == 0x4B) || 
					     (*(start + 3) == 0x7B))
					// lock xadd dword ptr ds:[rbx+8], e??
					sub_lockdec_rXX_offset_08_short(start, pattern, func01_rbx);
				else if ((*(start + 3) == 0x46) || (*(start + 3) == 0x6E) || 
					     (*(start + 3) == 0x7E) || (*(start + 3) == 0x6B))
					// lock xadd dword ptr ds:[rsi+8], e??
					sub_lockdec_rXX_offset_08_short(start, pattern, func01_rsi);
				else if ((*(start + 3) == 0x47) || (*(start + 3) == 0x57) || 
					     (*(start + 3) == 0x5F) || (*(start + 3) == 0x77) || 
					     (*(start + 3) == 0x6F) || (*(start + 3) == 0x4F))
					// lock xadd dword ptr ds:[rdi+8], e??
					sub_lockdec_rXX_offset_08_short(start, pattern, func01_rdi);
				else if ((*(start + 3) == 0x70) || (*(start + 3) == 0x48) ||
					 	 (*(start + 3) == 0x68) || (*(start + 3) == 0x78) || 
					     (*(start + 3) == 0x58))
					// lock xadd dword ptr ds:[rax+8], e??
					sub_lockdec_rXX_offset_08_short(start, pattern, func01_rax, 1);
				else
				{
					LogWindow::Log("%p %X", start, *(start + 3));
				}
			}
		}
	}

	LogWindow::Log("Patch BSHandleRefObject (find sign : %u)", all_count);
	LogWindow::Log("\tBSHandleRefObject -> IncRef %u", inc_count);
	LogWindow::Log("\tBSHandleRefObject -> DecRef %u", dec_count);

	VirtualProtect((void*)g_CodeBase, g_CodeEnd - g_CodeBase, protect, &protect);
	FlushInstructionCache(GetCurrentProcess(), (void*)g_CodeBase, g_CodeEnd - g_CodeBase);

#endif
}