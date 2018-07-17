#include <map>
#include "../common.h"
#include "../patches/dinput8.h"
#include "imgui_ext.h"
#include "imgui_impl_dx11.h"
#include "ui_renderer.h"
#include "../patches/TES/BSShader/BSShader.h"
#include "../patches/TES/Setting.h"
#include "../patches/rendering/GpuTimer.h"

extern SRWLOCK TaskListLock;
extern std::map<class BSTask *, std::string> TaskMap;
extern std::vector<std::string> TasksCurrentFrame;

namespace ui::opt
{
	bool EnableCache = true;
	bool LogHitches = true;
	bool LogQuestSceneActions = false;
	bool EnableNavmeshLog = false;
	bool RealtimeOcclusionView = false;
	bool EnableOcclusionTesting = true;
	bool EnableOccluderRendering = true;
	float OccluderMaxDistance = 15000.0f;
	float OccluderFirstLevelMinSize = 550.0f;
}

namespace ui
{
	bool showDemoWindow;
    bool showTESFormWindow;
    bool showLockWindow;
    bool showMemoryWindow;
	bool showShaderTweakWindow;
	bool showIniListWindow;
	bool showLogWindow;

	bool showFrameStatsWindow;
	bool showRTViewerWindow;
	bool showCullingWindow;
	bool showScenegraphWorldWindow;
	bool showSceneGraphMenuWindow;
	bool showSceneGraphMenu3DWindow;
	bool showSceneGraphShadowNodesWindow;
	bool showTaskListWindow;

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

	bool inFrame = false;

	void BeginFrame()
	{
		inFrame = true;
		ImGui_ImplDX11_NewFrame();

		// Draw a fullscreen overlay that renders over the game but not other menus
		ImGui::SetNextWindowPos(ImVec2(-1000.0f, -1000.0f));
		ImGui::SetNextWindowSize(ImVec2(1.0f, 1.0f));
		ImGui::Begin("##InvisiblePreOverlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
		ImGui::GetWindowDrawList()->PushClipRectFullScreen();
	}

    void EndFrame()
    {
		if (!inFrame)
			return;

		// ##InvisiblePreOverlay
		ImGui::GetWindowDrawList()->PopClipRect();
		ImGui::End();

        {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            RenderMenubar();
			ImGui::PopStyleVar();

			RenderFrameStatistics();
			RenderRenderTargetMenu();
			RenderOcclusionCullingMenu();
			RenderSceneGraphWindows();

            RenderSynchronization();
            RenderTESFormCache();
            RenderMemory();
			RenderShaderTweaks();
			RenderINITweaks();
			RenderTaskList();

			if (showDemoWindow)
				ImGui::ShowDemoWindow(&showDemoWindow);

            if (showLogWindow)
                log::Draw();
        }

		// Finally present everything to the screen
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

		if (ImGui::BeginMenu("SceneGraph"))
		{
			ImGui::MenuItem("World Tree", nullptr, &showScenegraphWorldWindow);
			ImGui::MenuItem("Menu Tree", nullptr, &showSceneGraphMenuWindow);
			ImGui::MenuItem("Menu3D Tree", nullptr, &showSceneGraphMenu3DWindow);
			ImGui::MenuItem("ShadowSceneNode Trees", nullptr, &showSceneGraphShadowNodesWindow);
			ImGui::EndMenu();
		}

        if (ImGui::BeginMenu("Renderer"))
        {
			ImGui::MenuItem("Frame Statistics", nullptr, &showFrameStatsWindow);
			ImGui::MenuItem("Render Target Viewer", nullptr, &showRTViewerWindow);
			ImGui::MenuItem("Occlusion Culling Viewer", nullptr, &showCullingWindow);
			ImGui::MenuItem("Shader Tweaks", nullptr, &showShaderTweakWindow);
			ImGui::EndMenu();
        }

		if (ImGui::BeginMenu("Navmesh"))
		{
			ImGui::MenuItem("Log To Console", nullptr, &opt::EnableNavmeshLog);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Statistics"))
		{
			ImGui::MenuItem("Synchronization", nullptr, &showLockWindow);
			ImGui::MenuItem("Memory", nullptr, &showMemoryWindow);
			ImGui::MenuItem("TESForm Cache", nullptr, &showTESFormWindow);
			ImGui::EndMenu();
		}

        if (ImGui::BeginMenu("Game"))
        {
			ImGui::MenuItem("Task List", nullptr, &showTaskListWindow);
			ImGui::MenuItem("Debug Log", nullptr, &showLogWindow);
			ImGui::MenuItem("INISetting Viewer", nullptr, &showIniListWindow);
			ImGui::Separator();
            bool blockInput = !ProxyIDirectInputDevice8A::GlobalInputAllowed();
            if (ImGui::MenuItem("Block Input", nullptr, &blockInput))
                ProxyIDirectInputDevice8A::ToggleGlobalInput(!blockInput);
			ImGui::MenuItem("Log Quest/Scene Actions", nullptr, &opt::LogQuestSceneActions);
			ImGui::MenuItem("Log Frame Hitches", nullptr, &opt::LogHitches);
            ImGui::EndMenu();
        }

		if (ImGui::BeginMenu("Miscellaneous"))
		{
			ImGui::MenuItem("ImGui Debug", nullptr, &showDemoWindow);
			ImGui::Separator();
			if (ImGui::MenuItem("Dump NiRTTI"))
			{
				FILE *f = fopen("C:\\nirtti.txt", "w");
				log::Add("Dumping NiRTTI script to %s...\n", "C:\\nirtti.txt");
				NiRTTI::DumpRTTIListing(f, true);
				fclose(f);
			}

			if (ImGui::MenuItem("Dump INISetting"))
			{
				FILE *f = fopen("C:\\inisetting.txt", "w");
				log::Add("Dumping INISetting script to %s...\n", "C:\\inisetting.txt");
				INISettingCollectionSingleton->DumpSettingIDAScript(f);
				INIPrefSettingCollectionSingleton->DumpSettingIDAScript(f);
				fclose(f);
			}
			ImGui::Separator();
			ImGui::MenuItem("##somespacing");
			ImGui::Separator();
			if (ImGui::MenuItem("Terminate Process"))
				TerminateProcess(GetCurrentProcess(), 0x13371337);
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

			// Convert from the game's containers to a standard c++ vector
			std::vector<Setting *> settingList;
			settingList.reserve(1000);

			for (auto *s = INISettingCollectionSingleton->SettingsA.QNext(); s; s = s->QNext())
					settingList.push_back(s->QItem());

			for (auto *s = INIPrefSettingCollectionSingleton->SettingsA.QNext(); s; s = s->QNext())
					settingList.push_back(s->QItem());

			ImGui::PushItemWidth(-1);

			// Draw the list itself
			ImGui::ListBoxVector<decltype(settingList)>("##rtbox", "Filter", &iniFilter, &settingList, &selectedIndex, [](const decltype(settingList) *Vec, size_t Index)
			{
				return Vec->at(Index)->pKey;
			}, 16);

			// Now the editor inputs
			if (ImGui::BeginGroupSplitter("Selection") && selectedIndex != -1)
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

    void RenderSynchronization()
    {
        if (!showLockWindow)
            return;

        if (ImGui::Begin("Synchronization", &showLockWindow))
        {
            if (ImGui::BeginGroupSplitter("Per Frame"))
            {
                ImGui::Text("Time acquiring read locks: %.2fms", ProfileGetDeltaTime("Read Lock Time"));
                ImGui::Text("Time acquiring write locks: %.2fms", ProfileGetDeltaTime("Write Lock Time"));
                ImGui::EndGroupSplitter();
            }

            if (ImGui::BeginGroupSplitter("Global"))
            {
                ImGui::Text("Time acquiring read locks: %.2fms", ProfileGetTime("Read Lock Time"));
                ImGui::Text("Time acquiring write locks: %.2fms", ProfileGetTime("Write Lock Time"));
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
			if (ImGui::Button("Dump jemalloc stats"))
			{
				// Explicitly omit arena information
				je_malloc_stats_print([](void *, const char *text)
				{
					ui::log::Add(text);
				}, nullptr, "a");
			}

            if (ImGui::BeginGroupSplitter("Per Frame"))
            {
                ImGui::Text("Allocs: %lld", ProfileGetDeltaValue("Alloc Count"));
                ImGui::Text("Frees: %lld", ProfileGetDeltaValue("Free Count"));
                ImGui::Text("Bytes: %.3f MB", (double)ProfileGetDeltaValue("Byte Count") / 1024 / 1024);
                ImGui::Spacing();
                ImGui::Text("Time spent allocating: %.2fms", ProfileGetDeltaTime("Time Spent Allocating"));
                ImGui::Text("Time spent freeing: %.2fms", ProfileGetDeltaTime("Time Spent Freeing"));
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
                ImGui::Text("Time spent allocating: %.2fms", ProfileGetTime("Time Spent Allocating"));
                ImGui::Text("Time spent freeing: %.2fms", ProfileGetTime("Time Spent Freeing"));
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

                ImGui::Text("Lookups: %s", ImGui::CommaFormat(cacheLookups));
                ImGui::Text("Hits: %s", ImGui::CommaFormat(cacheLookups - cacheMisses));
                ImGui::Text("Misses: %s", ImGui::CommaFormat(cacheMisses));
                ImGui::Spacing();
                ImGui::Text("Update time: %.2fms", ProfileGetDeltaTime("Cache Update Time"));
                ImGui::Text("Fetch time: %.2fms", ProfileGetDeltaTime("Cache Fetch Time"));
                ImGui::EndGroupSplitter();
            }

            if (ImGui::BeginGroupSplitter("Global"))
            {
                int64_t cacheLookups = ProfileGetValue("Cache Lookups");
                int64_t cacheMisses  = ProfileGetValue("Cache Misses");

                ImGui::Text("Lookups: %s", ImGui::CommaFormat(cacheLookups));
                ImGui::Text("Hits: %s", ImGui::CommaFormat(cacheLookups - cacheMisses));
                ImGui::Text("Misses: %s", ImGui::CommaFormat(cacheMisses));
                ImGui::Spacing();
                ImGui::Text("Update time: %.2fms", ProfileGetTime("Cache Update Time"));
				ImGui::Text("Fetch time: %.2fms", ProfileGetTime("Cache Fetch Time"));
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

	void RenderTaskList()
	{
		if (!showTaskListWindow)
			return;

		if (ImGui::Begin("Task List", &showTaskListWindow))
		{
			static std::map<std::string, uint64_t> taskHistory;

			AcquireSRWLockExclusive(&TaskListLock);
			{
				// Build global task history
				for (std::string& entry : TasksCurrentFrame)
				{
					const char *c = entry.c_str();

					if (strstr(c, "Queued"))
					{
						c += 6;

						if (strstr(c, " animation"))
							taskHistory["Queued animation"]++;
						else if (strstr(c, " head"))
							taskHistory["Queued head"]++;
						else if (strstr(c, " ref"))
							taskHistory["Queued ref"]++;
						else if (strstr(c, "PromoteLargeReferencesTask"))
							taskHistory["QueuedPromoteLargeReferencesTask"]++;
						else
							taskHistory[entry]++;
					}
					else if (strstr(c, "CellLoaderTask"))
						taskHistory["CellLoaderTask"]++;
					else
						taskHistory[entry]++;
				}

				TasksCurrentFrame.clear();

				// Show currently running tasks
				char header[32];
				sprintf_s(header, "Active Tasks (%lld)", TaskMap.size());

				if (ImGui::BeginGroupSplitter(header))
				{
					ImGui::BeginChild("taskscrolling", ImVec2(0, 300), false, ImGuiWindowFlags_HorizontalScrollbar);

					for (auto& entry : TaskMap)
						ImGui::TextUnformatted(entry.second.c_str());

					ImGui::EndChild();
					ImGui::EndGroupSplitter();
				}
			}
			ReleaseSRWLockExclusive(&TaskListLock);

			// Show history
			if (ImGui::BeginGroupSplitter("Task Counters"))
			{
				for (auto& entry : taskHistory)
					ImGui::Text("%s (%lld)", entry.first.c_str(), entry.second);

				ImGui::EndGroupSplitter();
			}
		}

		ImGui::End();
	}
}

namespace ui::log
{
	std::recursive_mutex Mutex;
	std::vector<char> Buf;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets; // Index to lines offset
    bool ScrollToBottom = true;

	void Draw()
	{
		Mutex.lock();
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
				const char *buf_begin = Buf.data();
				const char *line = buf_begin;
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
				ImGui::TextUnformatted(Buf.data(), Buf.data() + Buf.size() - 1);
			}

			if (ScrollToBottom)
				ImGui::SetScrollHere(1.0f);

			ScrollToBottom = false;
			ImGui::EndChild();
			ImGui::End();
		}
		Mutex.unlock();
    }

	void Add(const char *Format, va_list Args)
	{
		char tempBuffer[4096];
		size_t len = _vsnprintf_s(tempBuffer, _TRUNCATE, Format, Args);

		Mutex.lock();
		{
			size_t oldSize = Buf.size();

			// Clear if larger than 1MB
			if (oldSize > 1 * 1024 * 1024)
			{
				Buf.clear();
				LineOffsets.clear();
				oldSize = 0;
			}

			// Also guarantee a null terminator to fool any string functions
			if (oldSize > 0) Buf.pop_back();
			Buf.insert(Buf.end(), tempBuffer, &tempBuffer[len]);
			Buf.push_back('\0');

			for (size_t newSize = Buf.size(); oldSize < newSize; oldSize++)
			{
				if (Buf[oldSize] == '\n')
					LineOffsets.push_back((int)oldSize);
			}

			ScrollToBottom = true;
		}
		Mutex.unlock();
	}

    void Add(const char *Format, ...)
    {
		va_list args;
		va_start(args, Format);
		Add(Format, args);
		va_end(args);
    }

    void Clear()
    {
		Mutex.lock();
		{
			Buf.clear();
			LineOffsets.clear();
		}
		Mutex.unlock();
	}
}
