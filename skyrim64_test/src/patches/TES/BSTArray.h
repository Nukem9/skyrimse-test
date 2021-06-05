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

	void *QBuffer() const
	{
		return m_Buffer;
	}

	uint32_t QAllocSize() const
	{
		return m_AllocSize;
	}
};

class BSTArrayBase
{
	friend class __BSTArrayCheckOffsets;

private:
	uint32_t m_Size;

public:
	BSTArrayBase() : m_Size(0)
	{
	}

	uint32_t QSize() const
	{
		return m_Size;
	}

	bool QEmpty() const
	{
		return m_Size == 0;
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

	reference operator[](const size_type Pos)
	{
		return (this->_Myfirst()[Pos]);
	}

	const_reference operator[](const size_type Pos) const
	{
		return (this->_Myfirst()[Pos]);
	}

	reference at(const size_type Pos)
	{
		AssertMsg(Pos >= 0 && Pos < QSize(), "Exceeded array bounds");

		return (this->_Myfirst()[Pos]);
	}

	const_reference at(const size_type Pos) const
	{
		AssertMsg(Pos >= 0 && Pos < QSize(), "Exceeded array bounds");

		return (this->_Myfirst()[Pos]);
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
		return (_Ty *)_Alloc::QBuffer();
	}

	_Ty *_Mylast()
	{
		return ((_Ty *)_Alloc::QBuffer()) + QSize();
	}
};

class __BSTArrayCheckOffsets
{
	static_assert_offset(BSTArrayHeapAllocator, m_Buffer, 0x0);
	static_assert_offset(BSTArrayHeapAllocator, m_AllocSize, 0x8);

	static_assert_offset(BSTArrayBase, m_Size, 0x0);

	static_assert_offset(BSTArray<int>, m_Buffer, 0x0);
	static_assert_offset(BSTArray<int>, m_AllocSize, 0x8);
	static_assert_offset(BSTArray<int>, m_Size, 0x10);
};