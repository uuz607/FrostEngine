#pragma once

#include"..\Common\Interface\IThread.h"

class CThreadManager;

struct ThreadMetaData : public CMultiThreadRefCount 
{
	ThreadMetaData()
		: m_pThreadTask(nullptr)
		, m_pThreadManager(nullptr)
		, m_ThreadHandle(nullptr)
		, m_ThreadID(0)
		, m_ThreadName("Frost_UnnamedThread")
		, m_bRunning(false)
	{

	}

	IThread*			   m_pThreadTask;
	CThreadManager*		   m_pThreadManager;

	HANDLE				   m_ThreadHandle;
	
	ThreadID			   m_ThreadID;// The active threadId, 0 = Invalid Id

	FrostMutex			   m_ThreadExitMutex;
	FrostConditionVariable m_ThreadExitCondition;

	const char*			   m_ThreadName;
	volatile bool          m_bRunning;  // Indicates the thread is not ready to exit yet
};

class CThreadManager : public IThreadManager
{
public:
	virtual ~CThreadManager();

	virtual bool		JoinThread(IThread* pThreadTask, THREAD_JOIN_MODE JoinMode) override;
	virtual bool		SpawnThread(IThread* pThread, const char* sThreadName, ...) override;
	virtual const char* GetThreadName(ThreadID ThreadId) override;
	virtual ThreadID    GetThreadId(const char* sThreadName, ...) override;
	virtual void        ForEachOtherThread(IThreadManager::ThreadModifFunction pThreadModiFunction, void* pFuncData = 0) override;

private:
	#ifdef FROST_PLATFORM_WINAPI
	static unsigned __stdcall RunThread(void* ArgList);
	#endif

	bool     UnregisterThread(IThread* pThreadTask);
	bool     SpawnThreadImpl(IThread* pThread, const char* sThreadName);
	ThreadID GetThreadIdImpl(const char* sThreadName);
	/* 
		Note: Guard SThreadMetaData with a _smart_ptr and lock to ensure that a thread waiting to be signaled by another still
		has access to valid SThreadMetaData even though the other thread terminated and as a result unregistered itself from the CThreadManager.
		An example would be the join method. Where one thread waits on a signal from an other thread to terminate and release its SThreadMetaData,
		sharing the same SThreadMetaData condition variable.
	*/

	typedef std::map< IThread*, _smart_ptr<ThreadMetaData>>	SpawnedThreadMap;
	typedef std::pair<IThread*, _smart_ptr<ThreadMetaData>> ThreadMapPair;

	FrostMutex		 m_SpawnedThreadsLock;	// Use lock for the rare occasion a thread is created/destroyed
	SpawnedThreadMap m_SpawnedThreads;
};
