#pragma once

template<typename T>
class BSSimpleList
{
public:
	T m_item;
	BSSimpleList<T> *m_pkNext;

	BSSimpleList()
	{
		m_item = (T)0;
		m_pkNext = nullptr;
	}

	~BSSimpleList()
	{
		RemoveAllNodes(nullptr, nullptr);
	}

	static void operator delete(void *Ptr, std::size_t Size)
	{
		AutoFunc(void(__fastcall *)(void *), sub_1401026F0, 0x1026F0);
		sub_1401026F0((void *)Ptr);
	}

	void RemoveNode(void(*Callback)(BSSimpleList<T> *, void *) = nullptr, void *UserData = nullptr)
	{
		BSSimpleList<T> *currNode = m_pkNext;

		if (currNode)
		{
			m_pkNext = currNode->m_pkNext;
			m_item = currNode->m_item;
			currNode->m_pkNext = nullptr;

			if (Callback)
				Callback(currNode, UserData);
			else
				delete currNode;
		}
		else
		{
			m_item = (T)0;
		}
	}

	void RemoveAllNodes(void(*Callback)(BSSimpleList<T> *, void *) = nullptr, void *UserData = nullptr)
	{
		if (m_pkNext)
		{
			while (true)
			{
				BSSimpleList<T> *v6 = this->m_pkNext;
				BSSimpleList<T> *v7 = v6->m_pkNext;
				v6->m_pkNext = nullptr;

				if (Callback)
					break;

				BSSimpleList<T> *v8 = this->m_pkNext;

				if (v8)
				{
					delete v8;
					goto LABEL_6;
				}

			LABEL_7:
				this->m_pkNext = v7;

				if (!v7)
				{
					this->m_item = (T)0;
					return;
				}
			}

			Callback(this->m_pkNext, UserData);

		LABEL_6:
			goto LABEL_7;
		}

		this->m_item = (T)0;
	}

	T QItem() const
	{
		return m_item;
	}

	BSSimpleList<T> *QNext() const
	{
		return m_pkNext;
	}
};