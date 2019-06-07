#pragma once

#include<stdio.h>
#include<stdarg.h>

struct ILog
{
	virtual ~ILog() {}

	virtual void Log(const char *format, va_list args) = 0;
};

extern ILog* g_Log;

extern __declspec(dllexport)ILog*  CreateLog();

