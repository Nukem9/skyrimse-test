#include "../stdafx.h"
#include "imgui_impl_dx11.h"

namespace ui::opt
{
    bool EnableCache;
}

namespace ui
{
    bool showFPSWindow;
    bool showTESFormWindow;
    bool showLockWindow;

    bool enableCache = true;

    bool show_another_window = true;

    __int64 lastFrameAlloc;
    __int64 lastFrameFree;
    __int64 lastFrameByte;

    LARGE_INTEGER frameEnd;
    int64_t lastFrame;

    __int64 tickSum;
    __int64 deltaList[32];
    int counter;

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
        QueryPerformanceCounter(&frameEnd);

        ImGui::GetIO().MouseDrawCursor = true;

        ImGui_ImplDX11_NewFrame();
        {
            log::Draw();

            LARGE_INTEGER ticksPerSecond;
            QueryPerformanceFrequency(&ticksPerSecond);

            if (lastFrame == 0)
                lastFrame = frameEnd.QuadPart;

            __int64 delta = frameEnd.QuadPart - lastFrame;
            lastFrame     = frameEnd.QuadPart;

            tickSum -= deltaList[counter];
            tickSum += delta;
            deltaList[counter++] = delta;

            if (counter >= 32)
                counter = 0;

            double frameTimeMs = 1000 * (delta / (double)ticksPerSecond.QuadPart);

            if (frameTimeMs >= 100.0)
                log::Add("FRAME HITCH WARNING (%g ms)\n", frameTimeMs);

            double averageFrametime = ((double)tickSum / 32.0) / (double)ticksPerSecond.QuadPart;
            double averageFps       = 1.0 / averageFrametime;

            //UpdateHavokTimer((int)ceil(averageFps));

            float test = 0.0f; // *(float *)(g_ModuleBase + 0x1DADCA0);

            char tempBuf1[256];

            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("                        ", false))
                    ImGui::EndMenu();

                if (ImGui::BeginMenu("Modules"))
                {
                    ImGui::MenuItem("X3DAudio HRTF", g_Dll3DAudio ? "Detected" : "Not Detected");
                    ImGui::MenuItem("ReShade", g_DllReshade ? "Detected" : "Not Detected");
                    ImGui::MenuItem("ENB", g_DllEnb ? "Detected" : "Not Detected");
                    ImGui::MenuItem("SKSE64", g_DllSKSE ? "Detected" : "Not Detected");
                    ImGui::MenuItem("VTune", g_DllVTune ? "Detected" : "Not Detected");
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Windows"))
                {
                    if (ImGui::MenuItem("Framerate"))
                        showFPSWindow = true;

                    if (ImGui::MenuItem("TESForm Cache"))
                        showTESFormWindow = true;

                    if (ImGui::MenuItem("Synchronization"))
                        showLockWindow = true;

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Game"))
                {
                    bool g_BlockInput = !ProxyIDirectInputDevice8A::GlobalInputAllowed();

                    if (ImGui::MenuItem("Block Input", nullptr, &g_BlockInput))
                        ProxyIDirectInputDevice8A::ToggleGlobalInput(!g_BlockInput);

                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }

            if (showFPSWindow && ImGui::Begin("Framerate", &showFPSWindow))
            {
                ImGui::Text("FPS: %.2f", averageFps);
                ImGui::Spacing();
                ImGui::Text("Havok fMaxTime: %.2f FPS", 1.0f / test);
                ImGui::Text("Havok fMaxTimeComplex: %.2f FPS", 1.0f / test);
            }
            if (showFPSWindow)
                ImGui::End();

            if (showTESFormWindow && ImGui::Begin("TESForm Cache", &showTESFormWindow))
            {
                ImGui::Checkbox("Enable Cache", &enableCache);

                {
                    ImGui::Text("Per Frame");
                    ImGui::Separator();
                    ImGui::Indent();

                    int64_t cacheLookups = ProfileGetDeltaValue("Cache Lookups");
                    int64_t cacheMisses  = ProfileGetDeltaValue("Cache Misses");
                    int64_t nullFetches  = ProfileGetDeltaValue("Null Fetches");

                    ImGui::Text("Lookups: %s", format_commas(cacheLookups, tempBuf1));
                    ImGui::Text("Hits: %s", format_commas(cacheLookups - cacheMisses, tempBuf1));
                    ImGui::Text("Misses: %s", format_commas(cacheMisses, tempBuf1));
                    ImGui::Spacing();
                    ImGui::Text("Bitmap nullptr fetches: %s (%.2f%%)", format_commas(nullFetches, tempBuf1), ((double)nullFetches / (double)cacheLookups) * 100);
                    ImGui::Spacing();
                    ImGui::Text("Update time: %.5f seconds", ProfileGetDeltaTime("Cache Update Time"));
                    ImGui::Text("Fetch time: %.5f seconds", ProfileGetDeltaTime("Cache Fetch Time"));
                    ImGui::Unindent();
                }

                {
                    ImGui::Spacing();
                    ImGui::Text("Global");
                    ImGui::Separator();
                    ImGui::Indent();

                    int64_t cacheLookups = ProfileGetValue("Cache Lookups");
                    int64_t cacheMisses  = ProfileGetValue("Cache Misses");
                    int64_t nullFetches  = ProfileGetValue("Null Fetches");

                    ImGui::Text("Lookups: %s", format_commas(cacheLookups, tempBuf1));
                    ImGui::Text("Hits: %s", format_commas(cacheLookups - cacheMisses, tempBuf1));
                    ImGui::Text("Misses: %s", format_commas(cacheMisses, tempBuf1));
                    ImGui::Spacing();
                    ImGui::Text("Bitmap nullptr fetches: %s (%.2f%%)", format_commas(nullFetches, tempBuf1), ((double)nullFetches / (double)cacheLookups) * 100);
                    ImGui::Spacing();
                    ImGui::Text("Update time: %.5f seconds", ProfileGetTime("Cache Update Time"));
                    ImGui::Text("Fetch time: %.5f seconds", ProfileGetTime("Cache Fetch Time"));
                    ImGui::Unindent();
                }
            }
            if (showTESFormWindow)
                ImGui::End();

            if (showLockWindow && ImGui::Begin("Synchronization", &showLockWindow))
            {
                ImGui::Text("Per Frame");
                ImGui::Separator();
                ImGui::Indent();
                ImGui::Text("Time acquiring read locks: %.5f seconds", ProfileGetDeltaTime("Read Lock Time"));
                ImGui::Text("Time acquiring write locks: %.5f seconds", ProfileGetDeltaTime("Write Lock Time"));
                ImGui::Unindent();

                ImGui::Spacing();
                ImGui::Text("Global");
                ImGui::Separator();
                ImGui::Indent();
                ImGui::Text("Time acquiring read locks: %.5f seconds", ProfileGetTime("Read Lock Time"));
                ImGui::Text("Time acquiring write locks: %.5f seconds", ProfileGetTime("Write Lock Time"));
                ImGui::Unindent();
            }
            if (showLockWindow)
                ImGui::End();
            /*
			ImGui::Begin("Memory", &show_another_window);
			ImGui::Text("Allocs: %lld", tempAllocCount);
			ImGui::Text("Frees: %lld", tempFreeCount);
			ImGui::Text("Bytes: %.3f MB", (double)tempByteCount / 1024 / 1024);
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("Frame alloc delta: %lld", tempAllocCount - lastFrameAlloc);
			ImGui::Text("Frame free delta: %lld", tempFreeCount - lastFrameFree);
			ImGui::Text("Frame byte delta: %.3f MB", (double)(tempByteCount - lastFrameByte) / 1024 / 1024);
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("Active allocations: %lld", tempAllocCount - tempFreeCount);
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("Time spent allocating memory: %.5f seconds", (double)timeSpentAllocating / ticksPerSecond.QuadPart);
			ImGui::Text("Time spent freeing memory: %.5f seconds", (double)timeSpentFreeing / ticksPerSecond.QuadPart);
			ImGui::End();
			*/
        }
        ImGui::Render();
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
