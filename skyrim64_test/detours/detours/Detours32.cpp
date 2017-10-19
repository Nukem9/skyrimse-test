#include "stdafx.h"

#ifdef _M_IX86
namespace Detours
{
	namespace X86
	{
		#define HEADER_MAGIC		'@D32'

		#define MAX_INSTRUCT_SIZE	0x08

		#define JUMP_LENGTH_32		0x05	// jmp <addr>
		#define CALL_LENGTH_32		0x05	// call <addr>
		#define JUMP_EAX_LENGTH_32	0x07	// mov eax, <addr>; jmp eax
		#define JUMP_PTR_LENGTH_32	0x06	// jmp dword ptr <addr>
		#define PUSH_RET_LENGTH_32	0x06	// push <addr>; retn

		#define ALIGN_32(x)			Internal::AlignAddress((uint64_t)(x), 4);

		uint8_t *DetourFunction(uint8_t *Target, uint8_t *Detour, X86Option Options)
		{
			// Decode the data
			_DecodedInst decodedInstructions[DISASM_MAX_INSTRUCTIONS];
			uint32_t decodedCount = 0;

			_DecodeResult res = distorm_decode((_OffsetType)Target, (const unsigned char *)Target, 32, Decode32Bits, decodedInstructions, DISASM_MAX_INSTRUCTIONS, &decodedCount);

			// Check if decoding failed
			if (res != DECRES_SUCCESS)
				return nullptr;

			// Calculate the hook length from options
			uint32_t totalInstrSize = 0;
			uint32_t neededSize		= DetourGetHookLength(Options);

			// Calculate how many instructions are needed to place the jump
			for (uint32_t i = 0; i < decodedCount; i++)
			{
				totalInstrSize += decodedInstructions[i].size;

				if (totalInstrSize >= neededSize)
					break;
			}

			// Unable to find a needed length
			if (totalInstrSize < neededSize)
				return nullptr;

			// Allocate the trampoline page
			uint32_t allocSize = 0;
			allocSize += sizeof(JumpTrampolineHeader);	// Base structure
			allocSize += totalInstrSize;				// Size of the copied instructions
			allocSize += MAX_INSTRUCT_SIZE;				// Maximum instruction size
			allocSize += MAX_INSTRUCT_SIZE;				// Maximum instruction size
			allocSize += 0x64;							// Padding for any memory alignment

			uint8_t *jumpTrampolinePtr = (uint8_t *)VirtualAlloc(nullptr, allocSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			if (!jumpTrampolinePtr)
				return nullptr;

			// Fill out the header
			auto header					= (JumpTrampolineHeader *)jumpTrampolinePtr;

			header->Magic				= HEADER_MAGIC;
			header->Random				= GetCurrentProcessId() + GetCurrentThreadId() + totalInstrSize;

			header->CodeOffset			= Target;
			header->DetourOffset		= Detour;

			header->InstructionLength	= totalInstrSize;
			header->InstructionOffset	= ALIGN_32(jumpTrampolinePtr + sizeof(JumpTrampolineHeader));

			// Apply any changes because of the different IP
			{
				//BYTE *data = Reassembler::Reassemble32((uint32_t)header->InstructionOffset, decodedInstructions, decodedInstructionsCount, &header->InstructionLength);

				// Copy the fixed instructions over
				//memcpy(header->InstructionOffset, data, header->InstructionLength);
				memcpy(header->InstructionOffset, Target, header->InstructionLength);

				// Free unneeded data
				//Reassembler::Free(data);
			}

			header->TrampolineLength	= JUMP_LENGTH_32;
			header->TrampolineOffset	= ALIGN_32(header->InstructionOffset + header->InstructionLength + JUMP_LENGTH_32 + header->TrampolineLength);

			// Write the assembly in the allocation block
			DetourWriteStub(header);

			bool result = true;

			switch (Options)
			{
			case X86Option::USE_JUMP:		result = DetourWriteJump(header);		break;
			case X86Option::USE_CALL:		result = DetourWriteCall(header);		break;
			case X86Option::USE_EAX_JUMP:	result = DetourWriteEaxJump(header);	break;
			case X86Option::USE_JUMP_PTR:	result = DetourWriteJumpPtr(header);	break;
			case X86Option::USE_PUSH_RET:	result = DetourWritePushRet(header);	break;
			default:						result = false;							break;
			}

			// If an operation failed, free the memory and exit
			if (!result)
			{
				VirtualFree(jumpTrampolinePtr, 0, MEM_RELEASE);
				return nullptr;
			}

			// Force flush any possible CPU caches
			Internal::FlushCache(Target, totalInstrSize);
			Internal::FlushCache(jumpTrampolinePtr, allocSize);

			// Set read/execution on the trampoline page
			DWORD dwOld = 0;
			VirtualProtect(jumpTrampolinePtr, allocSize, PAGE_EXECUTE_READ, &dwOld);

			return header->InstructionOffset;
		}

		bool DetourRemove(uint8_t *Trampoline)
		{
			JumpTrampolineHeader *header = (JumpTrampolineHeader *)(Trampoline - sizeof(JumpTrampolineHeader));

			if (header->Magic != HEADER_MAGIC)
				return false;

			// Rewrite the backed-up code
			if (!Internal::AtomicCopy4X8(header->CodeOffset, header->InstructionOffset, header->InstructionLength))
				return false;

			Internal::FlushCache(header->CodeOffset, header->InstructionLength);
			VirtualFree(header, 0, MEM_RELEASE);

			return true;
		}

		uint8_t *DetourVTable(uint8_t *Target, uint8_t *Detour, uint32_t TableIndex)
		{
			// Each function is stored in an array - also get a copy of the original
			uint8_t *virtualPointer = (Target + (TableIndex * sizeof(ULONG)));
			uint8_t *original		= *(uint8_t **)virtualPointer;

			if (!Internal::AtomicCopy4X8(virtualPointer, Detour, sizeof(ULONG)))
				return nullptr;

			return original;
		}

		bool VTableRemove(uint8_t *Target, uint8_t *Function, uint32_t TableIndex)
		{
			// Reverse VTable detour
			return DetourVTable(Target, Function, TableIndex) != nullptr;
		}

		uint8_t *DetourIAT(uint8_t *TargetModule, uint8_t *Detour, const char *ImportModule, const char *API)
		{
			return Internal::IATHook(TargetModule, ImportModule, API, Detour);
		}

		void DetourWriteStub(JumpTrampolineHeader *Header)
		{
			/********** Allocated code block modifications **********/

			// Determine where the 'unhooked' part of the function starts
			uint8_t *unhookStart = (Header->CodeOffset + Header->InstructionLength);

			// Jump to hooked function (backed up instructions)
			uint8_t *binstr_ptr = (Header->InstructionOffset + Header->InstructionLength);

			AsmGen hookGen(binstr_ptr, ASMGEN_32);
			hookGen.AddCode("jmp 0x%X", unhookStart);
			hookGen.WriteStreamTo(binstr_ptr);

			// Jump to user function (write the trampoline)
			AsmGen userGen(Header->TrampolineOffset, ASMGEN_32);
			userGen.AddCode("jmp 0x%X", Header->DetourOffset);
			userGen.WriteStreamTo(Header->TrampolineOffset);
		}

		bool DetourWriteJump(JumpTrampolineHeader *Header)
		{
			AsmGen gen(Header->CodeOffset, ASMGEN_32);

			// Jump to trampoline (from hooked function)
			gen.AddCode("jmp 0x%X", Header->TrampolineOffset);

			return Internal::AtomicCopy4X8(Header->CodeOffset, gen.GetStream(), gen.GetStreamLength());
		}

		bool DetourWriteCall(JumpTrampolineHeader *Header)
		{
			AsmGen gen(Header->CodeOffset, ASMGEN_32);

			// Call to trampoline (from hooked function)
			gen.AddCode("call 0x%X", Header->TrampolineOffset);

			return Internal::AtomicCopy4X8(Header->CodeOffset, gen.GetStream(), gen.GetStreamLength());
		}

		bool DetourWriteEaxJump(JumpTrampolineHeader *Header)
		{
			AsmGen gen(Header->CodeOffset, ASMGEN_32);

			// Jump to trampoline with eax
			gen.AddCode("mov eax, 0x%X", Header->TrampolineOffset);
			gen.AddCode("jmp eax");

			return Internal::AtomicCopy4X8(Header->CodeOffset, gen.GetStream(), gen.GetStreamLength());
		}

		bool DetourWriteJumpPtr(JumpTrampolineHeader *Header)
		{
			AsmGen gen(Header->CodeOffset, ASMGEN_32);

			// Pointer jump to trampoline
			gen.AddCode("jmp dword ptr ds:[0x%X]", &Header->TrampolineOffset);

			return Internal::AtomicCopy4X8(Header->CodeOffset, gen.GetStream(), gen.GetStreamLength());
		}

		bool DetourWritePushRet(JumpTrampolineHeader *Header)
		{
			AsmGen gen(Header->CodeOffset, ASMGEN_32);

			// RET-Jump to trampoline
			gen.AddCode("push 0x%X", Header->TrampolineOffset);
			gen.AddCode("retn");

			return Internal::AtomicCopy4X8(Header->CodeOffset, gen.GetStream(), gen.GetStreamLength());
		}

		uint32_t DetourGetHookLength(X86Option Options)
		{
			uint32_t size = 0;

			switch (Options)
			{
			case X86Option::USE_JUMP:		size += JUMP_LENGTH_32;		break;
			case X86Option::USE_CALL:		size += CALL_LENGTH_32;		break;
			case X86Option::USE_EAX_JUMP:	size += JUMP_EAX_LENGTH_32;	break;
			case X86Option::USE_JUMP_PTR:	size += JUMP_PTR_LENGTH_32;	break;
			case X86Option::USE_PUSH_RET:	size += PUSH_RET_LENGTH_32;	break;
			default:						size = 0;					break;
			}

			return size;
		}
	}
}
#endif // _M_IX86