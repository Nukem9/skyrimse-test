#pragma once

#include "..\config.h"

#if !SKYRIM64_CREATIONKIT_ONLY

namespace ui
{
	void RenderFrameStatistics();
	void RenderRenderTargetMenu();
	void RenderOcclusionCullingMenu();
	void RenderSceneGraphWindows();

	namespace detail
	{
		bool InitializeNVAPI();
		float CalculateTrueAverageFPS();
		uint64_t GetSystemCpuTime();
		float GetProcessorUsagePercent();
		float GetThreadUsagePercent();
		float GetGpuUsagePercent(int GpuIndex);
	}
}

#endif