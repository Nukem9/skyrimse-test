//
// NOTE: "common.h" can't be included due to tracy server/client structure conflicts. Remove
// this once it's resolved.
//
#include <windows.h>
#include "../config.h"
#include "../../../imgui/imgui.h"
#if SKYRIM64_USE_TRACY
#include "../../../tracy_lib/tracy/server/TracyBadVersion.hpp"
#include "../../../tracy_lib/tracy/server/TracyFileRead.hpp"
#include "../../../tracy_lib/tracy/server/TracyView.hpp"
#endif

namespace ui
{
	extern bool showTracyWindow;

	void RenderTracyWindow()
	{
#if SKYRIM64_USE_TRACY
		static std::unique_ptr<tracy::View> View;

		// if (needs to be initialized)
		if (showTracyWindow && !View)
			View = std::make_unique<tracy::View>("127.0.0.1");

		// if (needs to be closed)
		if (!showTracyWindow && View)
			View.reset();

		if (!View)
			return;

		// Render the profiler ui
		if (!View->Draw())
			View.reset();
#endif
	}
}