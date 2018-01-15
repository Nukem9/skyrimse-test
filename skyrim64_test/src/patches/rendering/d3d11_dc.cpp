#include "common.h"
#include <atomic>
#include "../TES/BSGraphicsRenderer.h"

void TLS_RevertThread();
void TLS_RestoreThread();
uintptr_t GetMainTls();
uintptr_t GetMyTls();

struct ThreadData
{
	__int64 a1;
	unsigned int a2;
	void(*Callback)(__int64, unsigned int);

	HANDLE InitEvent;
	HANDLE RunEvent;
	HANDLE CompletedEvent;

	ID3D11DeviceContext2 *DeferredContext;
	std::atomic<ID3D11CommandList *> CommandList;

	BSGraphics::Renderer *ThreadGlobals;
};

ThreadData g_ThreadData[6];

ID3D11DeviceContext2 *devContext;

uintptr_t AllocateGuardedBlock();

DWORD WINAPI DC_Thread(LPVOID Arg)
{
	// Note: This thread is always running with the original TLS slot
	//TLS_RevertThread();

	*(uintptr_t *)(__readgsqword(0x58) + g_TlsIndex * sizeof(void *)) = AllocateGuardedBlock();

	SetThreadName(GetCurrentThreadId(), "DC_Thread");

	auto *oldData = BSGraphics::Renderer::GetGlobalsNonThreaded();	// Main thread (AKA global to entire program)
	auto *newData = BSGraphics::Renderer::GetGlobals();				// This thread

	ThreadData *jobData = (ThreadData *)Arg;
	jobData->CommandList = nullptr;
	jobData->ThreadGlobals = newData;
	SetEvent(jobData->InitEvent);

	while (true)
	{
		if (WaitForSingleObject(jobData->RunEvent, INFINITE) == WAIT_FAILED)
			__debugbreak();

		// Sanity check: We should be using the original TLS slot now
		if (newData != BSGraphics::Renderer::GetGlobals() || newData == BSGraphics::Renderer::GetGlobalsNonThreaded())
			__debugbreak();
		
		if (oldData == BSGraphics::Renderer::GetGlobals() || oldData != BSGraphics::Renderer::GetGlobalsNonThreaded())
			__debugbreak();

		// TLS this thread <==> TLS main thread
		if (newData == oldData)
			__debugbreak();

		if (jobData->CommandList.load() != nullptr)
			__debugbreak();

		newData->m_StateUpdateFlags = 0xFFFFFFFF & ~0x400;
		newData->m_DeviceContext = jobData->DeferredContext;

		jobData->Callback(jobData->a1, jobData->a2);

		ID3D11CommandList *commandList = nullptr;
		if (FAILED(jobData->DeferredContext->FinishCommandList(FALSE, &commandList)))
			__debugbreak();

		// Signal DC_WaitDeferred that this list is done
		jobData->CommandList.store(commandList);
	}

	return 0;
}

void DC_Init(ID3D11DeviceContext2 *ImmediateContext, ID3D11DeviceContext2 **DeferredContexts, int DeferredContextCount)
{
	devContext = ImmediateContext;

	for (int i = 0; i < DeferredContextCount; i++)
	{
		ThreadData *jobData = &g_ThreadData[i];
		memset(jobData, 0, sizeof(ThreadData));

		jobData->InitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		jobData->RunEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		jobData->CompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		jobData->DeferredContext = DeferredContexts[i];

		if (!jobData->InitEvent || !jobData->RunEvent || !jobData->CompletedEvent)
			__debugbreak();

		CreateThread(nullptr, 0, DC_Thread, jobData, 0, nullptr);
	}

	// Wait for threads to set up initial state
	for (int i = 0; i < DeferredContextCount; i++)
		WaitForSingleObject(g_ThreadData[i].InitEvent, INFINITE);
}

void DC_RenderDeferred(__int64 a1, unsigned int a2, void(*func)(__int64, unsigned int), int Index)
{
	ThreadData *jobData = &g_ThreadData[Index];

	jobData->a1 = a1;
	jobData->a2 = a2;
	jobData->Callback = func;

	ID3D11Buffer *psbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *vsbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *csbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];

	devContext->PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, psbuf);
	devContext->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vsbuf);
	devContext->CSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, csbuf);

	ID3D11ShaderResourceView *srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView *vsrvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];

	devContext->PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	devContext->VSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, vsrvs);

	auto dc1 = jobData->DeferredContext;

	dc1->PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, psbuf);
	dc1->VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vsbuf);
	dc1->CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, csbuf);

	dc1->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	dc1->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, vsrvs);

	memcpy(jobData->ThreadGlobals, BSGraphics::Renderer::GetGlobalsNonThreaded(), 0x4000);

	// Run the other thread ASAP to prevent delaying whoever called this
	SetEvent(jobData->RunEvent);

	for (int i = 0; i < 14; i++) if (psbuf[i]) psbuf[i]->Release();
	for (int i = 0; i < 14; i++) if (vsbuf[i]) vsbuf[i]->Release();
	for (int i = 0; i < 14; i++) if (csbuf[i]) csbuf[i]->Release();

	for (int i = 0; i < 128; i++) if (srvs[i]) srvs[i]->Release();
	for (int i = 0; i < 128; i++) if (vsrvs[i]) vsrvs[i]->Release();
}

void DC_WaitDeferred(int Index)
{
	ID3D11CommandList *list;

	// While (thread's command list pointer is null) - atomic version that zeros the thread's pointer
	{
		ProfileTimer("Waiting for command list completion");

		do
		{
			list = g_ThreadData[Index].CommandList.load();
		} while (list == nullptr);

		g_ThreadData[Index].CommandList.store(nullptr);
	}

	devContext->ExecuteCommandList(list, TRUE);
	list->Release();

	//memcpy(GetMainGlobals(), g_ThreadData[0].ThreadGlobals, sizeof(BSGraphicsRendererGlobals));
}