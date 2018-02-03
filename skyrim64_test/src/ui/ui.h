#pragma once

namespace ui
{
	namespace opt
	{
		extern bool EnableCache;
		extern bool LogHitches;
	}

	void Initialize(HWND Wnd, ID3D11Device *Device, ID3D11DeviceContext *DeviceContext);
	void HandleInput(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	void Render();

	void RenderMenubar();
	void RenderFramerate();
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