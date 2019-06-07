
#include"pch.h"
#include"SystemThreading.h"

CThreadManager::~CThreadManager()
{

}

#ifdef FROST_PLATFORM_WINAPI
unsigned __stdcall CThreadManager::RunThread(void* ArgList)
{
	ThreadMetaData* pThreadData = reinterpret_cast<ThreadMetaData*>(ArgList);

	pThreadData->m_pThreadTask->ThreadEntry();

	//Signal imminent thread end
	pThreadData->m_ThreadExitMutex.Lock();
	pThreadData->m_bRunning = false;
	pThreadData->m_ThreadExitCondition.Notify();
	pThreadData->m_ThreadExitMutex.Unlock();

	return NULL;
}
#endif

bool CThreadManager::JoinThread(IThread* pThreadTask, THREAD_JOIN_MODE JoinMode)
{
	_smart_ptr<ThreadMetaData> pThreadImpl = nullptr;
	{
		FrostAutoLock<FrostMutex> SpawnedThreadsLock(m_SpawnedThreadsLock);

		auto iterator_impl = m_SpawnedThreads.find(pThreadTask);
		if (iterator_impl == m_SpawnedThreads.end())
		{
			/*
				Thread has already finished and unregistered itself.
				As it is complete we cannot wait for it.
				Hence return true.
			*/ 
			return true;
		}

		pThreadImpl = iterator_impl->second;
	}

	if (JoinMode == THREAD_TRY_JOIN && pThreadImpl->m_bRunning)
	{
		return false;
	}

	// Wait for completion of the target thread exit condition
	pThreadImpl->m_ThreadExitMutex.Lock();
	while (pThreadImpl->m_bRunning)
	{
		pThreadImpl->m_ThreadExitCondition.Wait(pThreadImpl->m_ThreadExitMutex);
	}
	pThreadImpl->m_ThreadExitMutex.Unlock();

	return true;
}

bool CThreadManager::SpawnThread(IThread* pThreadTask, const char* sThreadName, ...)
{
	va_list args;
	va_start(args, sThreadName);

	// Format thread name
	char szThreadName[THREAD_NAME_LENGTH_MAX];
	if (!vsnprintf(szThreadName, THREAD_NAME_LENGTH_MAX, sThreadName, args))
	{
		
	}

	// Spawn thread
	bool ret = SpawnThreadImpl(pThreadTask, szThreadName);

	if (!ret)
	{
		
	}

	va_end(args);
	return ret;
}

bool CThreadManager::SpawnThreadImpl(IThread* pThreadTask, const char* sThreadName)
{
	if (pThreadTask == nullptr)
	{
		
		return false;
	}

	// Init thread meta data
	ThreadMetaData*  pThreadMetaData  = new ThreadMetaData();
	pThreadMetaData->m_pThreadTask	  = pThreadTask;
	pThreadMetaData->m_pThreadManager = this;
	pThreadMetaData->m_ThreadName	  = sThreadName;

	// Add thread to map
	{
		FrostAutoLock<FrostMutex> SpawnedThreadSafeScope(m_SpawnedThreadsLock);
		auto iterator_impl = m_SpawnedThreads.find(pThreadTask);
		if (iterator_impl != m_SpawnedThreads.end())
		{
			// Thread with same name already spawned
			delete pThreadMetaData;
			return false;
		}

		// Insert thread data
		m_SpawnedThreads.insert(ThreadMapPair(pThreadTask, pThreadMetaData));
	}

	// Create thread description
	FrostThreadUtility::THREAD_CREATION_DESC desc = { sThreadName, RunThread, pThreadMetaData, 0 };

	// Spawn new thread
	pThreadMetaData->m_bRunning = FrostThreadUtility::FrostCreateThread(desc, &(pThreadMetaData->m_ThreadHandle)); 

	// Validate thread creation
	if (!pThreadMetaData->m_bRunning)
	{
		// Remove thread from map (also releases ThreadMetaData _smart_ptr)
		m_SpawnedThreads.erase(m_SpawnedThreads.find(pThreadTask));
		return false;
	}

	return true;
}

ThreadID CThreadManager::GetThreadId(const char* sThreadName, ...)
{
	va_list args;
	va_start(args, sThreadName);

	// Format thread name
	char strThreadName[THREAD_NAME_LENGTH_MAX];
	if (!vsnprintf(strThreadName, THREAD_NAME_LENGTH_MAX, sThreadName, args))
	{
		
	}

	// Get thread name
	ThreadID ret = GetThreadIdImpl(strThreadName);

	va_end(args);
	return ret;
}

ThreadID CThreadManager::GetThreadIdImpl(const char* sThreadName)
{
	// Loop over internally spawned threads
	{
		FrostAutoLock<FrostMutex> ThreadIDSafeScope(m_SpawnedThreadsLock);

		for (auto it = m_SpawnedThreads.begin(); it != m_SpawnedThreads.end(); ++it)
		{
			if (it->second->m_ThreadName == sThreadName)
			{
				return it->second->m_ThreadID;
			}
		}
	}

	return 0;
}

const char* CThreadManager::GetThreadName(ThreadID ThreadId)
{
	// Loop over internally spawned threads
	{
		FrostAutoLock<FrostMutex> ThreadNameSafeScope(m_SpawnedThreadsLock);

		for (auto it = m_SpawnedThreads.begin(); it != m_SpawnedThreads.end(); ++it)
		{
			if (it->second->m_ThreadID == ThreadId)
			{
				return it->second->m_ThreadName;
			}
		}
	}

	return "";
}

bool CThreadManager::UnregisterThread(IThread* pThreadTask)
{
	FrostAutoLock<FrostMutex> UnregisterThreadSafeScope(m_SpawnedThreadsLock);

	auto iterator_impl = m_SpawnedThreads.find(pThreadTask);
	if (iterator_impl == m_SpawnedThreads.end())
	{
		// Duplicate thread deletion
		return false;
	}

	m_SpawnedThreads.erase(iterator_impl);
	return true;
}

void CThreadManager::ForEachOtherThread(IThreadManager::ThreadModifFunction pThreadModiFunction, void* pFuncData )
{
	ThreadID currentThreadID = FrostThreadUtility::FrostGetCurrentThreadID();

	// Loop over internally spawned threads
	{
		FrostAutoLock<FrostMutex> ExecuteThreadFunctionSafeScope (m_SpawnedThreadsLock);

		for (auto it = m_SpawnedThreads.begin(); it != m_SpawnedThreads.end(); ++it)
		{
			if (it->second->m_ThreadID != currentThreadID)
			{
				pThreadModiFunction(it->second->m_ThreadID, pFuncData);
			}
		}
	}
}


IThreadManager* CreateThreadManager()
{
	return new CThreadManager();
}