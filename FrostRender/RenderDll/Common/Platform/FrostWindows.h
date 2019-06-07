#pragma once

#include"FrostPlatformDefines.h"

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

// Do not define min/max in windows.h
#define NOMINMAX

// Prevents <Windows.h> from #including <Winsock.h>
// Manually define your <Winsock2.h> inclusion point elsewhere instead.
#ifndef _WINSOCKAPI_
	#define _WINSOCKAPI_
#endif

#include <windows.h>

#include<assert.h>
#define FROST_ASSERT_TRACE(condition,parenthese_message) assert(condition)
#define FROST_ASSERT_MESSAGE(condition,...)              assert(condition)
#define FROST_ASSERT(condition)                          assert(condition)

