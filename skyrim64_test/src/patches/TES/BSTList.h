#pragma once

template<typename T>
class BSSimpleList
{
private:
	T m_item;
	BSSimpleList<T> *m_pkNext;

public:
	T QItem()
	{
		return m_item;
	}

	BSSimpleList<T> *QNext()
	{
		return m_pkNext;
	}
};