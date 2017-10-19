#pragma once

namespace Detours
{
	struct JumpTrampolineHeader
	{
		uint32_t Magic;				// Used to verify the header
		uint32_t Random;			// Variable to change the code/data hash

		uint8_t *CodeOffset;		// Offset, in code, that was hooked:	"target"
		uint8_t *DetourOffset;		// User function that is called:		"destination"

		size_t InstructionLength;	// Length of the instructions that were replaced
		uint8_t *InstructionOffset;	// Where the backed-up instructions are

		size_t TrampolineLength;	// Length of the trampoline
		uint8_t *TrampolineOffset;	// Code offset where 'jmp (q/d)word ptr <user function>' occurs

									// Anything after this struct is null data or pure code (instructions/trampoline)
	};

	namespace Internal
	{
		void		SetGlobalOptions(uint32_t Options);
		uint32_t	GetGlobalOptions();

		uint8_t		*AlignAddress(uint64_t Address, uint8_t Align);

		bool		AtomicCopy4X8(uint8_t *Target, uint8_t *Memory, sizeptr_t Length);
		bool		WriteMemory(uint8_t *Target, uint8_t *Memory, sizeptr_t Length);
		bool		FlushCache(uint8_t *Target, sizeptr_t Length);

		uint8_t		*IATHook(uint8_t *Module, const char *ImportModule, const char *API, uint8_t *Detour);
	}

#ifdef _M_IX86
	namespace X86
	{
		void		DetourWriteStub(JumpTrampolineHeader *Header);
		bool		DetourWriteJump(JumpTrampolineHeader *Header);
		bool		DetourWriteCall(JumpTrampolineHeader *Header);
		bool		DetourWriteEaxJump(JumpTrampolineHeader *Header);
		bool		DetourWriteJumpPtr(JumpTrampolineHeader *Header);
		bool		DetourWritePushRet(JumpTrampolineHeader *Header);
	}
#endif // _M_IX86

#ifdef _WIN64
	namespace X64
	{
		void		DetourWriteStub(JumpTrampolineHeader *Header);
		bool		DetourWriteRaxJump(JumpTrampolineHeader *Header);
		bool		DetourWritePushRet(JumpTrampolineHeader *Header);
	}
#endif // _WIN64
}