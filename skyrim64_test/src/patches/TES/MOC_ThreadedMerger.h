#pragma once

#include <atomic>
#include "../../common.h"
#include "../../../MaskedOcclusionCulling/MaskedOcclusionCulling.h"
#include "../../../tbb2018/concurrent_queue.h"

class MOC_ThreadedMerger
{
private:
	enum CullType
	{
		CULL_TRAVERSE_SCENE,
		CULL_RENDER_GEOMETRY,
		CULL_FLUSH,
		CULL_TERMINATE_THREAD,
	};

	struct CullPacket
	{
		CullType Type;
		void *UserData;
	};

	uint32_t m_RenderWidth;
	uint32_t m_RenderHeight;
	uint32_t m_ThreadCount;
	HANDLE m_EarlySignalEvent;
	std::atomic_uint m_EarlySignalStack;

	void (*m_TraverseSceneCallback)(MaskedOcclusionCulling *MOC, void *UserData);
	void (*m_RenderGeometryCallback)(MaskedOcclusionCulling *MOC, void *UserData);

	std::vector<MaskedOcclusionCulling *> m_MOCInstances;	// Each thread has a MaskedOcclusionCulling instance
	std::vector<std::atomic_bool> m_ThreadInitialized;		// True if thread is ready
	std::vector<std::atomic_bool> m_ThreadWorking;			// True is thread is processing something
	tbb::concurrent_queue<CullPacket> m_PendingPackets;		// Queue of packets that each thread accesses

	std::atomic<MaskedOcclusionCulling *> m_FinalBuffer;	// Final scene depth buffer after calling Flush()

public:
	MOC_ThreadedMerger(uint32_t Width, uint32_t Height, uint32_t Threads = 1, bool EnableCPUConservation = true);
	~MOC_ThreadedMerger();

	void SetTraverseSceneCallback(void(*Callback)(MaskedOcclusionCulling *MOC, void *UserData));
	void SetRenderGeometryCallback(void(*Callback)(MaskedOcclusionCulling *MOC, void *UserData));

	void Flush();
	void Clear();
	void UpdateDepthViewTexture(ID3D11DeviceContext *Context, ID3D11Texture2D *Texture);

	__forceinline void SubmitSceneRender(void *UserData)
	{
		CullPacket p;
		p.UserData = UserData;
		p.Type = CULL_TRAVERSE_SCENE;

		m_PendingPackets.push(p);
	}

	__forceinline void SubmitGeometry(void *UserData)
	{
		CullPacket p;
		p.UserData = UserData;
		p.Type = CULL_RENDER_GEOMETRY;

		m_PendingPackets.push(p);
	}

	__forceinline MaskedOcclusionCulling *GetMOC()
	{
		return m_FinalBuffer.load();
	}

	__forceinline void NotifyPreWork()
	{
		if (m_EarlySignalEvent)
		{
			SetEvent(m_EarlySignalEvent);
			m_EarlySignalStack++;
		}
	}

	__forceinline void ClearPreWorkNotify()
	{
		if (m_EarlySignalEvent && --m_EarlySignalStack == 0)
			ResetEvent(m_EarlySignalEvent);
	}

private:
	void DepthColorize(const float *FloatData, uint8_t *OutColorArray);
	void CullThread(uint32_t ThreadIndex);
};