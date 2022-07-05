#include <atomic>
#include "../../common.h"

std::atomic<const char *> NextThreadName;
std::atomic<int> NextTaskletIndex;

bool (*sub_140C06440)(uintptr_t a1, int a2, const char *a3);
bool hk_sub_140C06440(uintptr_t a1, int a2, const char *a3)
{
    NextThreadName = _strdup(a3);
    bool result    = sub_140C06440(a1, a2, a3);

    while (NextThreadName.load())
        Sleep(1);

    return result;
}

LPTHREAD_START_ROUTINE BSThreadEntryFunc;
DWORD WINAPI hk_BSThreadEntryFunc(LPVOID lpArg)
{
    auto name      = NextThreadName.load();
    auto tid       = GetCurrentThreadId();
    NextThreadName = nullptr;

    if (name)
        XUtil::SetThreadName(tid, name);

#if !SKYRIM64_CREATIONKIT_ONLY
    ui::log::Add("Created thread \"%s\" (ID %d)\n", name, tid);
#endif

	AutoFunc(LPTHREAD_START_ROUTINE, sub_140C0D1C0, 0xC0D1C0);
    return sub_140C0D1C0(lpArg);
}

LPTHREAD_START_ROUTINE TaskletEntryFunc;
DWORD WINAPI hk_TaskletEntryFunc(LPVOID lpArg)
{
    char name[256];
    sprintf_s(name, "TaskletThread%d", NextTaskletIndex++);

    XUtil::SetThreadName(GetCurrentThreadId(), name);
#if !SKYRIM64_CREATIONKIT_ONLY
	ui::log::Add("Created thread \"%s\" (ID %d)\n", name, GetCurrentThreadId());
#endif

    return TaskletEntryFunc(lpArg);
}

void PatchBSThread()
{
    *(uintptr_t *)&sub_140C06440     = Detours::X64::DetourFunction(g_ModuleBase + 0xC07920, (uintptr_t)&hk_sub_140C06440);
    *(uintptr_t *)&BSThreadEntryFunc = Detours::X64::DetourFunction(g_ModuleBase + 0xC07B10, (uintptr_t)&hk_BSThreadEntryFunc);
    *(uintptr_t *)&TaskletEntryFunc  = Detours::X64::DetourFunction(g_ModuleBase + 0xC3AF40, (uintptr_t)&hk_TaskletEntryFunc);
}
