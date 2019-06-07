#include"pch.h"
#include"System.h"

SystemGlobalEnvironment* g_Env;

CSystem::CSystem()
{
	::memset(&m_Env, 0, sizeof(m_Env));
	m_Env.pSystem = this;
	g_Env = &m_Env;
}


CSystem::~CSystem()
{
	m_Env.pSystem = nullptr;
}

ISystem* CreateSystem()
{
	return new CSystem();
}