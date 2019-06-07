#pragma once

#include<ISystem.h>

class CSystem : public ISystem
{
public:
	CSystem();
	~CSystem();

	virtual IThreadManager* GetIThreadManager()  const override { return m_Env.pThreadManager; }
	virtual ILog*			GetILog() const override			{ return m_Env.pLog; }

	virtual void			SetIThreadManager(IThreadManager* pThreadManager) override { m_Env.pThreadManager = pThreadManager; }
	virtual void			SetILog(ILog* pLog) override							   { m_Env.pLog = pLog; }

private:
	SystemGlobalEnvironment m_Env;
};