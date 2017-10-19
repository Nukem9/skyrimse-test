#pragma once

#ifdef _M_IX86
#define LockCompareExchange32	InterlockedCompareExchange
#define LockExchange32			InterlockedExchange

static
LONGLONG
LockCompareExchange64(
	__inout LONGLONG volatile *Destination,
	__in    LONGLONG ExChange,
	__in    LONGLONG Comperand
)
{
	__asm
	{
		mov esi, [Destination]
		mov ebx, dword ptr [ExChange]
		mov ecx, dword ptr [ExChange + 4]
		mov eax, dword ptr [Comperand]
		mov edx, dword ptr [Comperand + 4]
		lock cmpxchg8b qword ptr [esi]
	}
}

FORCEINLINE
LONGLONG
LockExchange64(
	__inout LONGLONG volatile *Target,
	__in    LONGLONG Value
)
{
	LONGLONG Old;

	do
	{
		Old = *Target;
	} while (LockCompareExchange64(Target, Value, Old) != Old);

	return Old;
}
#else
#define LockCompareExchange32	InterlockedCompareExchange
#define LockExchange32			InterlockedExchange

#define LockCompareExchange64	InterlockedCompareExchange64
#define LockExchange64			InterlockedExchange64
#endif // _M_IX86