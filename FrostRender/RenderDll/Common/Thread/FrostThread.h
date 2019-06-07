#pragma once

#include"FrostThread_win32.h"

#define THREAD_NAME_LENGTH_MAX 64

template<class LockClass>
class FrostAutoLock
{
public:
	FrostAutoLock(LockClass& lock) : m_pLock(&lock)
	{
		m_pLock->Lock();
	}

	FrostAutoLock(const LockClass& lock)
		: m_pLock(const_cast<LockClass*>(&lock))
	{
		m_pLock->Lock();
	}

	~FrostAutoLock()
	{
		m_pLock->Unlock();
	}
private:
	FrostAutoLock(const FrostAutoLock&) = delete;
	FrostAutoLock& operator=(const FrostAutoLock&) = delete;

	LockClass* m_pLock;
};