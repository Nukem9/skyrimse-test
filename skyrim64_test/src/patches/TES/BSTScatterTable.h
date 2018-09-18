#pragma once

// Special thanks to himika (https://github.com/himika/libSkyrim/blob/2559175f7f30189b7d3681d01b3e055505c3e0d7/Skyrim/include/Skyrim/BSCore/BSTScatterTable.h)
// for providing most of this (iterators) as a reference.

template<typename Key, typename T>
struct BSTScatterTableDefaultKVStorage
{
private:
	Key m_Key;

public:
	T m_Value;

	const Key& GetKey()
	{
		return m_Key;
	}
};

template<typename Key, typename T, class Storage = BSTScatterTableDefaultKVStorage<Key, T>>
struct BSTScatterTableEntry : public Storage
{
	using key_type = Key;
	using value_type = T;

	BSTScatterTableEntry *m_Next;

	bool IsEmpty()
	{
		return m_Next == nullptr;
	}
};

// struct BSTScatterTableHeapAllocator<
//		struct BSTScatterTableEntry<unsigned int, class BSTArray<struct SetEventData, class BSTArrayHeapAllocator> const *, struct BSTScatterTableDefaultKVStorage>
//		>
template<typename T>
struct BSTScatterTableHeapAllocator
{
	using value_type = typename T::value_type;
	using pointer = typename T::value_type *;
	using const_pointer = const typename T::value_type *;
	using table_entry = T;

	T *Allocate(size_t Count)
	{
		throw;
	}
};

template<typename T, size_t MaxEntries>
struct BSTFixedSizeScatterTableAllocator
{
	// Doesn't allow any kind of dynamic allocation
	T m_StaticBuffer[MaxEntries];

	T *Allocate(size_t Count)
	{
		if (Count > MaxEntries)
			throw "FixedSizeScatterTableAllocator: Not enough static memory for map";

		return m_StaticBuffer;
	}
};

// struct BSTScatterTableDefaultHashPolicy<
//		unsigned int
//		>
template<typename Key>
struct BSTScatterTableDefaultHashPolicy
{
	size_t operator()(const Key& Value) const
	{
		return Value;
	}
};

template<typename Key>
struct BSTScatterTableCRCHashPolicy
{
	size_t operator()(const Key& Value) const
	{
		Key keyHash;
		CRC32_Lazy((int *)&keyHash, Value);

		return keyHash;
	}
};

// struct BSTScatterTableTraits<
//		unsigned int,
//		class BSTArray<struct SetEventData, class BSTArrayHeapAllocator> const *,
//		struct BSTScatterTableDefaultKVStorage,
//		struct BSTScatterTableDefaultHashPolicy<unsigned int>,
//		struct BSTScatterTableHeapAllocator<struct BSTScatterTableEntry<unsigned int, class BSTArray<struct SetEventData, class BSTArrayHeapAllocator> const *, struct BSTScatterTableDefaultKVStorage>>,
//		8>
template<
	typename Key,
	typename T,
	class Storage = BSTScatterTableDefaultKVStorage<Key, T>,
	class Hash = BSTScatterTableDefaultHashPolicy<Key>,
	class Allocator = BSTScatterTableHeapAllocator<T>,
	size_t InitialSize = 8
	>
struct BSTScatterTableTraits
{
	using key_type = Key;
	using mapped_type = T;
	using value_type = std::pair<Key, T>;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using hasher = Hash;
	using key_equal = std::equal_to<Key>;
	using allocator_type = Allocator;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = typename Allocator::pointer;
	using const_pointer = typename Allocator::const_pointer;

	using table_entry = typename Allocator::table_entry;
};

template<class Traits>
class BSTScatterTableKernel : public Traits, public Traits::hasher
{
	//
	// NOTE: There's a "missing" field at offset 0 here because of the minimum base class size
	// set in Visual Studio (1 byte, aligned to 4)
	//
	// When a list is empty, BSTScatterTableEntry::m_Next will be nullptr. If a list is NOT empty
	// and you hit the end, m_Next will be equal to m_EndOfList.
	//
	using table_entry = typename Traits::table_entry;

public:
	char _pad1[8];				// 0x04  +0
	uint32_t m_Size;			// 0x0C  +4
	uint32_t m_Free;			// 0x10  +8
	uint32_t m_LastFree;		// 0x14  +12
	table_entry *m_Terminator;	// 0x18  +16
};

template<class Traits>
class BSTScatterTableBase : public BSTScatterTableKernel<Traits>, public Traits::allocator_type
{
	//
	// Same invisible padding as BSTScatterTableKernel in here
	//
private:
	const static uint32_t InternalEndOfListMarker = 0x0EFBEADDE;

protected:
	using key_type = typename Traits::key_type;
	using mapped_type = typename Traits::mapped_type;
	using hasher = typename Traits::hasher;
	using const_reference = typename Traits::const_reference;
	using pointer = typename Traits::pointer;
	using const_pointer = typename Traits::const_pointer;
	using table_entry = typename Traits::table_entry;

	table_entry *m_Table;

public:
	BSTScatterTableBase()
	{
		m_Size			= 0;
		m_Free			= 0;
		m_LastFree		= 0;
		m_Terminator	= (table_entry *)&InternalEndOfListMarker;
		m_Table			= nullptr;
	}

	~BSTScatterTableBase()
	{
		AssertMsg(false, "Destructor not implemented");
	}

	class const_iterator
	{
		friend class BSTScatterTableBase;

	private:
		table_entry *m_Current;	// &m_Buckets[0];
		table_entry *m_End;		// &m_Buckets[m_AllocatedEntries];

	public:
		const_iterator& operator++(int)
		{
			do
				m_Current++;
			while (m_Current < m_End && m_Current->IsEmpty());

			return *this;
		}

		operator bool() const
		{
			return m_Current != nullptr;
		}

		operator const mapped_type() const
		{
			return m_Current->m_Value;
		}

		const mapped_type& operator*() const
		{
			return m_Current->m_Value;
		}
		
		const mapped_type& operator->() const
		{
			return m_Current->m_Value;
		}

		bool operator==(const const_iterator& Rhs) const
		{
			return m_Current == Rhs.m_Current;
		}
		
		bool operator!=(const const_iterator& Rhs) const
		{
			return m_Current != Rhs.m_Current;
		}

		void temphack(mapped_type a)
		{
			m_Current->m_Value = a;
		}

	protected:
		const_iterator(table_entry *Final) : m_Current(Final), m_End(Final)
		{
			// Empty/end() equivalent iterator constructor
		}

		const_iterator(table_entry *Current, table_entry *Final) : m_Current(Current), m_End(Final)
		{
			// Move to the first valid entry (that contains a value)
			while (m_Current < m_End && m_Current->IsEmpty())
				m_Current++;
		}
	};

	const_iterator begin() const noexcept
	{
		if (!m_Table)
			return const_iterator(nullptr);

		return const_iterator(&m_Table[0], &m_Table[m_Size]);
	}

	const_iterator end() const noexcept
	{
		if (!m_Table)
			return const_iterator(nullptr);
		
		return const_iterator(&m_Table[m_Size]);
	}

	const_iterator find(const key_type& Key) const
	{
		if (m_Table)
		{
			table_entry *entry = &m_Table[hasher()(Key) & (m_Size - 1)];

			if (!entry->IsEmpty())
			{
				while (entry != m_Terminator)
				{
					if (entry->GetKey() == Key)
						return const_iterator(entry, &m_Table[m_Size]);

					entry = entry->m_Next;
				}
			}
		}

		// Key not found
		return end();
	}

	bool get(const key_type& Key, mapped_type& Out) const
	{
		if (m_Table)
		{
			table_entry *entry = &m_Table[hasher()(Key) & (m_Size - 1)];

			if (!entry->IsEmpty())
			{
				while (entry != m_Terminator)
				{
					if (entry->GetKey() == Key)
					{
						Out = entry->m_Value;
						return true;
					}

					entry = entry->m_Next;
				}
			}
		}

		// Key not found
		return false;
	}

	mapped_type get(const key_type& Key) const
	{
		// Return a default-constructed T if not found
		mapped_type temp = mapped_type();

		get(Key, temp);
		return temp;
	}
};

// class BSTScatterTable<
//		unsigned int,
//		class BSTArray<struct SetEventData, class BSTArrayHeapAllocator> const *,
//		struct BSTScatterTableDefaultKVStorage,
//		struct BSTScatterTableDefaultHashPolicy,
//		struct BSTScatterTableHeapAllocator,
//		8>
template<
	typename Key,
	typename T,
	class Storage = BSTScatterTableDefaultKVStorage<Key, T>,
	class Hash = BSTScatterTableDefaultHashPolicy<Key>,
	class Allocator = BSTScatterTableHeapAllocator<BSTScatterTableEntry<Key, T>>,
	size_t InitialSize = 8
	>
class BSTScatterTable : public BSTScatterTableBase<BSTScatterTableTraits<Key, T, Storage, Hash, Allocator, InitialSize>>
{
};

template<typename Key, typename T>
struct BSTDefaultScatterTable : public BSTScatterTable<Key, T>
{
};

template<typename Key, typename T>
struct BSTCRCScatterTable : public BSTScatterTable<Key, T, BSTScatterTableDefaultKVStorage<Key, T>, BSTScatterTableCRCHashPolicy<Key>>
{
};

using Test2 = BSTDefaultScatterTable<uint32_t, uint32_t>;

static_assert(offsetof(Test2, m_Size) == 0xC, "");
static_assert(offsetof(Test2, m_Free) == 0x10, "");
static_assert(offsetof(Test2, m_LastFree) == 0x14, "");
static_assert(offsetof(Test2, m_Terminator) == 0x18, "");
//static_assert(offsetof(Test2, m_Buckets) == 0x28, "");
static_assert(sizeof(Test2) == 0x30, "");