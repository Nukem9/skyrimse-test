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

	GetMOC()->ComputePixelDepthBuffer(rawDepthPixels, false);

	D3D11_MAPPED_SUBRESOURCE resource;
	if (SUCCEEDED(Context->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
	{
		// Write directly to D3D11-allocated memory
		DepthColorize(rawDepthPixels, (uint8_t *)resource.pData);
		Context->Unmap(Texture, 0);
	}
}

void MOC_ThreadedMerger::DepthColorize(const float *FloatData, uint8_t *OutColorArray)
{
	int floatCount = m_RenderWidth * m_RenderHeight;

	// Find min/max w coordinate (discard cleared pixels)
	__m128 minsW = _mm_set1_ps(FLT_MAX);
	__m128 maxsW = _mm_setzero_ps();

	for (int i = 0; i < floatCount; i += 4)
	{
		__m128 data = _mm_loadu_ps(&FloatData[i]);

		// Maximum is unconditionally calculated (always > 0.0f)
		maxsW = _mm_max_ps(maxsW, data);

		// if (FloatData[] > 0.0f) minW = min(minW, FloatData[]);
		minsW = _mm_min_ps(minsW, _mm_blendv_ps(minsW, data, _mm_cmpgt_ps(data, _mm_setzero_ps())));
	}

	float tempMinW = std::min(minsW.m128_f32[0], std::min(minsW.m128_f32[1], std::min(minsW.m128_f32[2], minsW.m128_f32[3])));
	float tempMaxW = std::max(maxsW.m128_f32[0], std::max(maxsW.m128_f32[1], std::max(maxsW.m128_f32[2], maxsW.m128_f32[3])));

	minsW = _mm_set1_ps(tempMinW);
	maxsW = _mm_set1_ps(tempMaxW);

	const __m128 maxMinDifference = _mm_sub_ps(maxsW, minsW);
	const __m128 multModifier = _mm_set1_ps(223.0f);
	const __m128 addModifier = _mm_set1_ps(32.0f);
	const __m128i intsToBytesMask = _mm_setr_epi8(0, 0, 0, 0, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12);

	// Tone map depth values
	for (int i = 0; i < floatCount; i += 4)
	{
		// if (FloatData[] > 0.0f) intensity = (unsigned char)(223.0 * (FloatData[] - minW) / (maxW - minW) + 32.0);
		__m128 data = _mm_loadu_ps(&FloatData[i]);

		__m128 x;
		x = _mm_mul_ps(_mm_sub_ps(data, minsW), multModifier);			// 223.0 * (FloatData[] - minW)
		x = _mm_add_ps(_mm_div_ps(x, maxMinDifference), addModifier);	// 223.0 * (FloatData[] - minW) / (maxW - minW) + 32.0
		x = _mm_blendv_ps(_mm_setzero_ps(), x, _mm_cmpgt_ps(data, _mm_setzero_ps()));

		// Convert 4 RGB integers to 16 RGB bytes: 4 equal values each (x, y, z, w) -> (x, x, x, x, y, y, y, y, z, z, z, z, w, w, w, w)
		_mm_storeu_si128((__m128i *)&OutColorArray[i * 4], _mm_shuffle_epi8(_mm_cvttps_epi32(x), intsToBytesMask));
	}
}

void MOC_ThreadedMerger::CullThread(uint32_t ThreadIndex)
{
	SetThreadName(GetCurrentThreadId(), "MOC_ThreadedMerger Worker");

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