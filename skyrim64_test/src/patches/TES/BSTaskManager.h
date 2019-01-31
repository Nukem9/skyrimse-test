#pragma once

#include <map>

class BSTask
{
	friend class BSTaskManager;
	friend class IOManager;

private:
	const static uint32_t MAX_REF_COUNT = 100000;

	volatile int iRefCount;
	int eState;

public:
	static SRWLOCK TaskListLock;
	static std::map<BSTask *, std::string> TaskMap;
	static std::vector<std::string> TasksCurrentFrame;

	virtual ~BSTask();
	virtual void VFunc0() = 0;
	virtual void VFunc1() = 0;
	virtual void VFunc2();
	virtual bool GetName(char *Buffer, uint32_t BufferSize);

	void AddRef();
	void DecRef();

	bool GetName_AddCellGrassTask(char *Buffer, size_t BufferSize) const;
	bool GetName_AttachDistant3DTask(char *Buffer, size_t BufferSize) const;
	bool GetName_AudioLoadForPlaybackTask(char *Buffer, size_t BufferSize) const;
	bool GetName_AudioLoadToCacheTask(char *Buffer, size_t BufferSize) const;
	bool GetName_BGSParticleObjectCloneTask(char *Buffer, size_t BufferSize) const;
	bool GetName_BSScaleformMovieLoadTask(char *Buffer, size_t BufferSize) const;
	bool GetName_CheckWithinMultiBoundTask(char *Buffer, size_t BufferSize) const;
	bool GetName_QueuedFile(char *Buffer, size_t BufferSize) const;
	bool GetName_QueuedPromoteLocationReferencesTask(char *Buffer, size_t BufferSize) const;
	bool GetName_QueuedPromoteReferencesTask(char *Buffer, size_t BufferSize) const;
};
//static_assert_offset(BSTask, iRefCount, 0x8);
//static_assert_offset(BSTask, eState, 0xC);

class BSTaskManager
{
public:
};

class IOManager : public BSTaskManager
{
public:
	bool QueueTask(BSTask *Task);
};