#include "../common.h"
#include "../patches/dinput8.h"
#include "../patches/TES/Setting.h"
#include "../patches/rendering/GpuTimer.h"
#include "../patches/TES/BSShader/BSShaderRenderTargets.h"
#include "imgui_ext.h"
#include "ui.h"

extern LARGE_INTEGER g_FrameDelta;
extern std::vector<std::pair<ID3D11ShaderResourceView *, std::string>> g_ResourceViews;

namespace ui
{
	int64_t LastFrame;
	int64_t TickSum;
	int64_t TickDeltas[32];
	int TickDeltaIndex;

	double CalculateTrueAverageFPS()
	{
		// This includes the overhead from calling Present() (backbuffer flip)
		LARGE_INTEGER ticksPerSecond;
		QueryPerformanceFrequency(&ticksPerSecond);

		LARGE_INTEGER frameEnd;
		QueryPerformanceCounter(&frameEnd);

		if (LastFrame == 0)
			LastFrame = frameEnd.QuadPart;

		int64_t delta = frameEnd.QuadPart - LastFrame;
		LastFrame = frameEnd.QuadPart;

		TickSum -= TickDeltas[TickDeltaIndex];
		TickSum += delta;
		TickDeltas[TickDeltaIndex++] = delta;

		if (TickDeltaIndex >= ARRAYSIZE(TickDeltas))
			TickDeltaIndex = 0;

		double averageFrametime = (TickSum / 32.0) / (double)ticksPerSecond.QuadPart;
		return 1.0 / averageFrametime;
	}

	//
	// Frame statistics window
	//
	float DeltasFrameTime[240];
	float DeltasCPUPercent[240];
	float DeltasThreadPercent[240];
	float DeltasGPU0Percent[240];
	float DeltasGPU1Percent[240];
	float DeltasFrameTimeGPU[240];
	float DeltasGraphDrawCalls[240];
	float DeltasGraphDispatchCalls[240];

	void RenderFrameStatistics()
	{
		// Always calculate the frame time even if the window isn't visible
		LARGE_INTEGER ticksPerSecond;
		QueryPerformanceFrequency(&ticksPerSecond);

		double frameTimeMs = 1000.0 * (g_FrameDelta.QuadPart / (double)ticksPerSecond.QuadPart);

		if (ui::opt::LogHitches && frameTimeMs >= 50.0)
			ui::log::Add("FRAME HITCH WARNING (%g ms)\n", frameTimeMs);

		if (!showFrameStatsWindow)
			return;

		// Shift everything else back first...
		memmove(&DeltasFrameTime[0], &DeltasFrameTime[1], sizeof(DeltasFrameTime) - sizeof(float));
		memmove(&DeltasFrameTimeGPU[0], &DeltasFrameTimeGPU[1], sizeof(DeltasFrameTimeGPU) - sizeof(float));
		memmove(&DeltasCPUPercent[0], &DeltasCPUPercent[1], sizeof(DeltasCPUPercent) - sizeof(float));
		memmove(&DeltasThreadPercent[0], &DeltasThreadPercent[1], sizeof(DeltasThreadPercent) - sizeof(float));
		memmove(&DeltasGPU0Percent[0], &DeltasGPU0Percent[1], sizeof(DeltasGPU0Percent) - sizeof(float));
		memmove(&DeltasGPU1Percent[0], &DeltasGPU1Percent[1], sizeof(DeltasGPU1Percent) - sizeof(float));
		memmove(&DeltasGraphDrawCalls[0], &DeltasGraphDrawCalls[1], sizeof(DeltasGraphDrawCalls) - sizeof(float));
		memmove(&DeltasGraphDispatchCalls[0], &DeltasGraphDispatchCalls[1], sizeof(DeltasGraphDispatchCalls) - sizeof(float));

		if (ImGui::Begin("Frame Statistics", &showFrameStatsWindow))
		{
			// Draw frame time graph
			{
				DeltasFrameTime[239] = frameTimeMs;
				DeltasFrameTimeGPU[239] = g_GPUTimers.GetGPUTimeInMS(0);

				const char *names[2] = { "CPU", "GPU" };
				ImColor colors[2] = { ImColor(0.839f, 0.152f, 0.156f), ImColor(0.172f, 0.627f, 0.172f) };
				void *datas[2] = { (void *)DeltasFrameTime, (void *)DeltasFrameTimeGPU };

				ImGui::PlotMultiLines("Frame Time (ms)\n** Minus Present() flip", 2, names, colors, [](const void *a, int idx) { return ((float *)a)[idx]; }, datas, 240, 0.0f, 32.0f, ImVec2(400, 100));
			}

			// Draw processor usage (CPU, GPU) graph
			{
				DeltasCPUPercent[239] = Profiler::GetProcessorUsagePercent();
				DeltasThreadPercent[239] = Profiler::GetThreadUsagePercent();
				DeltasGPU0Percent[239] = Profiler::GetGpuUsagePercent(0);
				DeltasGPU1Percent[239] = Profiler::GetGpuUsagePercent(1);

				const char *names[4] = { "CPU Total", "Main Thread", "GPU 0", "GPU 1" };
				ImColor colors[4] = { ImColor(0.839f, 0.152f, 0.156f), ImColor(0.172f, 0.627f, 0.172f), ImColor(1.0f, 0.498f, 0.054f), ImColor(0.121f, 0.466f, 0.705f) };
				void *datas[4] = { (void *)DeltasCPUPercent, (void *)DeltasThreadPercent, (void *)DeltasGPU0Percent, (void *)DeltasGPU1Percent };

				ImGui::PlotMultiLines("Processor Usage (%)", 4, names, colors, [](const void *a, int idx) { return ((float *)a)[idx]; }, datas, 240, 0.0f, 100.0f, ImVec2(400, 100));
			}

			// Draw calls
			{
				DeltasGraphDrawCalls[239] = ProfileGetDeltaValue("Draw Calls");
				DeltasGraphDispatchCalls[239] = ProfileGetDeltaValue("Dispatch Calls");

				const char *names[2] = { "Draws", "Dispatches" };
				ImColor colors[2] = { ImColor(0.839f, 0.152f, 0.156f), ImColor(0.172f, 0.627f, 0.172f) };
				void *datas[2] = { (void *)DeltasGraphDrawCalls, (void *)DeltasGraphDispatchCalls };

				ImGui::PlotMultiLines("Draw Calls", 2, names, colors, [](const void *a, int idx) { return ((float *)a)[idx]; }, datas, 240, 0.0f, 10000.0f, ImVec2(400, 100));

				ProfileGetValue("Draw Calls");
				ProfileGetValue("Dispatch Calls");
			}

			ImGui::Text("FPS: %.2f", CalculateTrueAverageFPS());
			ImGui::Spacing();
			ImGui::Text("CB Bytes Requested: %lld", ProfileGetDeltaValue("CB Bytes Requested"));
			ImGui::Text("CB Bytes Wasted: %lld", ProfileGetDeltaValue("CB Bytes Wasted"));
			ImGui::Text("VIB Bytes Requested: %lld", ProfileGetDeltaValue("VIB Bytes Requested"));

			ProfileGetValue("CB Bytes Requested");
			ProfileGetValue("VIB Bytes Requested");
			ProfileGetValue("CB Bytes Wasted");
		}
		ImGui::End();
	}

	//
	// Render target viewer window
	//
	void RenderRenderTargetMenu()
	{
		if (!showRTViewerWindow)
			return;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));
		if (ImGui::Begin("Render Target Viewer", &showRTViewerWindow))
		{
			static ImGuiTextFilter rtFilter;
			static int selectedIndex;

			ImGui::PushItemWidth(-1);
			ImGui::ListBoxVector<decltype(g_ResourceViews)>("##rtbox", "Filter", &rtFilter, g_ResourceViews, &selectedIndex, [](const decltype(g_ResourceViews) *Vec, size_t Index)
			{
				return Vec->at(Index).second.c_str();
			}, 10);
			ImGui::PopItemWidth();

			if (selectedIndex != -1 && g_ResourceViews[selectedIndex].first)
				ImGui::Image((ImTextureID)g_ResourceViews[selectedIndex].first, ImVec2(1024, 768));
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}
}