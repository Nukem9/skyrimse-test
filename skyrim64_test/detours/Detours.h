#pragma once

namespace Detours
{
	// --------- Typedef ---------
	typedef unsigned __int8 uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;

#ifdef _M_IX86
	typedef __w64 unsigned long sizeptr_t;
#else
	typedef unsigned long long	sizeptr_t;
#endif // _M_IX86
	// ---------------------------

	const static uint32_t DISASM_MAX_INSTRUCTIONS	= 50;		// Maximum number of instructions to decode at once

	const static uint32_t OPT_MASK					= 0xFFF;	// Mask for all options
	const static uint32_t OPT_NONE					= 0x000;	// No options

#ifdef _M_IX86
	enum class X86Option
	{
		USE_JUMP,		// jmp <address>;
		USE_CALL,		// call <address>;
		USE_EAX_JUMP,	// mov eax, <address>; jmp eax;
		USE_JUMP_PTR,	// jmp dword ptr [<address>];
		USE_PUSH_RET,	// push <address>; retn;
	};

	namespace X86
	{
		// Redirects a single static function to another
		uint8_t		*DetourFunction(uint8_t *Target, uint8_t *Detour, X86Option Options = X86Option::USE_JUMP);

		// Redirects a class member function (__thiscall) to another
		template<typename T>
		uint8_t		*DetourFunctionClass(uint8_t *Target, T Detour, X86Option Options = X86Option::USE_JUMP)
		{
			return DetourFunction(Target, *(uint8_t **)&Detour, Options);
		}

		// Removes a detoured function (Static or class member)
		bool		DetourRemove(uint8_t *Trampoline);

		// Redirects an index in a virtual table
		uint8_t		*DetourVTable(uint8_t *Target, uint8_t *Detour, uint32_t TableIndex);

		// Redirects a class member virtual function (__thiscall) to another
		template<typename T>
		uint8_t		*DetourClassVTable(uint8_t *Target, T Detour, uint32_t TableIndex)
		{
			return DetourVTable(Target, *(uint8_t **)&Detour, TableIndex);
		}

		// Removes a detoured virtual table index
		bool		VTableRemove(uint8_t *Target, uint8_t *Function, uint32_t TableIndex);

		// Hook a function via the PE import address table
		uint8_t		*DetourIAT(uint8_t *TargetModule, uint8_t *Detour, const char *ImportModule, const char *API);

		// Returns a predetermined hook length from active local and global options
		uint32_t	DetourGetHookLength(X86Option Options);
	}
#endif // _M_IX86

#ifdef _WIN64
	enum class X64Option
	{
		USE_PUSH_RET,	// push <low 32 address>; [rsp+4h] = <hi 32 addr>; retn;
		USE_RAX_JUMP,	// mov rax, <address>; jmp rax;
	};

	namespace X64
	{
		// Redirects a single static function to another
		uint8_t		*DetourFunction(uint8_t *Target, uint8_t *Detour, X64Option Options = X64Option::USE_PUSH_RET);

		// Redirects a class member function (__thiscall) to another
		template<typename T>
		uint8_t		*DetourFunctionClass(uint8_t *Target, T Detour, X64Option Options = X64Option::USE_PUSH_RET)
		{
			return DetourFunction(Target, *(uint8_t **)&Detour, Options);
		}

		// Removes a detoured function (Static or class member)
		bool		DetourRemove(uint8_t *Trampoline);

		// Redirects an index in a virtual table
		uint8_t		*DetourVTable(uint8_t *Target, uint8_t *Detour, uint32_t TableIndex);

		// Redirects a class member virtual function (__thiscall) to another
		template<typename T>
		uint8_t		*DetourClassVTable(uint8_t *Target, T Detour, uint32_t TableIndex)
		{
			return DetourVTable(Target, *(uint8_t **)&Detour, TableIndex);
		}

		// Removes a detoured virtual table index
		bool		VTableRemove(uint8_t *Target, uint8_t *Function, uint32_t TableIndex);

		// Hook a function via the PE import address table
		uint8_t		*DetourIAT(uint8_t *TargetModule, uint8_t *Detour, const char *ImportModule, const char *API);

		// Returns a predetermined hook length from active local and global options
		uint32_t	DetourGetHookLength(X64Option Options);
	}
#endif // _WIN64
}