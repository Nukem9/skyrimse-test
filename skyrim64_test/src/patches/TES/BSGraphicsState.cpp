#include "../../common.h"
#include "BSGraphicsState.h"

namespace BSGraphics
{
	CameraStateData::CameraStateData()
	{
		Assert(false);

		memset(this, 0, sizeof(CameraStateData));

		// xmmword_143052BF0 = xmmword_141666D50;
		// dword_143052C04 = 0x3F800000;
		// dword_143052C00 = 0;
		UseJitter = true;
	}

	CameraStateData *State::FindCameraDataCache(const NiCamera *Camera, bool UseJitter)
	{
		for (uint32_t i = 0; i < kCameraDataCacheA.QSize(); i++)
		{
			if (kCameraDataCacheA[i].pReferenceCamera != Camera || kCameraDataCacheA[i].UseJitter != UseJitter)
				continue;

			return &kCameraDataCacheA[i];
		}

		return nullptr;
	}

	void State::SetCameraData(const NiCamera *Camera, uint32_t StateFlags)
	{
		Assert(false);

		/*
		static CameraStateData globalStateData;

		CameraStateData *data = FindCameraDataCache(Camera, StateFlags & 1);

		if (!data)
		{
			data = &globalStateData;
			BuildCameraStateData(data, Camera, StateFlags & 1);
		}

		ApplyCameraStateData(data, Camera->m_kPort, StateFlags);
		*/
	}
}