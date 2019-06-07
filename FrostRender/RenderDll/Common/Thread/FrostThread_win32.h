#pragma once

class FrostConditionVariable;

namespace FrostThreadWin32
{
	enum FROST_LOCK_TYPE
	{
		FROST_LOCK_TYPE_CRITICAL_SECTION,
		FROST_LOCK_TYPE_SRW,
		FROST_LOCK_TYPE_MUTEX
	};

	/* 
		From winnt.h.
		Since we are not allowed to include windows.h while being included from platform.h 
		and there seems to be no good way to include the required windows headers directly;
		without including a lot of other header, define a 1:1 copy of the required primitives defined in winnt.h .
	 */

	struct FROST_CRITICAL_SECTION //From winnt.h
	{
		void*          DebugInfo;
		long           LockCount;
		long           RecursionCount;
		void*          OwningThread;
		void*          LockSemaphore;
		unsigned long* SpinCount;  //!< Force size on 64-bit systems when packed.
	};

	struct FROST_SRWLOCK //From winnt.h
	{
		FROST_SRWLOCK();
		void* SRWLock_;
	};

	struct FROST_CONDITION_VARIABLE //From winnt.h
	{
		FROST_CONDITION_VARIABLE();
		void* CondVar_;
	};

	class FrostLock_SRWLOCK
	{
	public:
		static const FROST_LOCK_TYPE s_type = FROST_LOCK_TYPE_SRW;
		friend class ::FrostConditionVariable;
		
		FrostLock_SRWLOCK() = default;

		void Lock();
		void Unlock();
		bool TryLock();

		void LockShared();
		void UnlockShared();
		bool TryLockShared();

	private:
		FrostLock_SRWLOCK(const FrostLock_SRWLOCK&) = delete;
		FrostLock_SRWLOCK& operator=(const FrostLock_SRWLOCK&) = delete;

		FROST_SRWLOCK m_Win32LockType;
	};

	/*	
		SRW Lock (Slim Reader/Writer Lock)
		Faster + lighter than CriticalSection. Also only enters into kernel mode if contended.
		Cannot be shared between processes.
	*/

	class FrostLock_SRWLOCK_Recursive
	{
	public:
		static const FROST_LOCK_TYPE s_type = FROST_LOCK_TYPE_SRW;
		friend class ::FrostConditionVariable;

		FrostLock_SRWLOCK_Recursive(): m_RecurseCounter(0),m_ExclusiveOwningThreadId(NULL) {}

		void Lock();
		void Unlock();
		bool TryLock();

	private:
		FrostLock_SRWLOCK_Recursive(const FrostLock_SRWLOCK_Recursive&) = delete;
		FrostLock_SRWLOCK_Recursive& operator=(const FrostLock_SRWLOCK_Recursive&) = delete;

		FrostLock_SRWLOCK m_Win32LockType;
		UINT32			  m_RecurseCounter;

		// Due to its semantics, this member can be accessed in an unprotected manner,
		// but only for comparison with the current tid.
		DWORD			  m_ExclusiveOwningThreadId;
	};

	/*
		Critical section
		Faster then WinMutex as it only enters into kernel mode if contended.
		Cannot be shared between processes.
	*/
	class FrostLock_CriticalSection
	{
	public:
		static const FROST_LOCK_TYPE s_type = FROST_LOCK_TYPE_CRITICAL_SECTION;
		friend class ::FrostConditionVariable;

		FrostLock_CriticalSection();
		~FrostLock_CriticalSection();

		void Lock();
		void Unlock();
		bool TryLock();

	private:
		FrostLock_CriticalSection(const FrostLock_CriticalSection&) = delete;
		FrostLock_CriticalSection& operator=(const FrostLock_CriticalSection&) = delete;

		FROST_CRITICAL_SECTION m_Win32LockType;
	};

	/*
		WinMutex: (slow)
		Calls into kernel even when not contended.
		A named mutex can be shared between different processes.
	*/
	class FrostLock_WinMutex
	{
	public:
		static const FROST_LOCK_TYPE s_type = FROST_LOCK_TYPE_MUTEX;

		FrostLock_WinMutex();
		~FrostLock_WinMutex();

		void Lock();
		void Unlock();
		bool TryLock();

	private:
		FrostLock_WinMutex(const FrostLock_WinMutex&) = delete;
		FrostLock_WinMutex& operator=(const FrostLock_WinMutex&) = delete;

		HANDLE m_Win32LockType;
	};

	inline void FrostYieldThread()
	{
		::SwitchToThread();
	}

}

typedef FrostThreadWin32::FrostLock_SRWLOCK_Recursive FrostMutex;
typedef FrostThreadWin32::FrostLock_SRWLOCK			  FrostMutexFast; //Not recursive
											//! CryEvent represent a synchronization event.
class FrostEvent
{
public:
	FrostEvent();
	~FrostEvent();

	//! Reset the event to the unsignalled state.
	void Reset();

	//! Set the event to the signalled state.
	void Set();

	//! Access a HANDLE to wait on.
	void* GetHandle() const { return m_Handle; };

	//! Wait indefinitely for the object to become signalled.
	void Wait() const;

	//! Wait, with a time limit, for the object to become signalled.
	bool Wait(const DWORD TimeoutMillis) const;

private:
	FrostEvent(const FrostEvent&) = delete;
	FrostEvent& operator=(const FrostEvent&) = delete;

	HANDLE m_Handle;
};

class FrostConditionVariable
{
public:
	FrostConditionVariable() = default;

	void Wait(FrostMutex& lock);
	void Wait(FrostMutexFast& lock);
	bool TimeWait(FrostMutex& lock, DWORD Millis);
	bool TimeWait(FrostMutexFast& lock, DWORD Millis);
	void NotifySingle();
	void Notify();

private:
	FrostConditionVariable(const FrostConditionVariable&) = delete;
	FrostConditionVariable& operator=(const FrostConditionVariable&) = delete;

	FrostThreadWin32::FROST_CONDITION_VARIABLE m_CondVar;
};


//! Platform independent wrapper for a counting semaphore.
class FrostSemaphore
{
public:
	FrostSemaphore(long maxCount, long initCount = 0);
	~FrostSemaphore();
	void Acquire();
	void Release();

private:
	void* m_Semaphore;
};

/*
! Platform independent wrapper for a counting semaphore
! except that this version uses C-A-S only until a blocking call is needed.
! -> No kernel call if there are object in the semaphore.
*/
class FrostFastSemaphore
{
public:
	FrostFastSemaphore(long maxCount, long initCount = 0);
	~FrostFastSemaphore();
	void Acquire();
	void Release();

private:
	FrostSemaphore m_Semaphore;
	volatile long m_Counter;
};

class FrostRWLock
{
public:
	FrostRWLock() = default;

	void RLock();
	bool TryRLock();
	void RUnlock();

	void WLock();
	bool TryWLock();
	void WUnlock();

private:
	FrostRWLock(const FrostRWLock&) = delete;
	FrostRWLock& operator=(const FrostRWLock&) = delete;

	FrostThreadWin32::FrostLock_SRWLOCK m_SRWLock;
};



