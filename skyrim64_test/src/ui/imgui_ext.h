#pragma once

namespace ImGui
{
	bool BeginGroupSplitter(const char *Header);
	void EndGroupSplitter();
	void PlotMultiLines(const char* label, int num_datas, const char** names, const ImColor* colors, float(*getter)(const void* data, int idx), const void * const * datas, int values_count, float scale_min, float scale_max, ImVec2 graph_size);
	void PlotMultiHistograms(const char* label, int num_hists, const char** names, const ImColor* colors, float(*getter)(const void* data, int idx), const void * const * datas, int values_count, float scale_min, float scale_max, ImVec2 graph_size);
}