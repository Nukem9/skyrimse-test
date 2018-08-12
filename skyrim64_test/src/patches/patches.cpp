#include "../../xbyak/xbyak.h"
#include "../common.h"
#include "dinput8.h"
#include "TES/TESForm.h"
#include "TES/BSReadWriteLock.h"
#include "TES/BGSDistantTreeBlock.h"
#include "TES/BSGraphicsRenderer.h"
#include "TES/BSCullingProcess.h"
#include "TES/BSJobs.h"
#include "TES/BSShader/BSShaderManager.h"
#include "TES/BSShader/Shaders/BSBloodSplatterShader.h"
#include "TES/BSShader/Shaders/BSDistantTreeShader.h"
#include "TES/BSShader/Shaders/BSSkyShader.h"
#include "TES/BSShader/Shaders/BSGrassShader.h"
#include "TES/BSShader/Shaders/BSParticleShader.h"

void PatchAchievements();
void PatchD3D11();
void PatchLogging();
void PatchSettings();
void PatchSteam();
void PatchThreading();
void PatchWindow();
void PatchFileIO();
void ExperimentalPatchEmptyFunctions();
void ExperimentalPatchMemInit();

void PatchBSGraphicsRenderTargetManager();
void PatchBSThread();
void PatchMemory();
void PatchTESForm();
void TestHook5();

bool doCullTest = false;

extern const char BSTask_AddCellGrassTask[] = "AddCellGrassTask";
extern const char BSTask_AttachDistant3DTask[] = "AttachDistant3DTask";
extern const char BSTask_AudioLoadForPlaybackTask[] = "AudioLoadForPlaybackTask";
extern const char BSTask_AudioLoadToCacheTask[] = "AudioLoadToCacheTask";
extern const char BSTask_BGSParticleObjectCloneTask[] = "BGSParticleObjectCloneTask";
extern const char BSTask_BSScaleformMovieLoadTask[] = "BSScaleformMovieLoadTask";
extern const char BSTask_CheckWithinMultiBoundTask[] = "CheckWithinMultiBoundTask";
extern const char BSTask_QueuedFile[] = "QueuedFile";
extern const char BSTask_QueuedPromoteLocationReferencesTask[] = "QueuedPromoteLocationReferencesTask";
extern const char BSTask_QueuedPromoteReferencesTask[] = "QueuedPromoteReferencesTask";

class BSTask
{
	const static uint32_t MAX_REF_COUNT = 100000;


public:
	volatile int iRefCount;
	int eState;

	virtual ~BSTask();
	virtual void VFunc0() = 0;
	virtual void VFunc1() = 0;
	virtual void VFunc2();
	virtual bool GetName(char *Buffer, uint32_t BufferSize);

	void AddRef()
	{
		InterlockedIncrement((volatile long *)&iRefCount);
	}

	void DecRef()
	{
		if (InterlockedDecrement((volatile long *)&iRefCount) == 0)
			this->~BSTask();
	}

	template<const char *Name>
	bool GetName_Override(char *Buffer, size_t BufferSize)
	{
		strncpy_s(Buffer, BufferSize + 1, Name, BufferSize);
		return false;
	}
};

SRWLOCK TaskListLock = SRWLOCK_INIT;
std::map<BSTask *, std::string> TaskMap;
std::vector<std::string> TasksCurrentFrame;

bool IOManagerQueueTask(__int64 a1, BSTask *Task)
{
	AcquireSRWLockExclusive(&TaskListLock);

	// Loop through the current list and check if any were finished or canceled
	for (auto itr = TaskMap.begin(); itr != TaskMap.end();)
	{
		// Release our reference
		if (itr->first->eState == 5 || itr->first->eState == 6)
		{
			itr->first->DecRef();
			itr = TaskMap.erase(itr);
		}
		else
		{
			itr++;
		}
	}

	// Is this a new task entry?
	if (TaskMap.count(Task) <= 0)
	{
		std::string s(128, '\0');

		Task->AddRef();
		Task->GetName(s.data(), s.size());

		TasksCurrentFrame.push_back(s);
		TaskMap.emplace(Task, std::move(s));
	}

	ReleaseSRWLockExclusive(&TaskListLock);

	return ((bool(*)(__int64, BSTask *))(g_ModuleBase + 0xD2C550))(a1, Task);
}

char __fastcall test1(__int64 a1, __int64(__fastcall ***a2)(__int64, __int64, __int64), unsigned int a3, unsigned int a4)
{
	char result; // al

	if (a3)
		result = (**a2)((__int64)a2, a3, a4);
	else
		result = 1;

	MOC::ForceFlush();
	doCullTest = true;
	return result;
}

void test2()
{
	doCullTest = false;
}

void Patch_TESV()
{
	PatchThreading();
	PatchWindow();
	PatchD3D11();
	PatchSteam();
	PatchAchievements();
	PatchSettings();
	PatchMemory();
	//PatchFileIO();
	PatchTESForm();
	PatchBSThread();
	PatchBSGraphicsRenderTargetManager();
	PatchLogging();

	//
	// BGSDistantTreeBlock
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x4A8360), &BGSDistantTreeBlock::UpdateLODAlphaFade);

	//
	// BSCullingProcess
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xD50310), &BSCullingProcess::hk_Process);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12F93B1), &test1, Detours::X64Option::USE_REL32_JUMP);
	PatchMemory(g_ModuleBase + 0x12F93B1, (PBYTE)"\xE8", 1);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12F96C9), &test2, Detours::X64Option::USE_REL32_JUMP);
	PatchMemory(g_ModuleBase + 0x12F96C9, (PBYTE)"\xE8", 1);

	//
	// BSGraphicsRenderer
	//
	Detours::X64::DetourFunctionClass<void (BSGraphics::Renderer::*)(BSGraphics::TriShape *)>((PBYTE)(g_ModuleBase + 0x133EC20), &BSGraphics::Renderer::IncRef);
	Detours::X64::DetourFunctionClass<void (BSGraphics::Renderer::*)(BSGraphics::TriShape *)>((PBYTE)(g_ModuleBase + 0xD6B9B0), &BSGraphics::Renderer::DecRef);
	Detours::X64::DetourFunctionClass<void (BSGraphics::Renderer::*)(BSGraphics::DynamicTriShape *)>((PBYTE)(g_ModuleBase + 0x133ED50), &BSGraphics::Renderer::IncRef);
	Detours::X64::DetourFunctionClass<void (BSGraphics::Renderer::*)(BSGraphics::DynamicTriShape *)>((PBYTE)(g_ModuleBase + 0xD6C7D0), &BSGraphics::Renderer::DecRef);

	//
	// BSJobs
	//
	class jobhook : public Xbyak::CodeGenerator
	{
	public:
		jobhook() : Xbyak::CodeGenerator()
		{
			mov(rcx, qword[rax + rdx * 8 + 8]);
			mov(rdx, qword[rax + rdx * 8]);
			mov(r8, (uintptr_t)&BSJobs::DispatchJobCallback);
			call(r8);

			jmp(ptr[rip]);
			dq(g_ModuleBase + 0xC32111);
		}
	} static jobhookInstance;

	Detours::X64::DetourFunction((PBYTE)(g_ModuleBase + 0xC32109), (PBYTE)jobhookInstance.getCode());

	//
	// DirectInput (mouse, keyboard)
	//
	Detours::IATHook((PBYTE)g_ModuleBase, "dinput8.dll", "DirectInput8Create", (PBYTE)hk_DirectInput8Create);

	//
	// Locking
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06DF0), &BSReadWriteLock::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06E10), &BSReadWriteLock::LockForRead);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC070D0), &BSReadWriteLock::UnlockRead);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06E90), &BSReadWriteLock::LockForWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC070E0), &BSReadWriteLock::UnlockWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07080), &BSReadWriteLock::TryLockForWrite);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07110), &BSReadWriteLock::IsWritingThread);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC06F90), &BSReadWriteLock::LockForReadAndWrite);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07130), &BSAutoReadAndWriteLock::Initialize);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC07180), &BSAutoReadAndWriteLock::Deinitialize);

	//
	// NiRTTI
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0xC66F80), &NiRTTI::__ctor__);

	//
	// Shaders
	//
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12ACB20), &BSShaderManager::SetRenderMode);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12AD340), &BSShaderManager::SetCurrentAccumulator);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12AD330), &BSShaderManager::GetCurrentAccumulator);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12E0DA0), &BSShaderAccumulator::hk_RegisterObjectDispatch);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12E18B0), &BSShaderAccumulator::hk_FinishAccumulatingDispatch);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12EF750), &BSBloodSplatterShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1318050), &BSDistantTreeShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x13113F0), &BSSkyShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x12E4C10), &BSGrassShader::__ctor__);
	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1336C80), &BSParticleShader::__ctor__);
	TestHook5();// BSLightingShader

	//
	// Temporary hack to fix array overflow in BSParticleShader::SetupGeometry
	//
	uint32_t test = 0x2000;
	PatchMemory(g_ModuleBase + 0x02493CC + 1, (PBYTE)&test, sizeof(uint32_t));
	PatchMemory(g_ModuleBase + 0x0249374 + 1, (PBYTE)&test, sizeof(uint32_t));

	test = 100;
	PatchMemory(g_ModuleBase + 0x02494A8 + 2, (PBYTE)&test, sizeof(uint32_t));

	//
	// Misc
	//
	//ExperimentalPatchEmptyFunctions();
	ExperimentalPatchMemInit();

	// Broken printf statement that triggers invalid_parameter_handler(), "%08" should really be "%08X"
	const char *newFormat = "World object count changed on object '%s' %08X from %i to %i";

	PatchMemory(g_ModuleBase + 0x1696030, (PBYTE)newFormat, strlen(newFormat) + 1);

	//
	// Bug fix for when TAA/FXAA/DOF are disabled and quicksave doesn't work without
	// opening a menu
	//
	PatchMemory(g_ModuleBase + 0x12AE2DD, (PBYTE)"\x90\x90\x90\x90\x90\x90", 6);	// Kill jnz
	PatchMemory(g_ModuleBase + 0x12AE2E8, (PBYTE)"\x80\xBD\x11\x02\x00\x00\x00", 7);// Rewrite to 'cmp byte ptr [rbp+211h], 0'
	PatchMemory(g_ModuleBase + 0x12AE2EF, (PBYTE)"\x90\x90\x90\x90", 4);			// Extra rewrite padding

	*(PBYTE *)&TESObjectCell::CreateRootMultiBoundNode = Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x264230), &TESObjectCell::hk_CreateRootMultiBoundNode);

	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x154EB40), &BSTask::GetName_Override<BSTask_AddCellGrassTask>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x154A448), &BSTask::GetName_Override<BSTask_AttachDistant3DTask>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x1773738), &BSTask::GetName_Override<BSTask_AudioLoadForPlaybackTask>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x17737A0), &BSTask::GetName_Override<BSTask_AudioLoadToCacheTask>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x1631E20), &BSTask::GetName_Override<BSTask_BGSParticleObjectCloneTask>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x17D9B30), &BSTask::GetName_Override<BSTask_BSScaleformMovieLoadTask>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x157EFD8), &BSTask::GetName_Override<BSTask_CheckWithinMultiBoundTask>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x179ED68), &BSTask::GetName_Override<BSTask_QueuedFile>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x15785D8), &BSTask::GetName_Override<BSTask_QueuedPromoteLocationReferencesTask>, 4);
	Detours::X64::DetourClassVTable((uint8_t *)(g_ModuleBase + 0x1559E90), &BSTask::GetName_Override<BSTask_QueuedPromoteReferencesTask>, 4);

	uintptr_t addr = (uintptr_t)&IOManagerQueueTask;
	//PatchMemory(g_ModuleBase + 0x179EB58, (PBYTE)&addr, sizeof(uintptr_t));
}

void Patch_TESVCreationKit()
{
	PatchThreading();
	PatchWindow();
	PatchSteam();
	PatchFileIO();
}