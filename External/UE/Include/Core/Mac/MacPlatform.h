// Copyright Epic Games, Inc. All Rights Reserved.

/*================================================================================
	MacPlatform.h: Setup for the Mac platform
==================================================================================*/

#pragma once

#include "Clang/ClangPlatform.h"

#define PLATFORM_MAC_USE_CHAR16 1

/**
* Mac specific types
**/
struct FMacPlatformTypes : public FGenericPlatformTypes
{
	typedef unsigned int		DWORD;
	typedef size_t				SIZE_T;
	typedef decltype(NULL)		TYPE_OF_NULL;
#if PLATFORM_MAC_USE_CHAR16
	typedef char16_t			WIDECHAR;
	typedef WIDECHAR			TCHAR;
#else
	typedef char16_t			CHAR16;
#endif
};

typedef FMacPlatformTypes FPlatformTypes;

// Define ARM64 / X86 here so we can run UBT once for both platforms
#if __is_target_arch(arm64) || __is_target_arch(arm64e)
    #define PLATFORM_MAC_ARM64 1
    #define PLATFORM_MAC_X86 0
#else
    #define PLATFORM_MAC_ARM64 0
    #define PLATFORM_MAC_X86 1
#endif

// Base defines, must define these for the platform, there are no defaults
#define PLATFORM_DESKTOP				1
#define PLATFORM_64BITS					1
// Technically the underlying platform has 128bit atomics, but clang might not issue optimal code
#define PLATFORM_HAS_128BIT_ATOMICS		0
#define PLATFORM_CAN_SUPPORT_EDITORONLY_DATA	1

// Base defines, defaults are commented out

#define PLATFORM_LITTLE_ENDIAN						1
//#define PLATFORM_EXCEPTIONS_DISABLED				!PLATFORM_DESKTOP
#define PLATFORM_SEH_EXCEPTIONS_DISABLED				1
#define PLATFORM_SUPPORTS_PRAGMA_PACK				1
#define PLATFORM_ENABLE_VECTORINTRINSICS			1
#define PLATFORM_ENABLE_VECTORINTRINSICS_NEON       PLATFORM_MAC_ARM64
#ifndef PLATFORM_MAYBE_HAS_SSE4_1  // May be set from UnrealBuildTool
	#define PLATFORM_MAYBE_HAS_SSE4_1				PLATFORM_MAC_X86
#endif
// Current unreal minspec is sse2, not sse4, so on mac any calling code must check _cpuid before calling SSE4 instructions
// If called on a platform for which _cpuid for SSE4 returns false, attempting to call SSE4 intrinsics will crash
// If your title has raised the minspec to sse4, you can define PLATFORM_ALWAYS_HAS_SSE4_1 to 1
#ifndef PLATFORM_ALWAYS_HAS_SSE4_1 // May be set from UnrealBuildTool
	#define PLATFORM_ALWAYS_HAS_SSE4_1				0
#endif
// FMA3 support was added starting from Intel Haswell
#ifndef PLATFORM_ALWAYS_HAS_FMA3
	#define PLATFORM_ALWAYS_HAS_FMA3				0
#endif
//#define PLATFORM_USE_LS_SPEC_FOR_WIDECHAR			1
#define PLATFORM_USE_SYSTEM_VSWPRINTF				0
#define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG			1
#define PLATFORM_WCHAR_IS_4_BYTES					1
#if PLATFORM_MAC_USE_CHAR16
	#define PLATFORM_TCHAR_IS_CHAR16					1
#else
	#define PLATFORM_TCHAR_IS_4_BYTES					1
#endif
#define PLATFORM_HAS_BSD_TIME							1
#define PLATFORM_HAS_BSD_IPV6_SOCKETS					1
//#define PLATFORM_USE_PTHREADS							1
#define PLATFORM_MAX_FILEPATH_LENGTH_DEPRECATED			MAC_MAX_PATH
#define PLATFORM_SUPPORTS_TBB							1
#define PLATFORM_SUPPORTS_MIMALLOC						PLATFORM_64BITS
#define PLATFORM_SUPPORTS_STACK_SYMBOLS					1
#define PLATFORM_HAS_BSD_SOCKET_FEATURE_MSG_DONTWAIT	1
#define PLATFORM_IS_ANSI_MALLOC_THREADSAFE				1
#define PLATFORM_SUPPORTS_VIRTUAL_TEXTURE_STREAMING		1

#define PLATFORM_RHITHREAD_DEFAULT_BYPASS				0

#define PLATFORM_ENABLE_POPCNT_INTRINSIC				1

#define PLATFORM_GLOBAL_LOG_CATEGORY					LogMac

#if PLATFORM_MAC_X86
	#define PLATFORM_BREAK()							__asm__("int $3")
#else
    #define PLATFORM_BREAK()                            __builtin_trap()
#endif

#define PLATFORM_CODE_SECTION(Name)						__attribute__((section("__TEXT,__" Name ",regular,pure_instructions")))

#if __has_feature(cxx_decltype_auto)
	#define PLATFORM_COMPILER_HAS_DECLTYPE_AUTO			1
#else
	#define PLATFORM_COMPILER_HAS_DECLTYPE_AUTO			0
#endif

// Function type macros.
#define VARARGS															/* Functions with variable arguments */
#define CDECL															/* Standard C function */
#define STDCALL															/* Standard calling convention */
#if UE_BUILD_DEBUG
#define FORCEINLINE inline 												/* Don't force code to be inline */
#else
#define FORCEINLINE inline __attribute__ ((always_inline))				/* Force code to be inline */
#endif

#define FORCENOINLINE __attribute__((noinline))							/* Force code to NOT be inline */
#define FUNCTION_CHECK_RETURN_END __attribute__ ((warn_unused_result))	/* Warn that callers should not ignore the return value. */
#define FUNCTION_NO_RETURN_END __attribute__ ((noreturn))				/* Indicate that the function never returns. */

#define ABSTRACT abstract

// Alignment.
#define GCC_PACK(n) __attribute__((packed,aligned(n)))
#define GCC_ALIGN(n) __attribute__((aligned(n)))

// operator new/delete operators
// As of 10.9 we need to use _NOEXCEPT & cxx_noexcept compatible definitions
#if __has_feature(cxx_noexcept)
#define OPERATOR_NEW_THROW_SPEC
#else
#define OPERATOR_NEW_THROW_SPEC throw (std::bad_alloc)
#endif
#define OPERATOR_DELETE_THROW_SPEC _NOEXCEPT
#define OPERATOR_NEW_NOTHROW_SPEC  _NOEXCEPT
#define OPERATOR_DELETE_NOTHROW_SPEC  _NOEXCEPT

// DLL export and import definitions
#define DLLEXPORT			__attribute__((visibility("default")))
#define DLLIMPORT			__attribute__((visibility("default")))

#define MAC_MAX_PATH 1024
