#pragma once

#include "../../common.h"

class GPUTimer
{
private:
	struct GPUTimerState
	{
		bool TimestampQueryInFlight;
		ID3D11Query *GPUTimerBegin;
		ID3D11Query *GPUTimerEnd;
		float GPUTimeInMS;
	};

public:
	void Create(ID3D11Device *D3DDevice, uint32_t NumTimers);
	void Release();

	void BeginFrame(ID3D11DeviceContext *DeviceContext);
	void EndFrame(ID3D11DeviceContext *DeviceContext);

	void StartTimer(ID3D11DeviceContext *DeviceContext, uint32_t Id);
	void StopTimer(ID3D11DeviceContext *DeviceContext, uint32_t Id);

	float GetGPUTimeInMS(uint32_t Id);

protected:
	bool m_DisjointQueryInFlight;
	ID3D11Query *m_DisjointTimestampQuery;
	std::vector<GPUTimerState> m_Timers;
};

extern GPUTimer g_GPUTimers;