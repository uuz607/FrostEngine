#pragma once

#include<thread>

typedef unsigned long  ThreadID;

enum THREAD_JOIN_MODE 
{
	THREAD_TRY_JOIN,
	THREAD_JOIN
};

struct IThread
{
	virtual ~IThread() {};

	// Entry functions for code executed on thread
	virtual void ThreadEntry() = 0;
};

struct IThreadManager
{
	virtual ~IThreadManager() {};

	virtual bool		JoinThread(IThread* pThreadTask, THREAD_JOIN_MODE JoinStatus) = 0;

	//Spawn a new thread and apply thread config settings at thread beginning.
	virtual bool		SpawnThread(IThread* pThread, const char* sThreadName, ...) = 0;

	//Get Thread Name
	virtual const char*	GetThreadName(ThreadID ThreadId) = 0;

	//Get ThreadID
	virtual ThreadID    GetThreadId(const char* sThreadName, ...) = 0;

	//Execute function for each other thread but this one 
	typedef void(*ThreadModifFunction)(ThreadID ThreadId, void* pData);
	virtual void	    ForEachOtherThread(IThreadManager::ThreadModifFunction fpThreadModiFunction, void* pFuncData = 0) = 0;
};

extern __declspec(dllexport)IThreadManager* CreateThreadManager();