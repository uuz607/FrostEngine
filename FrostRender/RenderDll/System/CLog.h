#pragma once

#include<stdio.h>
#include<stdarg.h>
#include"..\Common\Interface\ILog.h"

class CLog : public ILog
{
public:
	CLog();
	~CLog();

	virtual void Log(const char *format, va_list args) override
	{
		vfprintf(m_file, format, args);
	}
private:

	FILE* m_file;
};


