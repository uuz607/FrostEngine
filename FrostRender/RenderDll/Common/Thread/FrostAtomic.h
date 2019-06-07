#pragma once

#include <intsafe.h>
#include <intrin.h>

// Returns the resulting incremented value
inline LONG FrostInterlockedIncrement(volatile int* pDst)
{
	static_assert(sizeof(int) == sizeof(LONG), "Unsecured cast. int is not same size as LONG.");
	return _InterlockedIncrement((volatile LONG*)pDst);
}

// Returns the resulting decremented value
inline LONG FrostInterlockedDecrement(volatile int* pDst)
{
	static_assert(sizeof(int) == sizeof(LONG), "Unsecured cast. int is not same size as LONG.");
	return _InterlockedDecrement((volatile LONG*)pDst);
}

// Returns the resulting added value
inline LONG CryInterlockedAdd(volatile int* pDst, int add)
{
	return CryInterlockedAdd((volatile LONG*)pDst, (LONG)add);
}

// Returns the resulting added value
inline LONG CryInterlockedAdd(volatile LONG* pDst, LONG add)
{
#if CRY_PLATFORM_X64
	return _InterlockedExchangeAdd(pDst, add) + add;
#else
	return CryInterlockedExchangeAdd(pDst, add) + add;
#endif
}

// Returns initial value prior exchange
inline LONG CryInterlockedExchange(volatile LONG* pDst, LONG exchange)
{
	return _InterlockedExchange(pDst, exchange);
}
