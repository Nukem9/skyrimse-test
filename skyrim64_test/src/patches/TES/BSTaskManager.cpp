#include <map>
#include <mutex>
#include "../../common.h"
#include "BSTaskManager.h"

SRWLOCK BSTask::TaskListLock = SRWLOCK_INIT;
std::map<BSTask *, std::string> BSTask::TaskMap;
std::vector<std::string> BSTask::TasksCurrentFrame;

void BSTask::AddRef()
{
	InterlockedIncrement((volatile long *)&iRefCount);
}

void BSTask::DecRef()
{
	if (InterlockedDecrement((volatile long *)&iRefCount) == 0)
		this->~BSTask();
}

bool BSTask::GetName_AddCellGrassTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "AddCellGrassTask");
	return false;
}

bool BSTask::GetName_AttachDistant3DTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "AttachDistant3DTask");
	return false;
}

bool BSTask::GetName_AudioLoadForPlaybackTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "AudioLoadForPlaybackTask");
	return false;
}

bool BSTask::GetName_AudioLoadToCacheTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "AudioLoadToCacheTask");
	return false;
}

bool BSTask::GetName_BGSParticleObjectCloneTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "BGSParticleObjectCloneTask");
	return false;
}

bool BSTask::GetName_BSScaleformMovieLoadTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "BSScaleformMovieLoadTask");
	return false;
}

bool BSTask::GetName_CheckWithinMultiBoundTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "CheckWithinMultiBoundTask");
	return false;
}

bool BSTask::GetName_QueuedFile(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "QueuedFile");
	return false;
}

bool BSTask::GetName_QueuedPromoteLocationReferencesTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "QueuedPromoteLocationReferencesTask");
	return false;
}

bool BSTask::GetName_QueuedPromoteReferencesTask(char *Buffer, size_t BufferSize)
{
	strcpy_s(Buffer, BufferSize, "QueuedPromoteReferencesTask");
	return false;
}

bool IOManager::QueueTask(BSTask *Task)
{
	AcquireSRWLockExclusive(&BSTask::TaskListLock);

	// Loop through the current list and check if any were finished or canceled
	for (auto itr = BSTask::TaskMap.begin(); itr != BSTask::TaskMap.end();)
	{
		// Release our reference (canceled or finished task)
		if (itr->first->eState == 5 || itr->first->eState == 6)
		{
			itr->first->DecRef();
			itr = BSTask::TaskMap.erase(itr);
		}
		else
		{
			itr++;
		}
	}

	// Is this a new task entry?
	if (BSTask::TaskMap.count(Task) <= 0)
	{
		std::string s(128, '\0');

		Task->AddRef();
		Task->GetName(s.data(), s.size());

		BSTask::TasksCurrentFrame.push_back(s);
		BSTask::TaskMap.emplace(Task, std::move(s));
	}
	
	// Sanity check if the UI counterpart is not running
	if (BSTask::TasksCurrentFrame.size() >= 500)
		BSTask::TasksCurrentFrame.clear();

	ReleaseSRWLockExclusive(&BSTask::TaskListLock);

	return ((bool(*)(IOManager *, BSTask *))(g_ModuleBase + 0xD2C550))(this, Task);
}