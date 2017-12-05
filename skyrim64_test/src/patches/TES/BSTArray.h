#pragma once

#include <stdint.h>

class BSTArrayHeapAllocator
{
	friend class __BSTArrayCheckOffsets;

private:
	void *m_Buffer;
	uint32_t m_AllocSize;

public:
	BSTArrayHeapAllocator() : m_Buffer(nullptr), m_AllocSize(0)
	{
	}

	void *QBuffer()
	{
		return m_Buffer;
	}

	uint32_t QAllocSize()
	{
		return m_AllocSize;
	}
};

class BSTArrayBase
{
	friend class __BSTArrayCheckOffsets;

private:
	uint32_t m_Count;

public:
	BSTArrayBase() : m_Count(0)
	{
	}

	uint32_t QSize()
	{
		return m_Count;
	}

	bool QEmpty()
	{
		return m_Count == 0;
	}
};

template <class _Ty, class _Alloc = BSTArrayHeapAllocator>
class BSTArray : public _Alloc, public BSTArrayBase
{
	friend class __BSTArrayCheckOffsets;

public:
	using value_type = _Ty;
	using allocator_type = _Alloc;
	using reference = _Ty&;
	using const_reference = const _Ty&;
	using size_type = uint32_t;

	BSTArray()
	{
	}

	reference Front() {
		return *reinterpret_cast<_Ty *>(QBuffer());
	}

	const_reference Front() const {
		return *reinterpret_cast<const _Ty *>(QBuffer());
	}

	reference operator[](const size_type Pos)
	{
		return *(&Front() + Pos);
	}

	const_reference operator[](const size_type Pos) const
	{
		return *(&Front() + Pos);
	}

	reference at(const size_type Pos)
	{
		return *(&Front() + Pos);
	}

	const_reference at(const size_type Pos) const
	{
		return *(&Front() + Pos);
	}

	reference front()
	{
		return (*this->_Myfirst());
	}

	const_reference front() const
	{
		return (*this->_Myfirst());
	}

	reference back()
	{
		return (this->_Mylast()[-1]);
	}

	const_reference back() const
	{
		return (this->_Mylast()[-1]);
	}

private:
	_Ty *_Myfirst()
	{
		return (_Ty *)QBuffer();
	}

	_Ty *_Mylast()
	{
		return ((_Ty *)QBuffer()) + QSize();
	}
};

class __BSTArrayCheckOffsets
{
	static_assert(offsetof(BSTArrayHeapAllocator, m_Buffer) == 0x0, "");
	static_assert(offsetof(BSTArrayHeapAllocator, m_AllocSize) == 0x8, "");

	static_assert(offsetof(BSTArrayBase, m_Count) == 0x0, "");

	static_assert(offsetof(BSTArray<int>, m_Buffer) == 0x0, "");
	static_assert(offsetof(BSTArray<int>, m_AllocSize) == 0x8, "");
	static_assert(offsetof(BSTArray<int>, m_Count) == 0x10, "");
};