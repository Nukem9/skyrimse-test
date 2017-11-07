#include "../../imgui/imgui.h"
#include "imgui_ext.h"

namespace ImGui
{
	bool BeginGroupSplitter(const char *Header)
	{
		ImGui::Spacing();
		ImGui::Text(Header);
		ImGui::Separator();
		ImGui::Indent();

		return true;
	}

	void EndGroupSplitter()
	{
		ImGui::Unindent();
		ImGui::Spacing();
	}
}