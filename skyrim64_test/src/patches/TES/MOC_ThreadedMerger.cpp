#include <thread>
#include "../../common.h"
#include "MOC_ThreadedMerger.h"

MOC_ThreadedMerger::MOC_ThreadedMerger(uint32_t Width, uint32_t Height, uint32_t Threads, bool EnableCPUConservation)
{
	m_RenderWidth = Width;
	m_RenderHeight = Height;
	m_ThreadCount = Threads;

	if (EnableCPUConservation)
		m_EarlySignalEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	else
		m_EarlySignalEvent = nullptr;

	m_EarlySignalStack.store(0);

	m_TraverseSceneCallback = nullptr;
	m_RenderGeometryCallback = nullptr;

	m_MOCInstances.resize(Threads);
	m_ThreadInitialized = std::vector<std::atomic_bool>(m_ThreadCount);
	m_ThreadWorking = std::vector<std::atomic_bool>(m_ThreadCount);

	for (uint32_t i = 0; i < m_ThreadCount; i++)
	{
		std::thread cullThread(&MOC_ThreadedMerger::CullThread, this, i);
		cullThread.detach();
	}
}

MOC_ThreadedMerger::~MOC_ThreadedMerger()
{
	for (uint32_t i = 0; i < m_ThreadCount; i++)
	{
		CullPacket p;
		p.UserData = nullptr;
		p.Type = CULL_TERMINATE_THREAD;

		m_PendingPackets.push(p);
	}
	
	CloseHandle(m_EarlySignalEvent);

	// Each thread will free remaining MaskedOcclusionCulling pointers. They will also exit before
	// setting ThreadWorking to false.
	for (uint32_t i = 0; i < m_ThreadCount; i++)
	{
		while (m_ThreadWorking[i].load())
			_mm_pause();
	}
}

void MOC_ThreadedMerger::SetTraverseSceneCallback(void(*Callback)(MaskedOcclusionCulling *MOC, void *UserData))
{
	InterlockedExchangePointer((volatile PVOID *)&m_TraverseSceneCallback, Callback);
}

void MOC_ThreadedMerger::SetRenderGeometryCallback(void(*Callback)(MaskedOcclusionCulling *MOC, void *UserData))
{
	InterlockedExchangePointer((volatile PVOID *)&m_RenderGeometryCallback, Callback);
}

void MOC_ThreadedMerger::Flush()
{
	NotifyPreWork();

	CullPacket p;
	p.UserData = nullptr;
	p.Type = CULL_FLUSH;

	m_FinalBuffer.store(nullptr);
	m_PendingPackets.push(p);

	while (!m_FinalBuffer.load())
		_mm_pause();

	ClearPreWorkNotify();
}

void MOC_ThreadedMerger::Clear()
{
	for (uint32_t i = 0; i < m_ThreadCount; i++)
		m_MOCInstances[i]->ClearBuffer();
}

void MOC_ThreadedMerger::UpdateDepthViewTexture(ID3D11DeviceContext *Context, ID3D11Texture2D *Texture)
{
	// Only allocate the buffer on demand
	static float *rawDepthPixels = new float[m_RenderWidth * m_RenderHeight];
	static uint8_t *gpuTextureData = (uint8_t *)_aligned_malloc(sizeof(char) * ((m_RenderWidth * m_RenderHeight * 4 + 31) & 0xFFFFFFE0), 32);

	GetMOC()->ComputePixelDepthBuffer(rawDepthPixels, false);
	DepthColorize(rawDepthPixels, gpuTextureData);

	Context->UpdateSubresource(Texture, 0, nullptr, gpuTextureData, m_RenderWidth * 4, 0);
}

void MOC_ThreadedMerger::DepthColorize(const float *FloatData, uint8_t *OutColorArray)
{
	int w = m_RenderWidth;
	int h = m_RenderHeight;

	// Find min/max w coordinate (discard cleared pixels)
	float minW = FLT_MAX;
	float maxW = 0.0f;

	for (int i = 0; i < w * h; i++)
	{
		if (FloatData[i] > 0.0f)
		{
			minW = std::min(minW, FloatData[i]);
			maxW = std::max(maxW, FloatData[i]);
		}
	}

	// Tone map depth values
	for (int i = 0; i < w * h; i++)
	{
		int intensity = 0;

		if (FloatData[i] > 0)
			intensity = (unsigned char)(223.0 * (FloatData[i] - minW) / (maxW - minW) + 32.0);

		OutColorArray[i * 4 + 0] = intensity;
		OutColorArray[i * 4 + 1] = intensity;
		OutColorArray[i * 4 + 2] = intensity;
		OutColorArray[i * 4 + 3] = intensity;
	}
}

void MOC_ThreadedMerger::CullThread(uint32_t ThreadIndex)
{
	MaskedOcclusionCulling *moc = MaskedOcclusionCulling::Create();
	moc->SetResolution(m_RenderWidth, m_RenderHeight);
	moc->ClearBuffer();

	m_MOCInstances[ThreadIndex] = moc;
	m_ThreadWorking[ThreadIndex].store(false);
	m_ThreadInitialized[ThreadIndex].store(true);

	CullPacket p;
	int idleCount = 0;

	while (true)
	{
		if (!m_PendingPackets.try_pop(p))
		{
			// Range from a few nanoseconds to 1-2 milliseconds
			if (idleCount <= 5)
				_mm_pause();
			else if (idleCount <= 100)
				std::this_thread::yield();
			else if (m_EarlySignalEvent)
				WaitForSingleObject(m_EarlySignalEvent, 1);

			idleCount++;
			continue;
		}

		// Now doing some kind of operation
		m_ThreadWorking[ThreadIndex].store(true);
		idleCount = 0;

	__fastloop:
		switch (p.Type)
		{
		case CULL_TRAVERSE_SCENE:
		{
			if (m_TraverseSceneCallback)
				m_TraverseSceneCallback(moc, p.UserData);
		}
		break;

		case CULL_RENDER_GEOMETRY:
		{
			if (m_RenderGeometryCallback)
				m_RenderGeometryCallback(moc, p.UserData);
		}
		break;

		case CULL_FLUSH:
		{
			// Merge the buffer from every other thread into this one
			while (true)
			{
				int threadsMerged = 1;

				for (uint32_t i = 0; i < m_ThreadCount; i++)
				{
					if (i == ThreadIndex)
						continue;

					if (m_ThreadWorking[i].load())
						continue;

					moc->MergeBuffer(m_MOCInstances[i]);
					threadsMerged++;
				}

				if (threadsMerged == m_ThreadCount)
					break;
			}

			// Notify whoever was waiting for this
			m_FinalBuffer.store(moc);
		}
		break;

		case CULL_TERMINATE_THREAD:
		{
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!
			// !!!! THREAD EXITS HERE !!!!
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!
			MaskedOcclusionCulling::Destroy(moc);
			return;
		}
		break;
		}

		if (m_PendingPackets.try_pop(p))
			goto __fastloop;

		m_ThreadWorking[ThreadIndex].store(false);
	}
}