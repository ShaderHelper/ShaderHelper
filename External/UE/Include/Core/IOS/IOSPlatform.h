// Copyright Epic Games, Inc. All Rights Reserved.

/*================================================================================
	IOSPlatform.h: Setup for the iOS platform
==================================================================================*/

#pragma once

#include "Clang/ClangPlatform.h"
#include "Availability.h"

/**
* iOS specific types
**/
struct FIOSPlatformTypes : public FGenericPlatformTypes
{
	typedef size_t				SIZE_T;
	typedef decltype(NULL)		TYPE_OF_NULL;
	typedef char16_t			WIDECHAR;
	typedef WIDECHAR			TCHAR;
};

typedef FIOSPlatformTypes FPlatformTypes;

// Base defines, must define these for the platform, there are no defaults
#define PLATFORM_DESKTOP				0

#ifdef __LP64__
#define PLATFORM_64BITS					1
// Technically the underlying platform has 128bit atomics, but clang might not issue optimal code
#define PLATFORM_HAS_128BIT_ATOMICS		0
#else
#define PLATFORM_64BITS					0
#define PLATFORM_HAS_128BIT_ATOMICS		0
#endif

// Base defines, defaults are commented out
#define PLATFORM_LITTLE_ENDIAN							1
#define PLATFORM_SEH_EXCEPTIONS_DISABLED				1
#define PLATFORM_SUPPORTS_PRAGMA_PACK					1
#define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG	1
#define PLATFORM_WCHAR_IS_4_BYTES						1
#define PLATFORM_TCHAR_IS_CHAR16						1
#define PLATFORM_USE_SYSTEM_VSWPRINTF					0
#define PLATFORM_HAS_BSD_TIME							1
#define PLATFORM_HAS_BSD_IPV6_SOCKETS					1
#define PLATFORM_HAS_BSD_SOCKET_FEATURE_MSG_DONTWAIT	1
#define PLATFORM_MAX_FILEPATH_LENGTH_DEPRECATED			IOS_MAX_PATH
#define PLATFORM_SUPPORTS_TEXTURE_STREAMING				1
#define PLATFORM_BUILTIN_VERTEX_HALF_FLOAT				0
#define PLATFORM_SUPPORTS_MULTIPLE_NATIVE_WINDOWS		0
#define PLATFORM_ALLOW_NULL_RHI							1
#define PLATFORM_ENABLE_VECTORINTRINSICS				1
#define PLATFORM_ENABLE_VECTORINTRINSICS_NEON			1
#define PLATFORM_SUPPORTS_STACK_SYMBOLS					1
#define PLATFORM_SUPPORTS_EARLY_MOVIE_PLAYBACK			1 // movies will start before engine is initalized
#define PLATFORM_USE_FULL_TASK_GRAPH					0 // @todo platplug: not platplug, but should investigate soon anyway
#define PLATFORM_IS_ANSI_MALLOC_THREADSAFE				1

// on iOS we now perform offline symbolication as it's significantly faster. Requires bGenerateCrashReportSymbols=true in the ini file.
#define	PLATFORM_RUNTIME_MALLOCPROFILER_SYMBOLICATION	0	
#define PLATFORM_NUM_AUDIODECOMPRESSION_PRECACHE_BUFFERS		0
#if PLATFORM_TVOS
#define PLATFORM_USES_GLES								0
#define PLATFORM_HAS_TOUCH_MAIN_SCREEN					0
#define	PLATFORM_SUPPORTS_OPUS_CODEC					0
#define PLATFORM_SUPPORTS_VORBIS_CODEC					0
#else
#define PLATFORM_USES_GLES								1
#define PLATFORM_HAS_TOUCH_MAIN_SCREEN					1
#endif
#define PLATFORM_UI_HAS_MOBILE_SCROLLBARS				1
#define PLATFORM_UI_NEEDS_TOOLTIPS						0
#define PLATFORM_UI_NEEDS_FOCUS_OUTLINES				0

#define PLATFORM_NEEDS_RHIRESOURCELIST					0
#define PLATFORM_SUPPORTS_GEOMETRY_SHADERS				0
#define PLATFORM_SUPPORTS_VIRTUAL_TEXTURE_STREAMING		1
#define PLATFORM_SUPPORTS_LANDSCAPE_VISUAL_MESH_LOD_STREAMING 1

#define PLATFORM_GLOBAL_LOG_CATEGORY					LogIOS

#define PLATFORM_BREAK()                                __builtin_trap()

#define PLATFORM_CODE_SECTION(Name)						__attribute__((section("__TEXT,__" Name ",regular,pure_instructions"))) \
														__attribute__((aligned(4)))

#if __has_feature(cxx_decltype_auto)
	#define PLATFORM_COMPILER_HAS_DECLTYPE_AUTO 1
#else
	#define PLATFORM_COMPILER_HAS_DECLTYPE_AUTO 0
#endif

//mallocpoison not safe with aligned ansi allocator.  returns the larger unaligned size during Free() which causes writes off the end of the allocation.
#define UE_USE_MALLOC_FILL_BYTES 0 

#define PLATFORM_RHITHREAD_DEFAULT_BYPASS				1

// Function type macros.
#define VARARGS															/* Functions with variable arguments */
#define CDECL															/* Standard C function */
#define STDCALL															/* Standard calling convention */
#if UE_BUILD_DEBUG || UE_DISABLE_FORCE_INLINE
#define FORCEINLINE inline 												/* Don't force code to be inline */
#else
#define FORCEINLINE inline __attribute__ ((always_inline))				/* Force code to be inline */
#endif
#define FORCENOINLINE __attribute__((noinline))							/* Force code to NOT be inline */
#define FUNCTION_CHECK_RETURN_END __attribute__ ((warn_unused_result))	/* Warn that callers should not ignore the return value. */
#define FUNCTION_NO_RETURN_END __attribute__ ((noreturn))				/* Indicate that the function never returns. */

#define ABSTRACT abstract

// Strings.
#define LINE_TERMINATOR TEXT("\n")
#define LINE_TERMINATOR_ANSI "\n"

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
#define OPERATOR_DELETE_THROW_SPEC noexcept
#define OPERATOR_NEW_NOTHROW_SPEC  noexcept
#define OPERATOR_DELETE_NOTHROW_SPEC  noexcept

// DLL export and import definitions
#define DLLEXPORT __attribute__((visibility("default")))
#define DLLIMPORT

#define IOS_MAX_PATH 1024

static_assert(__IPHONE_OS_VERSION_MAX_ALLOWED >= 13000, "Unreal requires Xcode 11 or later to build"); 
