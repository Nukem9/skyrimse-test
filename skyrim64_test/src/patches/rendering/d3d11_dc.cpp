#include "common.h"

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

	ID3D11CommandList *CommandList;
	BSGraphicsRendererGlobals *ThreadGlobals;
};

ThreadData g_ThreadData[4];

ID3D11DeviceContext2 *devContext;

thread_local bool m_TestBuffer;

uintptr_t AllocateGuardedBlock();

DWORD WINAPI DC_Thread(LPVOID Arg)
{
	// Note: This thread is always running with the original TLS slot
	//TLS_RevertThread();

	*(uintptr_t *)(__readgsqword(0x58) + g_TlsIndex * sizeof(void *)) = AllocateGuardedBlock();

	SetThreadName(GetCurrentThreadId(), "DC_Thread");

	ThreadData *jobData = (ThreadData *)Arg;
	BSGraphicsRendererGlobals *oldData = GetMainGlobals();		// Main thread
	BSGraphicsRendererGlobals *newData = GetThreadedGlobals();	// This thread

	jobData->CommandList = nullptr;
	jobData->ThreadGlobals = newData;
	SetEvent(jobData->InitEvent);

	while (true)
	{
		if (WaitForSingleObject(jobData->RunEvent, INFINITE) == WAIT_FAILED)
			__debugbreak();

		// Sanity check: We should be using the original TLS slot now
		if (newData != GetThreadedGlobals() || newData == GetMainGlobals())
			__debugbreak();
		
		if (oldData == GetThreadedGlobals() || oldData != GetMainGlobals())
			__debugbreak();

		// TLS this thread <==> TLS main thread
		if (newData == oldData)
			__debugbreak();

		newData->m_DeviceContext = jobData->DeferredContext;
		m_TestBuffer = true;

		jobData->Callback(jobData->a1, jobData->a2);
		jobData->DeferredContext->FinishCommandList(FALSE, &jobData->CommandList);

		SetEvent(jobData->CompletedEvent);
	}

	return 0;
}

void DC_Init(ID3D11DeviceContext2 *ImmediateContext, ID3D11DeviceContext2 **DeferredContexts, int DeferredContextCount)
{
	devContext = ImmediateContext;

	for (int i = 0; i < DeferredContextCount; i++)
	{
		g_ThreadData[i].InitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		g_ThreadData[i].RunEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		g_ThreadData[i].CompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		g_ThreadData[i].DeferredContext = DeferredContexts[i];

		if (!g_ThreadData[i].RunEvent || !g_ThreadData[i].CompletedEvent)
			__debugbreak();

		CreateThread(nullptr, 0, DC_Thread, &g_ThreadData[i], 0, nullptr);
	}

	// Wait for threads to set up initial state
	for (int i = 0; i < DeferredContextCount; i++)
		WaitForSingleObject(g_ThreadData[i].InitEvent, INFINITE);
}

void DC_RenderDeferred(__int64 a1, unsigned int a2, void(*func)(__int64, unsigned int))
{
	g_ThreadData[0].a1 = a1;
	g_ThreadData[0].a2 = a2;
	g_ThreadData[0].Callback = func;

	D3D_PRIMITIVE_TOPOLOGY topo;
	ID3D11RenderTargetView *rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	ID3D11DepthStencilView *dsv = nullptr;
	ID3D11Buffer *psbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *vsbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *csbuf[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11ShaderResourceView *srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView *vsrvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11SamplerState *pssamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *vssamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *cssamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11UnorderedAccessView *views[D3D11_1_UAV_SLOT_COUNT];
	ID3D11Buffer *vertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11Buffer *indexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];

	memset(cssamplers, 0, sizeof(cssamplers));
	memset(views, 0, sizeof(views));

	//devContext->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, vertexBuffers, nullptr, nullptr);
	devContext->IAGetPrimitiveTopology(&topo);
	//devContext->CSGetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, views);
	devContext->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pssamplers);
	devContext->VSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, vssamplers);
	devContext->CSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, cssamplers);
	devContext->PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	devContext->VSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, vsrvs);
	devContext->PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, psbuf);
	devContext->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vsbuf);
	devContext->CSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, csbuf);
	devContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtvs, &dsv);

	auto dc1 = g_ThreadData[0].DeferredContext;

	__try
	{
		//for (int i = 0; i < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; i++) if (vertexBuffers[i]) dc1->IASetVertexBuffers(i, 1, &vertexBuffers[i], nullptr, nullptr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
	dc1->IASetPrimitiveTopology(topo);
	dc1->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtvs, dsv);
	dc1->PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, psbuf);
	dc1->VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vsbuf);
	dc1->CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, csbuf);
	dc1->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);
	dc1->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, vsrvs);
	dc1->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pssamplers);
	dc1->VSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, vssamplers);
	dc1->CSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, cssamplers);
	//dc1->CSSetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, views, nullptr);

	memcpy(g_ThreadData[0].ThreadGlobals, GetMainGlobals(), 0x4000);

	// Run the other thread ASAP to prevent delaying whoever called this
	SetEvent(g_ThreadData[0].RunEvent);

	//for (int i = 0; i < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; i++) if (vertexBuffers[i]) vertexBuffers[i]->Release();
	for (int i = 0; i < 8; i++) if (rtvs[i]) rtvs[i]->Release();
	if (dsv) dsv->Release();
	for (int i = 0; i < 14; i++) if (psbuf[i]) psbuf[i]->Release();
	for (int i = 0; i < 14; i++) if (vsbuf[i]) vsbuf[i]->Release();
	for (int i = 0; i < 14; i++) if (csbuf[i]) csbuf[i]->Release();
	for (int i = 0; i < 128; i++) if (srvs[i]) srvs[i]->Release();
	for (int i = 0; i < 128; i++) if (vsrvs[i]) vsrvs[i]->Release();
	for (int i = 0; i < 16; i++) if (pssamplers[i]) pssamplers[i]->Release();
	for (int i = 0; i < 16; i++) if (vssamplers[i]) vssamplers[i]->Release();
	for (int i = 0; i < 16; i++) if (cssamplers[i]) cssamplers[i]->Release();
	for (int i = 0; i < 64; i++) if (views[i]) views[i]->Release();
}

void DC_WaitDeferred()
{
	if (WaitForSingleObject(g_ThreadData[0].CompletedEvent, INFINITE) == WAIT_FAILED)
		__debugbreak();

	devContext->ExecuteCommandList(g_ThreadData[0].CommandList, TRUE);
	g_ThreadData[0].CommandList->Release();
	g_ThreadData[0].CommandList = nullptr;

	//memcpy(GetMainGlobals(), g_ThreadData[0].ThreadGlobals, 0x4000);
}