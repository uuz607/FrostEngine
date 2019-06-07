
#include"CLog.h"

CLog::CLog() 
{ 
	::fopen_s(&m_file, "Log.txt", "w");
}

CLog::~CLog()
{
	::fclose(m_file);
}

ILog* CreateLog()
{
	return new CLog();
}