#pragma once

#pragma warning(disable:4094) // untagged 'struct' declared no symbols

#define Assert(Cond)					if(!(Cond)) XUtil::XAssert(__FILE__, __LINE__, #Cond);
#define AssertDebug(Cond)				if(!(Cond)) XUtil::XAssert(__FILE__, __LINE__, #Cond);
#define AssertMsg(Cond, Msg)			AssertMsgVa(Cond, Msg);
#define AssertMsgDebug(Cond, Msg)		AssertMsgVa(Cond, Msg);
#define AssertMsgVa(Cond, Msg, ...)		if(!(Cond)) XUtil::XAssert(__FILE__, __LINE__, "%s\n\n" Msg, #Cond, ##__VA_ARGS__);

#define PROPERTY(read_func, write_func)	__declspec(property(get = read_func, put = write_func))
#define READ_PROPERTY(read_func)		__declspec(property(get = read_func))

#define ZoneScopedN(X)
#define templated(...)					__VA_ARGS__
#define AutoPtr(Type, Name, Offset)		static Type& Name = (*(Type *)((uintptr_t)GetModuleHandle(nullptr) + Offset))
#define AutoFunc(Type, Name, Offset)	static auto Name = ((Type)((uintptr_t)GetModuleHandle(nullptr) + Offset))

#define static_assert_offset(Structure, Member, Offset) struct __declspec(empty_bases) : CheckOffset<offsetof(Structure, Member), Offset> { }
#define assert_vtable_index(Function, Index) AssertMsgVa(XUtil::VtableIndexer::GetIndexOf(Function) == Index, "Virtual table index does not match (%d != %d)", XUtil::VtableIndexer::GetIndexOf(Function), Index)

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
	class VtableIndexer
	{
	private:
		using VtableIndexFn = int(*)();
		inline static VtableIndexer *GlobalInstance;

	public:
		static VtableIndexer *Instance();

		template<typename T>
		static int GetIndexOf(T ptr)
		{
			return (Instance()->**((decltype(&ForceVtableReference)*)(&ptr)))();
		}

	private:
		virtual int ForceVtableReference();
	};

	void SetThreadName(uint32_t ThreadID, const char *ThreadName);
	void Trim(char *Buffer, char C);
	void XAssert(const char *File, int Line, const char *Format, ...);
	uint64_t MurmurHash64A(const void *Key, size_t Len, uint64_t Seed = 0);

	uintptr_t FindPattern(uintptr_t StartAddress, uintptr_t MaxSize, const char *Mask);
	std::vector<uintptr_t> FindPatterns(uintptr_t StartAddress, uintptr_t MaxSize, const char *Mask);
	bool GetPESectionRange(uintptr_t ModuleBase, const char *Section, uintptr_t *Start, uintptr_t *End);

	void PatchMemory(uintptr_t Address, const uint8_t *Data, size_t Size);
	void PatchMemory(uintptr_t Address, std::initializer_list<uint8_t> Data);
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

	void InstallCrashDumpHandler();
	LONG WINAPI CrashDumpExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo);
}

// thread-safe template versions of thisVirtualCall()

template<typename TR>
__forceinline TR thisVirtualCall(size_t reloff, const void* ths) {
	return (*(TR(__fastcall**)(const void*))(*(__int64*)ths + reloff))(ths);
}

template<typename TR, typename T1>
__forceinline TR thisVirtualCall(size_t reloff, const void* ths, T1 a1) {
	return (*(TR(__fastcall**)(const void*, T1))(*(__int64*)ths + reloff))(ths, a1);
}

template<typename TR, typename T1, typename T2>
__forceinline TR thisVirtualCall(size_t reloff, const void* ths, T1 a1, T2 a2) {
	return (*(TR(__fastcall**)(const void*, T1, T2))(*(__int64*)ths + reloff))(ths, a1, a2);
}

template<typename TR, typename T1, typename T2, typename T3>
__forceinline TR thisVirtualCall(size_t reloff, const void* ths, T1 a1, T2 a2, T3 a3) {
	return (*(TR(__fastcall**)(const void*, T1, T2, T3))(*(__int64*)ths + reloff))(ths, a1, a2, a3);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4>
__forceinline TR thisVirtualCall(size_t reloff, const void* ths, T1 a1, T2 a2, T3 a3, T4 a4) {
	return (*(TR(__fastcall**)(const void*, T1, T2, T3, T4))(*(__int64*)ths + reloff))(ths, a1, a2, a3, a4);
}