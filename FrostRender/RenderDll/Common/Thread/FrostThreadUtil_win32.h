#pragma once

#define DEFAULT_THREAD_STACK_SIZE_KB 0

namespace FrostThreadUtility
{
	struct THREAD_CREATION_DESC
	{
		typedef unsigned int(_stdcall *EntryFunc)(void*);

		const char* szThreadName;
		EntryFunc   pEntryFunc;
		void*		pArgList;
		uint32_t	nStackSizeInBytes;
	};

	inline HANDLE FrostGetCurrentThreadHandle()
	{
		return ::GetCurrentThread();
	}

	inline void FrostCloseThreadHandle(HANDLE& hObject)
	{
		if (hObject)
		{
			::CloseHandle(hObject);
		}
	}

	inline ThreadID FrostGetCurrentThreadID()
	{
		return ::GetCurrentThreadId();
	}

	inline ThreadID FrostGetThreadID(HANDLE Thread)
	{
		return ::GetThreadId(Thread);
	}

	inline void FrostSetThreadName(HANDLE Thread, const char* szThreadName)
	{
		const DWORD MS_VC_EXCEPTION = 0x406D1388;

		struct THREAD_NAME_DESC
		{
			DWORD  dwType;      // Must be 0x1000.
			LPCSTR szName;      // Pointer to name (in user addr space).
			DWORD  dwThreadID;  // Thread ID (-1=caller thread).
			DWORD  dwFlags;     // Reserved for future use, must be zero.
		};

		THREAD_NAME_DESC info;
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = GetThreadId(Thread);
		info.dwFlags = 0;

	#pragma warning(push)
	#pragma warning(disable : 6312 6322)
		// warning C6312: Possible infinite loop: use of the constant EXCEPTION_CONTINUE_EXECUTION in the exception-filter expression of a try-except
		// warning C6322: empty _except block
		__try
		{
			// Raise exception to set thread name for attached debugger
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
		}
		__except (GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER)
		{

		}
	#pragma warning(pop)
	}

	inline void FrostSetThreadPriority(HANDLE hThread, DWORD dwPriority)
	{
		::SetThreadPriority(hThread, dwPriority);
	}

	inline void FrostSetThreadPriorityBoost(HANDLE hThread, bool bEnabled)
	{
		::SetThreadPriorityBoost(hThread, !bEnabled);
	}

	inline bool FrostCreateThread(const THREAD_CREATION_DESC& ThreadDesc, HANDLE* phThread)
	{
		const uint32_t nStackSize = ThreadDesc.nStackSizeInBytes != 0 ? ThreadDesc.nStackSizeInBytes : DEFAULT_THREAD_STACK_SIZE_KB * 1024;

		// Create thread
		unsigned int threadId = 0;
		*phThread = (void*)::_beginthreadex(NULL, nStackSize, ThreadDesc.pEntryFunc, ThreadDesc.pArgList, CREATE_SUSPENDED, &threadId);

		if (!(*phThread))
		{
			return false;
		}

		// Start thread
		::ResumeThread(*phThread);

		// Print info to log
		FrostLog("<ThreadInfo>: New thread \"%s\" | StackSize: %u(KB)", ThreadDesc.szThreadName, ThreadDesc.nStackSizeInBytes / 1024);
		return true;
	}
}