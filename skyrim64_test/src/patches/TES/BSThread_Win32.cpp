#include "../../stdafx.h"

std::atomic<const char *> NextThreadName;
std::atomic<int> NextTaskletIndex;

bool (*sub_140C06440)(uintptr_t a1, int a2, const char *a3);
bool hk_sub_140C06440(uintptr_t a1, int a2, const char *a3)
{
    NextThreadName = _strdup(a3);
    bool result    = sub_140C06440(a1, a2, a3);

    while (NextThreadName)
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
        SetThreadName(tid, name);

    ui::log::Add("Created thread \"%s\" (ID %d)\n", name, tid);
    return ((LPTHREAD_START_ROUTINE)(g_ModuleBase + 0xC0BCE0))(lpArg);
}

LPTHREAD_START_ROUTINE TaskletEntryFunc;
DWORD WINAPI hk_TaskletEntryFunc(LPVOID lpArg)
{
    char name[256];
    sprintf_s(name, "TaskletThread%d", NextTaskletIndex++);

    SetThreadName(GetCurrentThreadId(), name);
	ui::log::Add("Created thread \"%s\" (ID %d)\n", name, GetCurrentThreadId());

    return TaskletEntryFunc(lpArg);
}

void PatchBSThread()
{
    *(uint8_t **)&sub_140C06440     = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xC06440), (PBYTE)&hk_sub_140C06440);
    *(uint8_t **)&BSThreadEntryFunc = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xC06630), (PBYTE)&hk_BSThreadEntryFunc);
    *(uint8_t **)&TaskletEntryFunc  = Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xC39A60), (PBYTE)&hk_TaskletEntryFunc);
}
