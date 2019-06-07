#pragma once

#include<IThread.h>
#include<ILog.h>

struct ISystem
{
	virtual ~ISystem() {}

	virtual IThreadManager* GetIThreadManager() const = 0;
	virtual ILog*			GetILog() const = 0;

	virtual void			SetIThreadManager(IThreadManager* pThreadManager) = 0;
	virtual void			SetILog(ILog* pLog) = 0;
};

struct SystemGlobalEnvironment
{
	IThreadManager* pThreadManager;
	ILog*			pLog;
	ISystem*		pSystem;
};

extern SystemGlobalEnvironment* g_Env;

inline ISystem* GetISystem()
{
	return g_Env->pSystem;
}

inline ILog* GetILog()
{
	return g_Env->pLog;
}

inline void FrostLog(const char* format, ...)
{
	ILog* pLog = GetILog();
	if (pLog)
	{
		va_list args;
		va_start(args, format);
		pLog->Log(format, args);
		va_end(args);
	}
}

extern __declspec(dllexport) ISystem* CreateSystem();