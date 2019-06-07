#pragma once

#if defined(_x86_64_) || defined(_M_X64)
	#define FROST_PLATFORM_X64   1
	#define FROST_PLATFORM_64BIT 1
    #define FROST_PLATFORM_SSE2  1
	typedef unsigned long ThreadID;
#elif defined(_i386) || defined(_M_IX86)
	#define FROST_PLATFORM_X86   1
    #define FROST_PLATFORM_32BIT 1
	typedef unsigned int ThreadID;
#endif 

#if defined(_WIN32)
	#define FROST_PLATFORM_DESKTOP 1
	#define FROST_PLATFORM_WINAPI  1
	#if defined(_WIN64)
		#if !FROST_PLATFORM_X64
			#error Unsupported Windows 64 CPU (the only supported is x86-64).
		#endif 
	#else
		#if !FROST_PLATFORM_X86 
			#error Unsupported Windows 32 CPU (the only supported is x86).
		#endif 
	#endif 
#endif 



