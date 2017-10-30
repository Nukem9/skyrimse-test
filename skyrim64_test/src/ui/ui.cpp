#include "../stdafx.h"
#include "imgui_ext.h"
#include "imgui_impl_dx11.h"

namespace ui::opt
{
    bool EnableCache = true;
	bool LogHitches = true;
}

namespace ui
{
    bool showFPSWindow;
    bool showTESFormWindow;
    bool showLockWindow;
    bool showMemoryWindow;
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
        ImGui_ImplDX11_Init(Wnd, Device, DeviceContext);

        ImGui::GetIO().MouseDrawCursor = true;

        // Dark theme
        ImGuiStyle &style = ImGui::GetStyle();

        style.WindowRounding                        = 6.f;
        style.ScrollbarRounding                     = 2.f;
        style.WindowTitleAlign.x                    = 0.45f;
        style.Colors[ImGuiCol_Text]                 = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.98f, 0.98f, 0.98f, 0.50f);
        style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        style.Colors[ImGuiCol_ChildWindowBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg]              = ImVec4(0.10f, 0.10f, 0.10f, 0.90f);
        style.Colors[ImGuiCol_Border]               = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        style.Colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_FrameBg]              = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.28f, 0.28f, 0.28f, 0.40f);
        style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.31f, 0.31f, 0.31f, 0.45f);
        style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.19f, 0.19f, 0.19f, 0.20f);
        style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.30f, 0.30f, 0.30f, 0.87f);
        style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.30f, 0.30f, 0.30f, 0.60f);
        style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.80f, 0.80f, 0.80f, 0.30f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.80f, 0.80f, 0.80f, 0.40f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.86f, 0.86f, 0.86f, 0.52f);
        style.Colors[ImGuiCol_ComboBg]              = ImVec4(0.21f, 0.21f, 0.21f, 0.99f);
        style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab]           = ImVec4(0.60f, 0.60f, 0.60f, 0.34f);
        style.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.84f, 0.84f, 0.84f, 0.34f);
        style.Colors[ImGuiCol_Button]               = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
        style.Colors[ImGuiCol_Header]               = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_Separator]            = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_SeparatorActive]      = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
        style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
        style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
        style.Colors[ImGuiCol_CloseButton]          = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
        style.Colors[ImGuiCol_CloseButtonHovered]   = ImVec4(0.90f, 0.90f, 0.90f, 0.60f);
        style.Colors[ImGuiCol_CloseButtonActive]    = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        style.Colors[ImGuiCol_PlotLines]            = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.27f, 0.36f, 0.59f, 0.61f);
        style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    }

    void HandleInput(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
    {
        ImGui_ImplDX11_WndProcHandler(Wnd, Msg, wParam, lParam);
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

            if (showLogWindow)
                log::Draw();
        }
        ImGui::Render();
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

        if (ImGui::BeginMenu("Windows"))
        {
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

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void RenderFramerate()
    {
        if (!showFPSWindow)
            return;

        float test = 0.0f; // *(float *)(g_ModuleBase + 0x1DADCA0);

        if (ImGui::Begin("Framerate", &showFPSWindow))
        {
            ImGui::Text("FPS: %.2f", g_AverageFps);
            ImGui::Spacing();
            ImGui::Text("Havok fMaxTime: %.2f FPS", 1.0f / test);
            ImGui::Text("Havok fMaxTimeComplex: %.2f FPS", 1.0f / test);
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
        ImGui::Begin("Debug Log");
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
        Buf.appendv(Format, args);
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
