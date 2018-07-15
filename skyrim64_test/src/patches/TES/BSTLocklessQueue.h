#pragma once

struct BSTLocklessQueue
{
	template<typename T, size_t Count, size_t Unknown>
	struct PtrMultiProdCons
	{
		T *QueueA[Count];
		volatile unsigned int uiQueueStart;		// Assumed
		volatile unsigned int uiQueueFetched;	// Assumed
		volatile unsigned int uiQueueEnd;		// Assumed
		volatile unsigned int uiQueueAlloced;	// Assumed
	};

	template<typename QueueContainer, typename T, size_t Count, size_t Unknown>
	struct ObjQueueBase
	{
		T ObjectA[Count];
		QueueContainer Queued;
		QueueContainer Free;
	};

	template<typename T, size_t Count, size_t Unknown>
	struct ObjMultiProdCons : ObjQueueBase<PtrMultiProdCons<T, Count * 2, Unknown>, T, Count, Unknown>
	{
	};
};