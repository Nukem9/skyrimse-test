#include <map>
#include "../common.h"
#include "../patches/dinput8.h"
#include "imgui_ext.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "ui_renderer.h"
#include "ui_tracy.h"
#include "../patches/TES/BSJobs.h"
#include "../patches/TES/BSTaskManager.h"
#include "../patches/TES/BSShader/BSShader.h"
#include "../patches/TES/Setting.h"
#include "../patches/rendering/GpuTimer.h"
#include "../patches/TES/TESForm.h"

namespace ui::opt
{
	bool EnableCache = true;
	bool LogHitches = true;
	bool LogQuestSceneActions = false;
	bool LogNavmeshProcessing = false;
	bool RealtimeOcclusionView = false;
	bool EnableOcclusionTesting = true;
	bool EnableOccluderRendering = true;
	float OccluderMaxDistance = 15000.0f;
	float OccluderFirstLevelMinSize = 550.0f;
}

namespace ui
{
	bool Initialized = false;
	bool InFrame = false;
	extern float LastFpsCount;

	bool showTracyWindow;
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
	bool showJobListWindow;

    void Initialize(HWND Wnd, ID3D11Device *Device, ID3D11DeviceContext *DeviceContext)
    {
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		//io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.MouseDrawCursor = true;

		ImGui_ImplWin32_Init(Wnd);
        ImGui_ImplDX11_Init(Device, DeviceContext);

		ImGui::StyleColorsDark();
		ImGui::GetStyle().ChildRounding = 0.0f;
		ImGui::GetStyle().FrameRounding = 0.0f;
		ImGui::GetStyle().GrabRounding = 0.0f;
		ImGui::GetStyle().PopupRounding = 0.0f;
		ImGui::GetStyle().ScrollbarRounding = 0.0f;
		ImGui::GetStyle().WindowRounding = 0.0f;

		Initialized = true;
    }

    void HandleInput(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
    {
		ImGui_ImplWin32_WndProcHandler(Wnd, Msg, wParam, lParam);
    }

	bool IsMouseDragging()
	{
		if (!Initialized)
			return false;

		return ImGui::IsMouseDragging();
	}

	void BeginFrame()
	{
		ZoneScopedN("ui::BeginFrame");

		InFrame = true;
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Draw a fullscreen overlay that renders over the game but not other menus
		ImGui::SetNextWindowPos(ImVec2(-1000.0f, -1000.0f));
		ImGui::SetNextWindowSize(ImVec2(1.0f, 1.0f));
		ImGui::Begin("##InvisiblePreOverlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
		ImGui::GetWindowDrawList()->PushClipRectFullScreen();
	}

    void EndFrame()
    {
		ZoneScopedN("ui::EndFrame");

		if (!InFrame)
			return;

		// ##InvisiblePreOverlay
		ImGui::GetWindowDrawList()->PopClipRect();
		ImGui::End();

        {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            RenderMenubar();
			ImGui::PopStyleVar();

			RenderTracyWindow();
			RenderFrameStatistics();
			RenderRenderTargetMenu();
			RenderOcclusionCullingMenu();
			RenderSceneGraphWindows();

            RenderSynchronization();
            RenderTESFormCache();
            RenderMemory();
			RenderShaderTweaks();
			RenderINITweaks();
			RenderJobList();
			RenderTaskList();

			if (showDemoWindow)
				ImGui::ShowDemoWindow(&showDemoWindow);

            if (showLogWindow)
                log::Draw();
        }

		// Finally present everything to the screen
        ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
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

		if (ImGui::BeginMenu("Statistics"))
		{
			if (ImGui::MenuItem("Open Tracy", nullptr, nullptr, SKYRIM64_USE_TRACY ? true : false))
				showTracyWindow = true;
			if (ImGui::MenuItem("Close Tracy", nullptr, nullptr, SKYRIM64_USE_TRACY ? true : false))
				showTracyWindow = false;
			ImGui::Separator();
			ImGui::MenuItem("Job List", nullptr, &showJobListWindow);
			ImGui::MenuItem("Task List", nullptr, &showTaskListWindow);
			ImGui::Separator();
			ImGui::MenuItem("Synchronization", nullptr, &showLockWindow);
			ImGui::MenuItem("Memory", nullptr, &showMemoryWindow);
			ImGui::MenuItem("TESForm Cache", nullptr, &showTESFormWindow);
			ImGui::EndMenu();
		}

        if (ImGui::BeginMenu("Game"))
        {
			ImGui::MenuItem("Debug Log", nullptr, &showLogWindow);
			ImGui::MenuItem("INISetting Viewer", nullptr, &showIniListWindow);
			ImGui::Separator();
			ImGui::MenuItem("Log Navmesh Processing", nullptr, &opt::LogNavmeshProcessing);
			ImGui::MenuItem("Log Quest/Scene Actions", nullptr, &opt::LogQuestSceneActions);
			ImGui::MenuItem("Log Frame Hitches", nullptr, &opt::LogHitches);
			bool blockInput = !ProxyIDirectInputDevice8A::GlobalInputAllowed();
			if (ImGui::MenuItem("Block Game Input", nullptr, &blockInput))
				ProxyIDirectInputDevice8A::ToggleGlobalInput(!blockInput);
            ImGui::EndMenu();
        }

		if (ImGui::BeginMenu("Weather"))
		{
			auto weatherTypes = TESForm::LookupFormsByType(54);

			for (TESForm *form : weatherTypes)
			{
				char name[256];
				sprintf_s(name, "%s", form->GetName());
				//form->GetFullTypeName(name, 256);

				ImGui::MenuItem(name, nullptr, false);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Miscellaneous"))
		{
			if (ImGui::MenuItem("Use Dark Theme"))
				ImGui::StyleColorsDark();
			if (ImGui::MenuItem("Use Light Theme"))
				ImGui::StyleColorsLight();
			ImGui::MenuItem("ImGui Debug", nullptr, &showDemoWindow);
			ImGui::Separator();
			if (ImGui::MenuItem("Dump NiRTTI Script"))
			{
				if (FILE *f; fopen_s(&f, "C:\\nirtti.txt", "w") == 0)
				{
					log::Add("Dumping NiRTTI script to %s...\n", "C:\\nirtti.txt");
					NiRTTI::DumpRTTIListing(f, true);
					fclose(f);
				}
			}
			if (ImGui::MenuItem("Dump INISetting Script"))
			{
				if (FILE *f; fopen_s(&f, "C:\\inisetting.txt", "w") == 0)
				{
					log::Add("Dumping INISetting script to %s...\n", "C:\\inisetting.txt");
					INISettingCollectionSingleton->DumpSettingIDAScript(f);
					INIPrefSettingCollectionSingleton->DumpSettingIDAScript(f);
					fclose(f);
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Terminate Process"))
				TerminateProcess(GetCurrentProcess(), 0x13371337);
			ImGui::EndMenu();
		}

		// Display FPS and mouse X, Y in the top right corner
		char stats[256];
		float len1 = ImGui::CalcTextSize("1000.1 FPS ").x;
		float len2 = ImGui::CalcTextSize("(-1000, -1000)").x;

		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() - (len1 + len2));
		sprintf_s(stats, "%.1f FPS", LastFpsCount);

		if (ImGui::BeginMenu(stats, false))
			ImGui::EndMenu();

		POINT p;
		GetCursorPos(&p);

		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() - len2);
		sprintf_s(stats, "(%04d, %04d)", p.x, p.y);

		if (ImGui::BeginMenu(stats, false))
			ImGui::EndMenu();

        ImGui::EndMainMenuBar();
    }

	void RenderINITweaks()
	{
		if (!showIniListWindow)
			return;

		if (ImGui::Begin("INISetting Viewer", &showIniListWindow))
		{
			static ImGuiTextFilter iniFilter;
			static int indexHolder;

			// Convert from the game's containers to a standard c++ vector
			std::vector<Setting *> settingList;
			settingList.reserve(1848);

			for (auto *s = INISettingCollectionSingleton->SettingsA.QNext(); s; s = s->QNext())
					settingList.push_back(s->QItem());

			for (auto *s = INIPrefSettingCollectionSingleton->SettingsA.QNext(); s; s = s->QNext())
					settingList.push_back(s->QItem());

			ImGui::PushItemWidth(-1);

			// Draw the list itself
			int selection = ImGui::ListBoxVector<decltype(settingList)>("##rtbox", "Filter", &iniFilter, &settingList, &indexHolder, [](const decltype(settingList) *Vec, size_t Index)
			{
				return Vec->at(Index)->pKey;
			}, 16);

			// Now the editor inputs
			if (ImGui::BeginGroupSplitter("Selection") && selection != -1)
			{
				Setting *s = settingList.at(selection);

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

	void RenderJobList()
	{
		if (!showJobListWindow)
			return;

		if (ImGui::Begin("Job List", &showJobListWindow))
		{
			// Easy way to sort by name. Pair<total count, current count>.
			std::map<const std::string, std::pair<uint64_t, uint32_t>> sortedMap;
			int activeJobs = 0;

			for (auto& [k, v] : BSJobs::JobTracker)
			{
				uint32_t active = v.ActiveCount.load();

				if (active > 0)
					activeJobs++;

				sortedMap.insert_or_assign(v.Name, std::pair(v.TotalCount.load(), active));
				v.ActiveCount.store(0);
			}

			// Show currently running jobs
			char header[64];
			sprintf_s(header, "Active Jobs This Frame (%d)", activeJobs);

			if (ImGui::BeginGroupSplitter(header))
			{
				ImGui::BeginChild("jobscrolling1", ImVec2(0, 300), false, ImGuiWindowFlags_HorizontalScrollbar);

				for (const auto& [k, v] : sortedMap)
				{
					if (v.second > 0)
						ImGui::Text("%s (%d)", k.c_str(), v.second);
				}

				ImGui::EndChild();
				ImGui::EndGroupSplitter();
			}

			// Show history
			if (ImGui::BeginGroupSplitter("Job Counters"))
			{
				ImGui::BeginChild("jobscrolling2", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

				for (const auto& [k, v] : sortedMap)
				{
					if (v.first > 0)
						ImGui::Text("%s (%lld)", k.c_str(), v.first);
				}

				ImGui::EndChild();
				ImGui::EndGroupSplitter();
			}
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

			AcquireSRWLockExclusive(&BSTask::TaskListLock);
			{
				// Build global task history
				for (std::string& entry : BSTask::TasksCurrentFrame)
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

				BSTask::TasksCurrentFrame.clear();

				// Show currently running tasks
				char header[64];
				sprintf_s(header, "Active Tasks This Frame (%lld)", BSTask::TaskMap.size());

				if (ImGui::BeginGroupSplitter(header))
				{
					ImGui::BeginChild("taskscrolling1", ImVec2(0, 300), false, ImGuiWindowFlags_HorizontalScrollbar);

					for (auto& entry : BSTask::TaskMap)
						ImGui::TextUnformatted(entry.second.c_str());

					ImGui::EndChild();
					ImGui::EndGroupSplitter();
				}
			}
			ReleaseSRWLockExclusive(&BSTask::TaskListLock);

			// Show history
			if (ImGui::BeginGroupSplitter("Task Counters"))
			{
				ImGui::BeginChild("taskscrolling2", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

				for (auto& entry : taskHistory)
					ImGui::Text("%s (%lld)", entry.first.c_str(), entry.second);

				ImGui::EndChild();
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
