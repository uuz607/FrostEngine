#pragma once

#include <intrin.h>

/* Interlocked API*/

// Returns the resulting incremented value
inline LONG FrostInterlockedIncrement(volatile int* pDst)
{
	static_assert(sizeof(int) == sizeof(LONG), "Unsecured cast. int is not same size as LONG.");
	return ::_InterlockedIncrement((volatile LONG*)pDst);
}

// Returns the resulting decremented value
inline LONG FrostInterlockedDecrement(volatile int* pDst)
{
	static_assert(sizeof(int) == sizeof(LONG), "Unsecured cast. int is not same size as LONG.");
	return ::_InterlockedDecrement((volatile LONG*)pDst);
}

#pragma region FrostInterlockedAdd
// Returns the resulting added value
inline LONG FrostInterlockedAdd(volatile LONG* pDst, LONG add)
{
	return ::_InterlockedExchangeAdd(pDst, add) + add;
}

//Returns the resulting added value
inline LONG FrostInterlockedAdd(volatile int* pDst, int add)
{
	return FrostInterlockedAdd((volatile LONG*)pDst, (LONG)add);
}

// Returns the resulting added value
inline size_t FrostInterlockedAdd(volatile size_t* pDst, size_t add)
{
#if FROST_PLATFORM_X64
	static_assert(sizeof(size_t) == sizeof(LONG64), "Unsecured cast. size_t is not same size as LONG64.");
	return (size_t)::_InterlockedExchangeAdd64((volatile LONG64*)pDst, add) + add;
#else
	static_assert(sizeof(size_t) == sizeof(LONG), "Unsecured cast. size_t is not same size as LONG.");
	return (size_t)FrostInterlockedAdd((volatile LONG*)pDst, (LONG)add);
#endif 
}
#pragma endregion

#pragma region FrostInterlockedExchange
// Returns initial value prior exchange
inline LONG FrostInterlockedExchange(volatile LONG* pDst, LONG exchange)
{
	return ::_InterlockedExchange(pDst, exchange);
}

// Returns initial value prior exchange
inline LONG FrostInterlockedExchange(volatile int* pDst, int exchange)
{
	return FrostInterlockedExchange((volatile LONG*)pDst, (LONG)exchange);
}
#pragma endregion

#pragma region FrostInterlockedExchangeAdd
inline LONG FrostInterlockedExchangeAdd(volatile LONG* pDst, volatile LONG value)
{
	return ::_InterlockedExchangeAdd(pDst, value);
}
// Returns initial value prior exchange
inline size_t FrostInterlockedExchangeAdd(volatile size_t* pDst, size_t add)
{
#if FROST_PLATFORM_X64
	static_assert(sizeof(size_t) == sizeof(LONGLONG), "Unsecured cast. size_t is not same size as LONGLONG(int64).");
	return ::_InterlockedExchangeAdd64((volatile LONGLONG*)pDst, add); // intrinsic returns previous value
#else
	static_assert(sizeof(size_t) == sizeof(LONG), "Unsecured cast. size_t is not same size as LONG.");
	return (size_t)FrostInterlockedExchangeAdd((volatile LONG*)pDst, (LONG)add); // intrinsic returns previous value
#endif
}
#pragma endregion


// Returns initial value prior exchange
inline LONG FrostInterlockedExchangeAnd(volatile LONG* pDst, LONG value)
{
	return ::_InterlockedAnd(pDst, value);
}

// Returns initial value prior exchange
inline LONG FrostInterlockedExchangeOr(volatile LONG* pDst, LONG value)
{
	return ::_InterlockedOr(pDst, value);
}

// Returns initial address prior exchange
inline void* FrostInterlockedExchangePointer(void* volatile* pDst, void* pExchange)
{
#if FROST_PLATFORM_X64 || _MSC_VER > 1700
	return ::_InterlockedExchangePointer(pDst, pExchange);
#else
	static_assert(sizeof(void*) == sizeof(LONG), "Unsecured cast. void* is not same size as LONG.");
	return (void*)::_InterlockedExchange((LONG volatile*)pDst, (LONG)pExchange);
#endif
}

// Returns initial address prior exchange
inline LONG FrostInterlockedCompareExchange(LONG volatile* pDst, LONG exchange, LONG comperand)
{
	return ::_InterlockedCompareExchange(pDst, exchange, comperand);
}

// Returns initial address prior exchange
inline void* FrostInterlockedCompareExchangePointer(void* volatile* pDst, void* pExchange, void* pComperand)
{
#if FROST_PLATFORM_X64 || _MSC_VER > 1700
	return ::_InterlockedCompareExchangePointer(pDst, pExchange, pComperand);
#else
	static_assert(sizeof(void*) == sizeof(LONG), "Unsecured cast. void* is not same size as LONG.");
	return (void*)::_InterlockedCompareExchange((LONG volatile*)pDst, (LONG)pExchange, (LONG)pComperand);
#endif
}

// Returns initial value prior exchange
inline INT64 FrostInterlockedCompareExchange64(volatile INT64* pDst, INT64 exchange, INT64 compare)
{
	static_assert(sizeof(INT64) == sizeof(__int64), "Unsecured cast. int64 is not same size as __int64.");
	return ::_InterlockedCompareExchange64(pDst, exchange, compare);
}

#if FROST_PLATFORM_64BIT
// Returns initial value prior exchange
inline unsigned char FrostInterlockedCompareExchange128(volatile INT64* pDst, INT64 exchangeHigh, INT64 exchangeLow, INT64* pComparandResult)
{
	static_assert(sizeof(INT64) == sizeof(__int64), "Unsecured cast. int64 is not same size as __int64.");
	FROST_ASSERT_MESSAGE((((INT64)pDst) & 15) == 0, "The destination data must be 16-byte aligned to avoid a general protection fault.");
	return ::_InterlockedCompareExchange128(pDst, exchangeHigh, exchangeLow, pComparandResult);
}
#endif
