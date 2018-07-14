#pragma once

namespace ImGui
{
	const char *CommaFormat(__int64 Input);
	bool BeginGroupSplitter(const char *Header);
	void EndGroupSplitter();
	void PlotMultiLines(const char* label, int num_datas, const char** names, const ImColor* colors, float(*getter)(const void* data, int idx), const void * const * datas, int values_count, float scale_min, float scale_max, ImVec2 graph_size);
	void PlotMultiHistograms(const char* label, int num_hists, const char** names, const ImColor* colors, float(*getter)(const void* data, int idx), const void * const * datas, int values_count, float scale_min, float scale_max, ImVec2 graph_size);

	template<typename T>
	bool ListBoxVector(const char *Label, const char *FilterLabel, ImGuiTextFilter *Filter, const T *List, int *CurrentItem, const char *(* Getter)(const T *List, size_t Index), int HeightInItems = -1)
	{
		Filter->Draw(FilterLabel, -100.0f);

		const T *listCopy = List;
		T tempList;

		if (Filter->IsActive())
		{
			for (size_t i = 0; i < List->size(); i++)
			{
				if (Filter->PassFilter(Getter(List, i)))
					tempList.push_back(List->at(i));
			}

			listCopy = &tempList;
		}

		if (!ListBoxHeader(Label, (int)listCopy->size(), HeightInItems))
			return false;

		bool valueChanged = false;
		ImGuiListClipper clipper((int)listCopy->size(), GetTextLineHeightWithSpacing());

		while (clipper.Step())
		{
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				const bool itemSelected = (i == *CurrentItem);

				PushID(i);
				if (Selectable(Getter(listCopy, i), itemSelected))
				{
					*CurrentItem = i;
					valueChanged = true;
				}

				if (itemSelected)
					SetItemDefaultFocus();
				PopID();
			}
		}

		if (*CurrentItem > listCopy->size())
			*CurrentItem = -1;

		ListBoxFooter();
		return valueChanged;
	}
}