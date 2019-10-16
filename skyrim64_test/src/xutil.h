#pragma once

#pragma warning(disable:4094) // untagged 'struct' declared no symbols

#define Assert(Cond)					if(!(Cond)) XUtil::XAssert(__FILE__, __LINE__, #Cond);
#define AssertDebug(Cond)				if(!(Cond)) XUtil::XAssert(__FILE__, __LINE__, #Cond);
#define AssertMsg(Cond, Msg)			AssertMsgVa(Cond, Msg);
#define AssertMsgDebug(Cond, Msg)		AssertMsgVa(Cond, Msg);
#define AssertMsgVa(Cond, Msg, ...)		if(!(Cond)) XUtil::XAssert(__FILE__, __LINE__, "%s\n\n" Msg, #Cond, ##__VA_ARGS__);

#define templated(...)					__VA_ARGS__
#define AutoPtr(Type, Name, Offset)		static Type& Name = (*(Type *)((uintptr_t)GetModuleHandle(nullptr) + Offset))
#define AutoFunc(Type, Name, Offset)	static auto Name = ((Type)((uintptr_t)GetModuleHandle(nullptr) + Offset))

#define static_assert_offset(Structure, Member, Offset) struct __declspec(empty_bases) : CheckOffset<offsetof(Structure, Member), Offset> { }
#define assert_vtable_index(Function, Index) AssertMsgVa(VtableIndexUtil::GetIndexOf(Function) == Index, "Virtual table index does not match (%d != %d)", VtableIndexUtil::GetIndexOf(Function), Index)

#define GAME_TLS(Type, Offset) *(Type *)(*(uintptr_t *)(__readgsqword(0x58u) + 8i64 * (*(uint32_t *)(g_ModuleBase + 0x34BBA78))) + (Offset))

#define STATIC_CONSTRUCTOR(Id, Lambda) \
	struct \
	{ \
		static void Id() \
		{ \
			StaticConstructor<&Id>::C; \
			Lambda(); \
		} \
	};

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

class VtableIndexUtil
{
private:
	typedef int(*VtableIndexFn)();
	static VtableIndexUtil *GlobalInstance;

public:
	static VtableIndexUtil *Instance();

	template<typename T>
	static int GetIndexOf(T ptr)
	{
		return (Instance()->**((decltype(&ForceVtableReference)*)(&ptr)))();
	}

private:
	virtual int ForceVtableReference();
};

template<void(*ctor)()>
struct StaticConstructor
{
	struct Constructor
	{
		Constructor()
		{
			ctor();
		}
	};

	static Constructor C;
};

template<void(*ctor)()>
typename StaticConstructor<ctor>::Constructor StaticConstructor<ctor>::C;

template <size_t Offset, size_t RequiredOffset>
struct __declspec(empty_bases)CheckOffset
{
	static_assert(Offset <= RequiredOffset, "Offset is larger than expected");
	static_assert(Offset >= RequiredOffset, "Offset is smaller than expected");
};

namespace XUtil
{
	void SetThreadName(uint32_t ThreadID, const char *ThreadName);
	void Trim(char *Buffer, char C);
	void XAssert(const char *File, int Line, const char *Format, ...);
	uint64_t MurmurHash64A(const void *Key, size_t Len, uint64_t Seed = 0);

	uintptr_t FindPattern(uintptr_t StartAddress, uintptr_t MaxSize, const uint8_t *Bytes, const char *Mask);
	void PatchMemory(uintptr_t Address, uint8_t *Data, size_t Size);
	void PatchMemoryNop(uintptr_t Address, size_t Size);
	void DetourJump(uintptr_t Target, uintptr_t Destination);
	void DetourCall(uintptr_t Target, uintptr_t Destination);

	template<typename T>
	void DetourJump(uintptr_t Target, T Destination)
	{
		static_assert(std::is_member_function_pointer_v<T> || (std::is_pointer_v<T> && std::is_function_v<typename std::remove_pointer<T>::type>));

		DetourJump(Target, *(uintptr_t *)&Destination);
	}

	template<typename T>
	void DetourCall(uintptr_t Target, T Destination)
	{
		static_assert(std::is_member_function_pointer_v<T> || (std::is_pointer_v<T> && std::is_function_v<typename std::remove_pointer<T>::type>));

		DetourCall(Target, *(uintptr_t *)&Destination);
	}
}