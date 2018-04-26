#include "../common.h"
#include "../patches/dinput8.h"
#include "imgui_ext.h"
#include "imgui_impl_dx11.h"
#include "../patches/TES/BSShader/BSShader.h"
#include "../patches/TES/Setting.h"
#include "../patches/rendering/GpuTimer.h"

namespace ui::opt
{
    bool EnableCache = true;
	bool LogHitches = true;
}

int commandThreshold = 500;
int commandThreshold2 = 500;

extern LARGE_INTEGER g_FrameDelta;

namespace ui
{
	bool showDemoWindow;
    bool showFPSWindow;
    bool showTESFormWindow;
    bool showLockWindow;
    bool showMemoryWindow;
	bool showShaderTweakWindow;
	bool showIniListWindow;
	bool showLogWindow;

    char *format_commas(int64_t n, char *out)
    {
        int c;
        char buf[100];
        char *p;
        char *q = out; // Backup pointer for return...

        if (n < 0)
        {
            *out++ = '-';
            n      = abs(n);
        }

        snprintf(buf, 100, "%lld", n);
        c = 2 - strlen(buf) % 3;

        for (p = buf; *p != 0; p++)
        {
            *out++ = *p;
            if (c == 1)
            {
                *out++ = ',';
            }
            c = (c + 1) % 3;
        }
        *--out = 0;

        return q;
    }

    void Initialize(HWND Wnd, ID3D11Device *Device, ID3D11DeviceContext *DeviceContext)
    {
		ImGui::CreateContext();
        ImGui_ImplDX11_Init(Wnd, Device, DeviceContext);

		ImGui::StyleColorsDark();
		ImGui::GetStyle().ChildRounding = 0.0f;
		ImGui::GetStyle().FrameRounding = 0.0f;
		ImGui::GetStyle().GrabRounding = 0.0f;
		ImGui::GetStyle().PopupRounding = 0.0f;
		ImGui::GetStyle().ScrollbarRounding = 0.0f;
		ImGui::GetStyle().WindowRounding = 0.0f;
		ImGui::GetIO().MouseDrawCursor = true;
    }

    void HandleInput(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
    {
		ImGui_ImplWin32_WndProcHandler(Wnd, Msg, wParam, lParam);
    }

    void Render()
    {
        ImGui_ImplDX11_NewFrame();
        {
            RenderMenubar();
			RenderFramerate();
            RenderSynchronization();
            RenderTESFormCache();
            RenderMemory();
			RenderShaderTweaks();
			RenderINITweaks();

			if (showDemoWindow)
				ImGui::ShowDemoWindow(&showDemoWindow);

            if (showLogWindow)
                log::Draw();
        }
        ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void RenderMenubar()
    {
        if (!ImGui::BeginMainMenuBar())
            return;

        // Empty space for MSI afterburner display
        if (ImGui::BeginMenu("                        ", false))
            ImGui::EndMenu();

        // Module info, all disabled
        if (ImGui::BeginMenu("Modules"))
        {
            ImGui::MenuItem("X3DAudio HRTF", g_Dll3DAudio ? "Detected" : "Not Detected");
            ImGui::MenuItem("ReShade", g_DllReshade ? "Detected" : "Not Detected");
            ImGui::MenuItem("ENB", g_DllEnb ? "Detected" : "Not Detected");
            ImGui::MenuItem("SKSE64", g_DllSKSE ? "Detected" : "Not Detected");
            ImGui::MenuItem("VTune", g_DllVTune ? "Detected" : "Not Detected");
            ImGui::EndMenu();
        }

#if SKYRIM64_USE_VTUNE
		if (g_DllVTune && ImGui::BeginMenu("VTune"))
		{
			if (ImGui::MenuItem("Start Collection"))
				__itt_resume();

			if (ImGui::MenuItem("Stop Collection"))
				__itt_pause();

			if (ImGui::MenuItem("Detach"))
				__itt_detach();

			ImGui::EndMenu();
		}
#endif

        if (ImGui::BeginMenu("Windows"))
        {
			ImGui::MenuItem("ImGui Debug", nullptr, &showDemoWindow);
			ImGui::MenuItem("Debug Log", nullptr, &showLogWindow);
            ImGui::MenuItem("Framerate", nullptr, &showFPSWindow);
            ImGui::MenuItem("Synchronization", nullptr, &showLockWindow);
            ImGui::MenuItem("Memory", nullptr, &showMemoryWindow);
            ImGui::MenuItem("TESForm Cache", nullptr, &showTESFormWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Game"))
        {
			ImGui::MenuItem("Log Frame Hitches", nullptr, &opt::LogHitches);

            bool g_BlockInput = !ProxyIDirectInputDevice8A::GlobalInputAllowed();

            if (ImGui::MenuItem("Block Input", nullptr, &g_BlockInput))
                ProxyIDirectInputDevice8A::ToggleGlobalInput(!g_BlockInput);

			ImGui::MenuItem("Shader Tweaks", nullptr, &showShaderTweakWindow);
			ImGui::MenuItem("INISetting Viewer", nullptr, &showIniListWindow);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

	void RenderINITweaks()
	{
		if (!showIniListWindow)
			return;

		if (ImGui::Begin("INISetting Viewer", &showIniListWindow))
		{
			static ImGuiTextFilter iniFilter;
			static int selectedIndex;

			iniFilter.Draw("Filter", -100.0f);

			// Filter the listbox input before we send it to imgui - which doesn't support dynamic filtering
			std::vector<Setting *> settingList;

			for (auto *s = INISettingCollectionSingleton->SettingsA.QNext(); s; s = s->QNext())
			{
				if (iniFilter.IsActive())
				{
					if (iniFilter.PassFilter(s->QItem()->pKey))
						settingList.push_back(s->QItem());
				}
				else
				{
					// No filter present
					settingList.push_back(s->QItem());
				}
			}

			for (auto *s = INIPrefSettingCollectionSingleton->SettingsA.QNext(); s; s = s->QNext())
			{
				if (iniFilter.IsActive())
				{
					if (iniFilter.PassFilter(s->QItem()->pKey))
						settingList.push_back(s->QItem());
				}
				else
				{
					// No filter present
					settingList.push_back(s->QItem());
				}
			}

			ImGui::PushItemWidth(-1);

			// Draw the list itself
			ImGui::ListBox("##inibox", &selectedIndex, [](void *data, int index, const char **out)
			{
				*out = reinterpret_cast<std::vector<Setting *> *>(data)->at(index)->pKey;
				return true;
			}, (void *)&settingList, settingList.size(), 16);

			// Now the editor inputs
			if (ImGui::BeginGroupSplitter("Selection") && selectedIndex < settingList.size())
			{
				Setting *s = settingList.at(selectedIndex);

				ImGui::PushItemWidth(60);
				ImGui::LabelText("##lblIniVar", "Variable:");
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::InputText("##txtIniVar", (char *)s->pKey, 0, ImGuiInputTextFlags_ReadOnly);

				char tempBuffer[512];
				s->GetAsString(tempBuffer, ARRAYSIZE(tempBuffer));

				ImGui::PushItemWidth(60);
				ImGui::LabelText("##lblIniValue", "Value:");
				ImGui::PopItemWidth();
				ImGui::SameLine();

				// Update the setting on user request
				if (ImGui::InputText("##txtIniValue", tempBuffer, ARRAYSIZE(tempBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
					s->SetFromString(tempBuffer);

				ImGui::LabelText("##lblNotice", "Press enter to save inputs. String editing is disabled.");
				ImGui::EndGroupSplitter();
			}

			ImGui::PopItemWidth();
		}

		ImGui::End();
	}

	int64_t LastFrame;
	int64_t TickSum;
	int64_t TickDeltas[32];
	int TickDeltaIndex;

	float DeltasGraph[240];
	float DeltasGraph2[240];
	float DeltasGraph3[240];
	float DeltasGraph4[240];
	float DeltasGraph5[240];
	float DeltasGraphGPU[240];

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

    void RenderFramerate()
    {
		// Always calculate the frame time even if the window isn't visible
		LARGE_INTEGER ticksPerSecond;
		QueryPerformanceFrequency(&ticksPerSecond);

		double frameTimeMs = 1000.0 * (g_FrameDelta.QuadPart / (double)ticksPerSecond.QuadPart);

		if (ui::opt::LogHitches && frameTimeMs >= 50.0)
			ui::log::Add("FRAME HITCH WARNING (%g ms)\n", frameTimeMs);

        if (!showFPSWindow)
            return;

		// Shift everything else back first...
		memmove(&DeltasGraph[0], &DeltasGraph[1], sizeof(DeltasGraph) - sizeof(float));
		memmove(&DeltasGraph2[0], &DeltasGraph2[1], sizeof(DeltasGraph2) - sizeof(float));
		memmove(&DeltasGraph3[0], &DeltasGraph3[1], sizeof(DeltasGraph3) - sizeof(float));
		memmove(&DeltasGraph4[0], &DeltasGraph4[1], sizeof(DeltasGraph4) - sizeof(float));
		memmove(&DeltasGraph5[0], &DeltasGraph5[1], sizeof(DeltasGraph5) - sizeof(float));
		memmove(&DeltasGraphGPU[0], &DeltasGraphGPU[1], sizeof(DeltasGraphGPU) - sizeof(float));

        float test = 0.0f; // *(float *)(g_ModuleBase + 0x1DADCA0);

        if (ImGui::Begin("Framerate", &showFPSWindow))
        {
			// Draw frame time graph
			{
				DeltasGraph[239] = frameTimeMs;
				DeltasGraphGPU[239] = g_GPUTimers.GetGPUTimeInMS(0);

				const char *names[2] = { "CPU", "GPU" };
				ImColor colors[2] = { ImColor(0.839f, 0.152f, 0.156f), ImColor(0.172f, 0.627f, 0.172f) };
				void *datas[2] = { (void *)DeltasGraph, (void *)DeltasGraphGPU };

				ImGui::PlotMultiLines("Frame Time (ms)\n** Minus Present() flip", 2, names, colors, [](const void *a, int idx) { return ((float *)a)[idx]; }, datas, 240, 0.0f, 32.0f, ImVec2(400, 100));
			}

			// Draw processor usage (CPU, GPU) graph
			{
				DeltasGraph2[239] = Profiler::GetProcessorUsagePercent();
				DeltasGraph3[239] = Profiler::GetThreadUsagePercent();
				DeltasGraph4[239] = Profiler::GetGpuUsagePercent(0);
				DeltasGraph5[239] = Profiler::GetGpuUsagePercent(1);

				const char *names[4] = { "CPU Total", "Main Thread", "GPU 0", "GPU 1" };
				ImColor colors[4] = { ImColor(0.839f, 0.152f, 0.156f), ImColor(0.172f, 0.627f, 0.172f), ImColor(1.0f, 0.498f, 0.054f), ImColor(0.121f, 0.466f, 0.705f) };
				void *datas[4] = { (void *)DeltasGraph2, (void *)DeltasGraph3, (void *)DeltasGraph4, (void *)DeltasGraph5 };

				ImGui::PlotMultiLines("Processor Usage (%)", 4, names, colors, [](const void *a, int idx) { return ((float *)a)[idx]; }, datas, 240, 0.0f, 100.0f, ImVec2(400, 100));
			}

            ImGui::Text("FPS: %.2f", CalculateTrueAverageFPS());
            ImGui::Spacing();
			ImGui::InputInt("Command Threshold", &commandThreshold, 10, 100);
			ImGui::InputInt("Command Threshold Min", &commandThreshold2, 10, 100);
            ImGui::Text("Havok fMaxTime: %.2f FPS", 1.0f / test);
            ImGui::Text("Havok fMaxTimeComplex: %.2f FPS", 1.0f / test);
			ImGui::Spacing();
			ImGui::Text("CB Bytes Requested: %lld", ProfileGetDeltaValue("CB Bytes Requested"));
			ImGui::Text("VIB Bytes Requested: %lld", ProfileGetDeltaValue("VIB Bytes Requested"));
			ImGui::Text("CB Bytes Wasted: %lld", ProfileGetDeltaValue("CB Bytes Wasted"));
			ImGui::Spacing();
			ImGui::Text("Generating game command lists: %.5f ms", ProfileGetDeltaTime("GameCommandList") * 1000);
			ImGui::Text("Generating D3D11 command lists: %.5f ms", ProfileGetDeltaTime("GameCommandListToD3D") * 1000);
			ImGui::Text("Waiting for command list completion: %.5f ms", ProfileGetDeltaTime("Waiting for command list completion") * 1000);

			ImGui::Text("T1: %.5f ms", ProfileGetDeltaTime("T1") * 1000);
			ImGui::Text("RenderBatches: %.5f ms", ProfileGetDeltaTime("RenderBatches") * 1000);
			ImGui::Text("LowAniso: %.5f ms", ProfileGetDeltaTime("LowAniso") * 1000);
			ImGui::Text("RenderGrass: %.5f ms", ProfileGetDeltaTime("RenderGrass") * 1000);
			ImGui::Text("DC_WaitDeferred: %.5f ms", ProfileGetDeltaTime("DC_WaitDeferred") * 1000);
			ImGui::Text("LOD: %.5f ms", ProfileGetDeltaTime("LOD") * 1000);
			ImGui::Text("BSShaderAccumulator: %.5f ms", ProfileGetDeltaTime("BSShaderAccumulator") * 1000);
			ImGui::Text("Spins: %lld", ProfileGetDeltaValue("Spins"));
			ImGui::Text("Command Count: %lld", ProfileGetDeltaValue("Command Count"));
			ImGui::Text("MTCommandCount Count: %lld", ProfileGetDeltaValue("DrawPassGeometry"));

			ProfileGetValue("DrawPassGeometry");

			ProfileGetValue("CB Bytes Requested");
			ProfileGetValue("VIB Bytes Requested");
			ProfileGetValue("CB Bytes Wasted");

			ProfileGetTime("GameCommandList");
			ProfileGetTime("GameCommandListToD3D");

			ProfileGetValue("Command Count");
			ProfileGetValue("Spins");
			ProfileGetTime("Waiting for command list completion");
			ProfileGetTime("T1");
			ProfileGetTime("RenderBatches");
			ProfileGetTime("LowAniso");
			ProfileGetTime("RenderGrass");
			ProfileGetTime("DC_WaitDeferred");
			ProfileGetTime("LOD");
			ProfileGetTime("BSShaderAccumulator");
		}
        ImGui::End();
    }

    void RenderSynchronization()
    {
        if (!showLockWindow)
            return;

        if (ImGui::Begin("Synchronization", &showLockWindow))
        {
            if (ImGui::BeginGroupSplitter("Per Frame"))
            {
                ImGui::Text("Time acquiring read locks: %.5f seconds", ProfileGetDeltaTime("Read Lock Time"));
                ImGui::Text("Time acquiring write locks: %.5f seconds", ProfileGetDeltaTime("Write Lock Time"));
                ImGui::EndGroupSplitter();
            }

            if (ImGui::BeginGroupSplitter("Global"))
            {
                ImGui::Text("Time acquiring read locks: %.5f seconds", ProfileGetTime("Read Lock Time"));
                ImGui::Text("Time acquiring write locks: %.5f seconds", ProfileGetTime("Write Lock Time"));
                ImGui::EndGroupSplitter();
            }
        }

        ImGui::End();
    }

    void RenderMemory()
    {
        if (!showMemoryWindow)
            return;

        if (ImGui::Begin("Memory", &showMemoryWindow))
        {
            if (ImGui::BeginGroupSplitter("Per Frame"))
            {
                ImGui::Text("Allocs: %lld", ProfileGetDeltaValue("Alloc Count"));
                ImGui::Text("Frees: %lld", ProfileGetDeltaValue("Free Count"));
                ImGui::Text("Bytes: %.3f MB", (double)ProfileGetDeltaValue("Byte Count") / 1024 / 1024);
                ImGui::Spacing();
                ImGui::Text("Time spent allocating: %.5f seconds", ProfileGetDeltaTime("Time Spent Allocating"));
                ImGui::Text("Time spent freeing: %.5f seconds", ProfileGetDeltaTime("Time Spent Freeing"));
                ImGui::EndGroupSplitter();
            }

            if (ImGui::BeginGroupSplitter("Global"))
            {
                int64_t allocCount = ProfileGetValue("Alloc Count");
                int64_t freeCount  = ProfileGetValue("Free Count");

                ImGui::Text("Allocs: %lld", allocCount);
                ImGui::Text("Frees: %lld", freeCount);
                ImGui::Text("Bytes: %.3f MB", (double)ProfileGetValue("Byte Count") / 1024 / 1024);
                ImGui::Spacing();
                ImGui::Text("Time spent allocating: %.5f seconds", ProfileGetTime("Time Spent Allocating"));
                ImGui::Text("Time spent freeing: %.5f seconds", ProfileGetTime("Time Spent Freeing"));
                ImGui::Spacing();
                ImGui::Text("Active allocations: %lld", allocCount - freeCount);
                ImGui::EndGroupSplitter();
            }
        }

        ImGui::End();
    }

    void RenderTESFormCache()
    {
        if (!showTESFormWindow)
            return;

        if (ImGui::Begin("TESForm Cache", &showTESFormWindow))
        {
            ImGui::Checkbox("Enable Cache", &opt::EnableCache);

            if (ImGui::BeginGroupSplitter("Per Frame"))
            {
                int64_t cacheLookups = ProfileGetDeltaValue("Cache Lookups");
                int64_t cacheMisses  = ProfileGetDeltaValue("Cache Misses");
                int64_t nullFetches  = ProfileGetDeltaValue("Null Fetches");

                char tempBuf[256];
                ImGui::Text("Lookups: %s", format_commas(cacheLookups, tempBuf));
                ImGui::Text("Hits: %s", format_commas(cacheLookups - cacheMisses, tempBuf));
                ImGui::Text("Misses: %s", format_commas(cacheMisses, tempBuf));
                ImGui::Spacing();
                ImGui::Text("Bitmap nullptr fetches: %s (%.2f%%)", format_commas(nullFetches, tempBuf), ((double)nullFetches / (double)cacheLookups) * 100);
                ImGui::Spacing();
                ImGui::Text("Update time: %.5f seconds", ProfileGetDeltaTime("Cache Update Time"));
                ImGui::Text("Fetch time: %.5f seconds", ProfileGetDeltaTime("Cache Fetch Time"));
                ImGui::EndGroupSplitter();
            }

            if (ImGui::BeginGroupSplitter("Global"))
            {
                int64_t cacheLookups = ProfileGetValue("Cache Lookups");
                int64_t cacheMisses  = ProfileGetValue("Cache Misses");
                int64_t nullFetches  = ProfileGetValue("Null Fetches");

                char tempBuf[256];
                ImGui::Text("Lookups: %s", format_commas(cacheLookups, tempBuf));
                ImGui::Text("Hits: %s", format_commas(cacheLookups - cacheMisses, tempBuf));
                ImGui::Text("Misses: %s", format_commas(cacheMisses, tempBuf));
                ImGui::Spacing();
                ImGui::Text("Bitmap nullptr fetches: %s (%.2f%%)", format_commas(nullFetches, tempBuf), ((double)nullFetches / (double)cacheLookups) * 100);
                ImGui::Spacing();
                ImGui::Text("Update time: %.5f seconds", ProfileGetTime("Cache Update Time"));
                ImGui::Text("Fetch time: %.5f seconds", ProfileGetTime("Cache Fetch Time"));
                ImGui::EndGroupSplitter();
            }
        }

        ImGui::End();
    }

	void RenderShaderTweaks()
	{
		if (!showShaderTweakWindow)
			return;

		if (ImGui::Begin("Shader Tweaks", &showShaderTweakWindow))
		{
			ImGui::Checkbox("Use original BSLightingShader::Technique", &BSShader::g_ShaderToggles[6][0]);
			ImGui::Checkbox("Use original BSLightingShader::Material", &BSShader::g_ShaderToggles[6][1]);
			ImGui::Checkbox("Use original BSLightingShader::Geometry", &BSShader::g_ShaderToggles[6][2]);
			ImGui::Spacing();
			ImGui::Checkbox("Use original BSGrassShader::Technique", &BSShader::g_ShaderToggles[1][0]);
			ImGui::Checkbox("Use original BSGrassShader::Material", &BSShader::g_ShaderToggles[1][1]);
			ImGui::Checkbox("Use original BSGrassShader::Geometry", &BSShader::g_ShaderToggles[1][2]);
			ImGui::Spacing();
			ImGui::Checkbox("Use original BSBloodSplatterShader::Technique", &BSShader::g_ShaderToggles[4][0]);
			ImGui::Checkbox("Use original BSBloodSplatterShader::Material", &BSShader::g_ShaderToggles[4][1]);
			ImGui::Checkbox("Use original BSBloodSplatterShader::Geometry", &BSShader::g_ShaderToggles[4][2]);
			ImGui::Spacing();
			ImGui::Checkbox("Use original BSDistantTreeShader::Technique", &BSShader::g_ShaderToggles[9][0]);
			ImGui::Checkbox("Use original BSDistantTreeShader::Material", &BSShader::g_ShaderToggles[9][1]);
			ImGui::Checkbox("Use original BSDistantTreeShader::Geometry", &BSShader::g_ShaderToggles[9][2]);
			ImGui::Spacing();
			ImGui::Checkbox("Use original BSSkyShader::Technique", &BSShader::g_ShaderToggles[2][0]);
			ImGui::Checkbox("Use original BSSkyShader::Material", &BSShader::g_ShaderToggles[2][1]);
			ImGui::Checkbox("Use original BSSkyShader::Geometry", &BSShader::g_ShaderToggles[2][2]);
		}

		ImGui::End();
	}
}

namespace ui::log
{
    ImGuiTextBuffer Buf;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets; // Index to lines offset
    bool ScrollToBottom;

    void Draw()
    {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Debug Log", &ui::showLogWindow);
        if (ImGui::Button("Clear"))
            Clear();
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);
        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (copy)
            ImGui::LogToClipboard();

        if (Filter.IsActive())
        {
            const char *buf_begin = Buf.begin();
            const char *line      = buf_begin;
            for (int line_no = 0; line != NULL; line_no++)
            {
                const char *line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
                if (Filter.PassFilter(line, line_end))
                    ImGui::TextUnformatted(line, line_end);
                line = line_end && line_end[1] ? line_end + 1 : NULL;
            }
        }
        else
        {
            ImGui::TextUnformatted(Buf.begin());
        }

        if (ScrollToBottom)
            ImGui::SetScrollHere(1.0f);
        ScrollToBottom = false;
        ImGui::EndChild();
        ImGui::End();
    }

    void Add(const char *Format, ...)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, Format);
        Buf.appendfv(Format, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size);
        ScrollToBottom = true;
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
    }
}
