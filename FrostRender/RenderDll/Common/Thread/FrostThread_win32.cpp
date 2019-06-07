#include"Common\Platform\FrostWindows.h"
#include"FrostThread_win32.h"

namespace FrostThreadWin32
{
	static_assert(sizeof(FROST_CRITICAL_SECTION) == sizeof(CRITICAL_SECTION), "Win32 CRITICAL_SECTION size does not match CRY_CRITICAL_SECTION");
	static_assert(sizeof(FROST_SRWLOCK) == sizeof(SRWLOCK), "Win32 SRWLOCK size does not match CRY_SRWLOCK");
	static_assert(sizeof(FROST_CONDITION_VARIABLE) == sizeof(CONDITION_VARIABLE), "Win32 CONDITION_VARIABLE size does not match CRY_CONDITION_VARIABLE");

	FROST_SRWLOCK::FROST_SRWLOCK() : SRWLock_(0)
	{
		static_assert(sizeof(SRWLock_) == sizeof(PSRWLOCK), "RWLock-pointer has invalid size");
		::InitializeSRWLock(reinterpret_cast<PSRWLOCK>(&SRWLock_));
	}

	FROST_CONDITION_VARIABLE::FROST_CONDITION_VARIABLE() : CondVar_(0)
	{
		static_assert(sizeof(CondVar_) == sizeof(PCONDITION_VARIABLE), "ConditionVariable-pointer has invalid size");
		::InitializeConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(&CondVar_));
	}

	#pragma region FrostLock_SRWLOCK implementation
	void FrostLock_SRWLOCK::LockShared()
	{
		::AcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Win32LockType.SRWLock_));
	}

	void FrostLock_SRWLOCK::UnlockShared()
	{
		::ReleaseSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Win32LockType.SRWLock_));
	}

	bool FrostLock_SRWLOCK::TryLockShared()
	{
		return ::TryAcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Win32LockType.SRWLock_)) == TRUE;
	}

	void FrostLock_SRWLOCK::Lock()
	{
		::AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Win32LockType.SRWLock_));
	}

	void FrostLock_SRWLOCK::Unlock()
	{
		::ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Win32LockType.SRWLock_));
	}

	bool FrostLock_SRWLOCK::TryLock()
	{
		return ::TryAcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Win32LockType.SRWLock_)) == TRUE;
	}
	#pragma endregion	

	#pragma region FrostLock_SRWLOCK_Recursive implementation
	void FrostLock_SRWLOCK_Recursive::Lock()
	{
		const DWORD threadId = ::GetCurrentThreadId();

		if (threadId == m_ExclusiveOwningThreadId)
		{
			++m_RecurseCounter;
		}
		else
		{
			m_Win32LockType.Lock();
			assert(m_RecurseCounter == 0);
			assert(m_ExclusiveOwningThreadId == NULL);
			m_ExclusiveOwningThreadId = threadId;
		}
	}

	void FrostLock_SRWLOCK_Recursive::Unlock()
	{
		const DWORD threadId = ::GetCurrentThreadId();
		assert(m_ExclusiveOwningThreadId == threadId);

		if (m_RecurseCounter)
		{
			--m_RecurseCounter;
		}
		else
		{
			m_ExclusiveOwningThreadId = NULL;
			m_Win32LockType.Unlock();
		}
	}

	bool FrostLock_SRWLOCK_Recursive::TryLock()
	{
		const DWORD threadId = ::GetCurrentThreadId();
		if (m_ExclusiveOwningThreadId == threadId)
		{
			++m_RecurseCounter;
			return true;
		}
		else
		{
			const bool ret = m_Win32LockType.TryLock();
			if (ret)
			{
				m_ExclusiveOwningThreadId = threadId;
			}
			return ret;
		}
	}
	#pragma endregion

	#pragma region FrostLock_CriticalSection implementation
	FrostLock_CriticalSection::FrostLock_CriticalSection()
	{
		::InitializeCriticalSection((CRITICAL_SECTION*)&m_Win32LockType);
	}

	FrostLock_CriticalSection::~FrostLock_CriticalSection()
	{
		::DeleteCriticalSection((CRITICAL_SECTION*)&m_Win32LockType);
	}

	void FrostLock_CriticalSection::Lock()
	{
		::EnterCriticalSection((CRITICAL_SECTION*)&m_Win32LockType);
	}

	void FrostLock_CriticalSection::Unlock()
	{
		::LeaveCriticalSection((CRITICAL_SECTION*)&m_Win32LockType);
	}

	bool FrostLock_CriticalSection::TryLock()
	{
		return ::TryEnterCriticalSection((CRITICAL_SECTION*)&m_Win32LockType) == TRUE;
	}
	#pragma endregion

	#pragma region FrostLock_WinMutex implementation
	FrostLock_WinMutex::FrostLock_WinMutex()
		: m_Win32LockType(::CreateMutex(NULL, FALSE, NULL))
	{

	}

	FrostLock_WinMutex::~FrostLock_WinMutex()
	{
		::CloseHandle(m_Win32LockType);
	}

	void FrostLock_WinMutex::Lock()
	{
		::WaitForSingleObject(m_Win32LockType, INFINITE);
	}
	
	void FrostLock_WinMutex::Unlock()
	{
		::ReleaseMutex(m_Win32LockType);
	}

	bool FrostLock_WinMutex::TryLock()
	{
		return WaitForSingleObject(m_Win32LockType, 0) != WAIT_TIMEOUT;
	}
	#pragma endregion
}

#pragma region FrostEvent implementation
FrostEvent::FrostEvent()
{
	m_Handle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

FrostEvent::~FrostEvent()
{
	::CloseHandle(m_Handle);
}

void FrostEvent::Reset()
{
	::ResetEvent(m_Handle);
}

void FrostEvent::Set()
{
	::SetEvent(m_Handle);
}

void FrostEvent::Wait() const
{
	::WaitForSingleObject(m_Handle, INFINITE);
}

bool FrostEvent::Wait(DWORD TimeMillis) const
{
	if (::WaitForSingleObject(m_Handle, TimeMillis) == WAIT_TIMEOUT)
		return false;
	return true;
}
#pragma endregion

#pragma region FrostConditionVariable implementation
void FrostConditionVariable::Wait(FrostMutex& lock)
{
	TimeWait(lock, INFINITE);
}

void FrostConditionVariable::Wait(FrostMutexFast& lock)
{
	TimeWait(lock, INFINITE);
}

bool FrostConditionVariable::TimeWait(FrostMutex& lock, DWORD Millis)
{
	if (lock.s_type == FrostThreadWin32::FROST_LOCK_TYPE_SRW)
	{
		assert(lock.m_RecurseCounter == 0);
		lock.m_ExclusiveOwningThreadId = NULL;
		bool ret = ::SleepConditionVariableSRW(reinterpret_cast<PCONDITION_VARIABLE>(&m_CondVar), reinterpret_cast<PSRWLOCK>(&lock.m_Win32LockType), Millis, ULONG(0)) == TRUE;
		lock.m_ExclusiveOwningThreadId = ::GetCurrentThreadId();
		return ret;
	}
	else if (lock.s_type == FrostThreadWin32::FROST_LOCK_TYPE_CRITICAL_SECTION)
	{
		return SleepConditionVariableCS(reinterpret_cast<PCONDITION_VARIABLE>(&m_CondVar), reinterpret_cast<PCRITICAL_SECTION>(&lock.m_Win32LockType), Millis) == TRUE;
	}
}

bool FrostConditionVariable::TimeWait(FrostMutexFast& lock, DWORD Millis)
{
	if (lock.s_type == FrostThreadWin32::FROST_LOCK_TYPE_SRW)
	{
		return ::SleepConditionVariableSRW(reinterpret_cast<PCONDITION_VARIABLE>(&m_CondVar), reinterpret_cast<PSRWLOCK>(&lock.m_Win32LockType), Millis, ULONG(0)) == TRUE;
	}
	else if (lock.s_type == FrostThreadWin32::FROST_LOCK_TYPE_CRITICAL_SECTION)
	{
		return ::SleepConditionVariableCS(reinterpret_cast<PCONDITION_VARIABLE>(&m_CondVar), reinterpret_cast<PCRITICAL_SECTION>(&lock.m_Win32LockType), Millis) == TRUE;
	}
}

void FrostConditionVariable::NotifySingle()
{
	::WakeConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(&m_CondVar));
}

void FrostConditionVariable::Notify()
{
	::WakeAllConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(&m_CondVar));
}
#pragma endregion


#pragma region FrostSemaphore implementation
FrostSemaphore::FrostSemaphore(long maxCount, long initCount)
{
	m_Semaphore = (void*)::CreateSemaphore(NULL, initCount, maxCount, NULL);
}

FrostSemaphore::~FrostSemaphore()
{
	::CloseHandle((HANDLE)m_Semaphore);
}

void FrostSemaphore::Acquire()
{
	::WaitForSingleObject((HANDLE)m_Semaphore, INFINITE);
}

void FrostSemaphore::Release()
{
	::ReleaseSemaphore((HANDLE)m_Semaphore, 1, NULL);
}
#pragma endregion

#pragma region FrostFastSemaphore implementation
FrostFastSemaphore::FrostFastSemaphore(long maxCount, long initCount)
	: m_Semaphore(maxCount)
	, m_Counter(initCount)
{

}

FrostFastSemaphore::~FrostFastSemaphore()
{

}

void FrostFastSemaphore::Acquire()
{
	long count = 0;
	do
	{
		count = m_Counter;
	} while (::InterlockedCompareExchange(&m_Counter, count - 1, count) != count);

	// if the count would have been 0 or below, go to kernel semaphore
	if ((count - 1) < 0)
		m_Semaphore.Acquire();
}

void FrostFastSemaphore::Release()
{
	long count = 0;
	do
	{
		count = m_Counter;
	} while (::InterlockedCompareExchange(&m_Counter, count + 1, count) != count);

	// if the count would have been 0 or below, go to kernel semaphore
	if (count < 0)
		m_Semaphore.Release();
}
#pragma endregion

#pragma region FrostRWLock implementation
void FrostRWLock::RLock()
{
	m_SRWLock.LockShared();
}

bool FrostRWLock::TryRLock()
{
	return m_SRWLock.TryLockShared();
}

void FrostRWLock::RUnlock()
{
	m_SRWLock.UnlockShared();
}

void FrostRWLock::WLock()
{
	m_SRWLock.Lock();
}

bool FrostRWLock::TryWLock()
{
	return m_SRWLock.TryLock();
}

void FrostRWLock::WUnlock()
{
	m_SRWLock.Unlock();
}
#pragma endregion
