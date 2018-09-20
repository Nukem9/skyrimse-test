#pragma once

#pragma warning(disable:4094) // untagged 'struct' declared no symbols

#define Assert(cond)					if(!(cond)) XutilAssert(__FILE__, __LINE__, #cond);
#define AssertDebug(cond)				if(!(cond)) XutilAssert(__FILE__, __LINE__, #cond);
#define AssertMsg(cond, msg)			AssertMsgVa(cond, msg);
#define AssertMsgDebug(cond, msg)		AssertMsgVa(cond, msg);
#define AssertMsgVa(cond, msg, ...)		if(!(cond)) XutilAssert(__FILE__, __LINE__, "%s\n\n" msg, #cond, ##__VA_ARGS__);

#define VTABLE_FUNCTION_INDEX(Function) vtable_index_util::getIndexOf(&Function)

#define GAME_TLS(Type, Offset) *(Type *)(*(uintptr_t *)(__readgsqword(0x58u) + 8i64 * (*(uint32_t *)(g_ModuleBase + 0x34BBA78))) + (Offset))

class vtable_index_util
{
private:
	typedef int(*VtableIndexFn)();
	static vtable_index_util *GlobalInstance;

public:
	static vtable_index_util *Instance();

	template<typename T>
	static int getIndexOf(T ptr)
	{
		return (Instance()->**((decltype(&ForceVtableReference)*)(&ptr)))();
	}

private:
	virtual int ForceVtableReference();
};

template<void(*ctor)()>
struct static_constructor
{
	struct constructor { constructor() { ctor(); } };
	static constructor c;
};

template<void(*ctor)()>
typename static_constructor<ctor>::constructor static_constructor<ctor>::c;

#define STATIC_CONSTRUCTOR(Id, Lambda) struct { static void Id(){ static_constructor<&Id>::c; Lambda(); } };

#define DECLARE_CONSTRUCTOR_HOOK(Class) \
	static Class *__ctor__(void *Instance) \
	{ \
		return new (Instance) Class(); \
	} \
	\
	static Class *__dtor__(Class *Thisptr, unsigned __int8) \
	{ \
		Thisptr->~Class(); \
		return Thisptr; \
	}

uintptr_t FindPatternSimple(uintptr_t StartAddress, uintptr_t MaxSize, const BYTE *ByteMask, const char *Mask);
void PatchMemory(ULONG_PTR Address, PBYTE Data, SIZE_T Size);
void SetThreadName(DWORD dwThreadID, const char *ThreadName);
void Trim(char *Buffer, char C);
void XutilAssert(const char *File, int Line, const char *Format, ...);

#define templated(...) __VA_ARGS__
#define AutoPtr(Type, Name, Offset) static Type& Name = (*(Type *)((uintptr_t)GetModuleHandle(nullptr) + Offset))

#define static_assert_offset(Structure, Member, Offset) struct __declspec(empty_bases) : CheckOffset<offsetof(Structure, Member), Offset> { }
#define assert_vtable_index(Function, Index) AssertMsgVa(vtable_index_util::getIndexOf(Function) == Index, "VTable index does not match (%d != %d)", vtable_index_util::getIndexOf(Function), Index)

template <size_t Offset, size_t RequiredOffset>
struct __declspec(empty_bases) CheckOffset
{
	static_assert(Offset <= RequiredOffset, "Offset is larger than expected");
	static_assert(Offset >= RequiredOffset, "Offset is smaller than expected");
};