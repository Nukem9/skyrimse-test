#pragma once

namespace ui
{
	namespace opt
	{
		extern bool EnableCache;
		extern bool LogHitches;
		extern bool RealtimeOcclusionView;
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

	void Initialize(HWND Wnd, ID3D11Device *Device, ID3D11DeviceContext *DeviceContext);
	void HandleInput(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	void Render();

	void RenderMenubar();
	void RenderSynchronization();
	void RenderMemory();
	void RenderTESFormCache();
	void RenderShaderTweaks();
	void RenderINITweaks();

	namespace log
	{
		void Draw();
		void Add(const char *Format, ...);
		void Clear();
	}
}