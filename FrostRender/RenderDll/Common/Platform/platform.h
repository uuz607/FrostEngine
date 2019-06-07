#pragma once

#include"FrostPlatformDefines.h"

#ifndef CHECK_REFERENCE_COUNTS     //define that in your StdAfx.h to override per-project
#define CHECK_REFERENCE_COUNTS 1 //default value
#endif

#if CHECK_REFERENCE_COUNTS
#define CHECK_REFCOUNT_CRASH(x) { if (!(x)) *((int*)0) = 0; }
#else
#define CHECK_REFCOUNT_CRASH(x)
#endif

#if !defined(_DEBUG)
	#define ILINE CRY_FORCE_INLINE
#else
	#define  ILINE inline
#endif

#if (_MSC_VER >= 1800)
	#define _ALLOW_INITIALIZER_LISTS
#endif


#define BIT(x)    (1U << (x))
#define BIT64(x)  (1ULL << (x))
#define MASK(x)   (BIT(x) - 1U)
#define MASK64(x) (BIT64(x) - 1ULL)
