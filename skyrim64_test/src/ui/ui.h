#pragma once

namespace ui
{
	namespace opt
	{
		extern bool EnableCache;
		extern bool LogHitches;
		extern bool EnableNavmeshLog;
		extern bool RealtimeOcclusionView;
		extern bool EnableOcclusionTesting;
		extern bool EnableOccluderRendering;
		extern float OccluderMaxDistance;
		extern float OccluderFirstLevelMinSize;
	}

	extern bool showDemoWindow;
	extern bool showTESFormWindow;
	extern bool showLockWindow;
	extern bool showMemoryWindow;
	extern bool showShaderTweakWindow;
	extern bool showIniListWindow;
	extern bool showLogWindow;

	extern bool showFrameStatsWindow;
	extern bool showRTViewerWindow;
	extern bool showCullingWindow;
	extern bool showScenegraphWorldWindow;
	extern bool showSceneGraphMenuWindow;
	extern bool showSceneGraphMenu3DWindow;
	extern bool showTaskListWindow;

	void Initialize(HWND Wnd, ID3D11Device *Device, ID3D11DeviceContext *DeviceContext);
	void HandleInput(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	void BeginFrame();
	void EndFrame();

	void RenderMenubar();
	void RenderSynchronization();
	void RenderMemory();
	void RenderTESFormCache();
	void RenderShaderTweaks();
	void RenderINITweaks();
	void RenderTaskList();

	namespace log
	{
		void Draw();
		void Add(const char *Format, ...);
		void Clear();
	}
}