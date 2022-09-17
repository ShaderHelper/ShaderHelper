// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	UnixPlatformString.h: Unix platform string classes, mostly implemented with ANSI C++
==============================================================================================*/

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/StandardPlatformString.h"
#include "GenericPlatform/GenericWidePlatformString.h"
/**
* Unix string implementation
**/
struct FUnixPlatformString : public 
#if PLATFORM_TCHAR_IS_CHAR16
	FGenericWidePlatformString
#else
	FStandardPlatformString
#endif
{
	template<typename CHAR>
	static FORCEINLINE int32 Strlen(const CHAR* String)
	{
		if(!String)
			return 0;

		int Len = 0;
		while(String[Len])
		{
			++Len;
		}

		return Len;
	}
};

typedef FUnixPlatformString FPlatformString;

// Format specifiers to be able to print values of these types correctly, for example when using UE_LOG.
// SIZE_T format specifier
#define SIZE_T_FMT "zu"
// SIZE_T format specifier for lowercase hexadecimal output
#define SIZE_T_x_FMT "zx"
// SIZE_T format specifier for uppercase hexadecimal output
#define SIZE_T_X_FMT "zX"

#if PLATFORM_64BITS
// SSIZE_T format specifier
#define SSIZE_T_FMT "lld"
// SSIZE_T format specifier for lowercase hexadecimal output
#define SSIZE_T_x_FMT "llx"
// SSIZE_T format specifier for uppercase hexadecimal output
#define SSIZE_T_X_FMT "llX"

// UPTRINT format specifier for decimal output
#define UPTRINT_FMT "llu"
// UPTRINT format specifier for lowercase hexadecimal output
#define UPTRINT_x_FMT "llx"
// UPTRINT format specifier for uppercase hexadecimal output
#define UPTRINT_X_FMT "llX"
#else
// SSIZE_T format specifier
#define SSIZE_T_FMT "d"
// SSIZE_T format specifier for lowercase hexadecimal output
#define SSIZE_T_x_FMT "x"
// SSIZE_T format specifier for uppercase hexadecimal output
#define SSIZE_T_X_FMT "X"

// UPTRINT format specifier for decimal output
#define UPTRINT_FMT "u"
// UPTRINT format specifier for lowercase hexadecimal output
#define UPTRINT_x_FMT "x"
// UPTRINT format specifier for uppercase hexadecimal output
#define UPTRINT_X_FMT "X"
#endif

// PTRINT format specifier for decimal output
#define PTRINT_FMT SSIZE_T_FMT
// PTRINT format specifier for lowercase hexadecimal output
#define PTRINT_x_FMT SSIZE_T_x_FMT
// PTRINT format specifier for uppercase hexadecimal output
#define PTRINT_X_FMT SSIZE_T_X_FMT

// int64 format specifier for decimal output
#define INT64_FMT "lld"
// int64 format specifier for lowercase hexadecimal output
#define INT64_x_FMT "llx"
// int64 format specifier for uppercase hexadecimal output
#define INT64_X_FMT "llX"

// uint64 format specifier for decimal output
#define UINT64_FMT "llu"
// uint64 format specifier for lowercase hexadecimal output
#define UINT64_x_FMT "llx"
// uint64 format specifier for uppercase hexadecimal output
#define UINT64_X_FMT "llX"
