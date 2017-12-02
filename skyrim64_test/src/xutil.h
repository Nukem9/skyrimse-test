#pragma once

#include <vector>
#include <type_traits>
#pragma warning(disable:4094) // untagged 'struct' declared no symbols

#define VTABLE_FUNCTION_INDEX(Function) vtable_index_util::getIndexOf(&Function)

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

#pragma pack(push, 8)  
const DWORD MS_VC_EXCEPTION = 0x406D1388;

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;		// Must be 0x1000.  
	LPCSTR szName;		// Pointer to name (in user addr space).  
	DWORD dwThreadID;	// Thread ID (-1=caller thread).  
	DWORD dwFlags;		// Reserved for future use, must be zero.  
} THREADNAME_INFO;
#pragma pack(pop)

#define STATIC_CONSTRUCTOR(Id, Lambda) struct { static void Id(){ static_constructor<&Id>::c; Lambda(); } };

intptr_t FindPattern(const std::vector<unsigned char>& data, intptr_t baseAddress, const unsigned char *lpPattern, const char *pszMask, intptr_t offset, intptr_t resultUsage);
uintptr_t FindPatternSimple(uintptr_t StartAddress, uintptr_t MaxSize, const BYTE *ByteMask, const char *Mask);
void PatchMemory(ULONG_PTR Address, PBYTE Data, SIZE_T Size);
void SetThreadName(DWORD dwThreadID, const char *ThreadName);
void Trim(char *Buffer, char C);


template<typename T, uintptr_t Offset = 0>
class AutoPtr
{
private:
	const uintptr_t m_RawAddress;

	AutoPtr(const AutoPtr&) = delete;
	AutoPtr operator =(const AutoPtr&) = delete;

public:
	template<bool B = Offset == 0, typename std::enable_if<B, void>::type* = nullptr>
	AutoPtr() :
		m_RawAddress(0)
	{
	}

	template<bool B = Offset == 0, typename std::enable_if<!B, void>::type* = nullptr>
	AutoPtr() :
		m_RawAddress(((uintptr_t)GetModuleHandle(nullptr) + Offset))
	{
	}

	// where T would end up being a double pointer (ex. T=void *, T=MyClass *)
	template<bool B = std::is_pointer<T>::value, typename std::enable_if<B, void>::type* = nullptr>
	inline operator T& () const
	{
		return **(T **)m_RawAddress;
	}

	template<bool B = std::is_pointer<T>::value, typename std::enable_if<B, void>::type* = nullptr>
	inline T& operator -> () const
	{
		return *(T *)m_RawAddress;
	}

	template<bool B = std::is_pointer<T>::value, typename std::enable_if<B, void>::type* = nullptr>
	inline operator bool() const
	{
		if constexpr(Offset == 0)
			return false;

		return *(T *)m_RawAddress != nullptr;
	}

	// where T would end up being a single pointer (ex. T=uint32_t, T=MyClass)
	template<bool B = std::is_pointer<T>::value, typename std::enable_if<!B, void>::type* = nullptr>
	inline operator T& () const
	{
		return *(T *)m_RawAddress;
	}

	template<bool B = std::is_pointer<T>::value, typename std::enable_if<!B, void>::type* = nullptr>
	inline T* operator -> () const
	{
		return (T *)m_RawAddress;
	}

	inline void operator = (const T& Value)
	{
		*(T *)m_RawAddress = Value;
	}

	T& get()
	{
		return *(T *)m_RawAddress;
	}

	inline operator T () const
	{
		return *(T *)m_RawAddress;
	}
};