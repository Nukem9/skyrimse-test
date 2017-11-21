#include "common.h"
#include <functional>

thread_local char BSGraphics_TLSGlob[0x4000];

HANDLE event;
HANDLE doneevent;
void(*execFunc)(__int64, unsigned int);

__int64 ga1;
unsigned int ga2;

void TLS_RevertThread();
void TLS_RestoreThread();
uintptr_t GetMainTls();
uintptr_t GetMyTls();

DWORD WINAPI DC_Thread(LPVOID Arg)
{
	uintptr_t bsg_offset = GetTlsOffset(&BSGraphics_TLSGlob);

	void *newData = (void *)(GetMyTls() + bsg_offset);		// This thread
	void *oldData = (void *)(GetMainTls() + bsg_offset);	// Main thread

	// Note: This thread is always running with the original TLS slot
	TLS_RevertThread();

	while (true)
	{
		if (WaitForSingleObject(event, INFINITE) == WAIT_FAILED)
			__debugbreak();

		// We should be using the original TLS slot for this thread now
		if (newData != (void *)&BSGraphics_TLSGlob)
			__debugbreak();
		
		if (oldData == (void *)&BSGraphics_TLSGlob)
			__debugbreak();

		// TLS this thread <==> TLS main thread
		if (newData == oldData)
			__debugbreak();

		memcpy(newData, oldData, 0x4000);

		execFunc(ga1, ga2);

		memcpy(oldData, newData, 0x4000);

		SetEvent(doneevent);
	}

	return 0;
}

void DC_Init()
{
	event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
	doneevent = CreateEventA(nullptr, FALSE, FALSE, nullptr);

	if (!event || !doneevent)
		__debugbreak();

	CreateThread(nullptr, 0, DC_Thread, nullptr, 0, nullptr);
}

void DC_RenderDeferred(__int64 a1, unsigned int a2, void(*func)(__int64, unsigned int))
{
	ga1 = a1;
	ga2 = a2;
	execFunc = func;

	// Switch to the other thread ASAP to prevent delaying whoever called this
	SetEvent(event);
}

void DC_WaitDeferred()
{
	if (WaitForSingleObject(doneevent, INFINITE) == WAIT_FAILED)
		__debugbreak();
}