// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/Build.h"
#include "Misc/LargeWorldCoordinates.h"

// define all other platforms to be zero
//@port Define the platform here to be zero when compiling for other platforms
#if !defined(PLATFORM_WINDOWS)
	#define PLATFORM_WINDOWS 0
#endif
#if !defined(PLATFORM_XBOXONE)
	#define PLATFORM_XBOXONE 0
#endif
#if !defined(PLATFORM_MAC)
	#define PLATFORM_MAC 0
	// If PLATFORM_MAC is defined these will be set appropriately in
	// MacPlatform.h
	#define PLATFORM_MAC_X86 0
	#define PLATFORM_MAC_ARM64 0
#endif
#if !defined(PLATFORM_PS4)
	#define PLATFORM_PS4 0
#endif
#if !defined(PLATFORM_IOS)
	#define PLATFORM_IOS 0
#endif
#if !defined(PLATFORM_TVOS)
	#define PLATFORM_TVOS 0
#endif
#if !defined(PLATFORM_ANDROID)
	#define PLATFORM_ANDROID 0
#endif
#if !defined(PLATFORM_ANDROID_ARM)
	#define PLATFORM_ANDROID_ARM 0
#endif
#if !defined(PLATFORM_ANDROID_ARM64)
	#define PLATFORM_ANDROID_ARM64 0
#endif
#if !defined(PLATFORM_ANDROID_X86)
	#define PLATFORM_ANDROID_X86 0
#endif
#if !defined(PLATFORM_ANDROID_X64)
	#define PLATFORM_ANDROID_X64 0
#endif
#if !defined(PLATFORM_APPLE)
	#define PLATFORM_APPLE 0
#endif
#if !defined(PLATFORM_LINUX)
	#define PLATFORM_LINUX 0
#endif
#if !defined(PLATFORM_LINUXARM64)
	#define PLATFORM_LINUXARM64 0
#endif
#if !defined(PLATFORM_SWITCH)
	#define PLATFORM_SWITCH 0
#endif
#if !defined(PLATFORM_FREEBSD)
	#define PLATFORM_FREEBSD 0
#endif
#if !defined(PLATFORM_UNIX)
	#define PLATFORM_UNIX 0
#endif
#if !defined(PLATFORM_MICROSOFT)
	#define PLATFORM_MICROSOFT 0
#endif
#if !defined(PLATFORM_HOLOLENS)
#define PLATFORM_HOLOLENS 0
#endif

// Platform specific compiler pre-setup.
#include "PreprocessorHelpers.h"
#include COMPILED_PLATFORM_HEADER(PlatformCompilerPreSetup.h)

// Generic compiler pre-setup.
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

// Whether the CPU is x86/x64 (i.e. both 32 and 64-bit variants)
#ifndef PLATFORM_CPU_X86_FAMILY
	#if (defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__amd64__) || defined(__x86_64__))
		#define PLATFORM_CPU_X86_FAMILY	1
	#else
		#define PLATFORM_CPU_X86_FAMILY	0
	#endif
#endif

// Whether the CPU is AArch32/AArch64 (i.e. both 32 and 64-bit variants)
#ifndef PLATFORM_CPU_ARM_FAMILY
	#if (defined(__arm__) || defined(_M_ARM) || defined(__aarch64__) || defined(_M_ARM64))
		#define PLATFORM_CPU_ARM_FAMILY	1
	#else
		#define PLATFORM_CPU_ARM_FAMILY	0
	#endif
#endif

#if PLATFORM_APPLE
	#include <stddef.h> // needed for size_t
#endif

#include "GenericPlatform/GenericPlatform.h"

//------------------------------------------------------------------
// Setup macros for static code analysis
//------------------------------------------------------------------
#ifndef PLATFORM_COMPILER_CLANG
#if defined(__clang__)
#define PLATFORM_COMPILER_CLANG			1
#else
#define PLATFORM_COMPILER_CLANG			0
#endif // defined(__clang__)
#endif

#if PLATFORM_WINDOWS
	#include "Windows/WindowsPlatformCodeAnalysis.h"
#elif PLATFORM_COMPILER_CLANG
	#include "Clang/ClangPlatformCodeAnalysis.h"
#endif

#ifndef USING_ADDRESS_SANITISER
	#define USING_ADDRESS_SANITISER 0
#endif

//---------------------------------------------------------
// Include main platform setup header (XXX/XXXPlatform.h)
//---------------------------------------------------------

#include COMPILED_PLATFORM_HEADER(Platform.h)

//------------------------------------------------------------------
// Finalize define setup
//------------------------------------------------------------------

// Base defines, must define these for the platform, there are no defaults
#ifndef PLATFORM_DESKTOP
	#error "PLATFORM_DESKTOP must be defined"
#endif
#ifndef PLATFORM_64BITS
	#error "PLATFORM_64BITS must be defined"
#endif

// Base defines, these have defaults
#ifndef PLATFORM_LITTLE_ENDIAN
	#define PLATFORM_LITTLE_ENDIAN				0
#endif
#ifndef PLATFORM_SUPPORTS_UNALIGNED_LOADS
	#define PLATFORM_SUPPORTS_UNALIGNED_LOADS	0
#endif
#ifndef PLATFORM_EXCEPTIONS_DISABLED
	#define PLATFORM_EXCEPTIONS_DISABLED		!PLATFORM_DESKTOP
#endif
#ifndef PLATFORM_SEH_EXCEPTIONS_DISABLED
	#define PLATFORM_SEH_EXCEPTIONS_DISABLED	0
#endif
#ifndef PLATFORM_SUPPORTS_PRAGMA_PACK
	#define PLATFORM_SUPPORTS_PRAGMA_PACK		0
#endif

// Defines for the availibility of the various levels of vector intrinsics.
// These may be set from UnrealBuildTool, otherwise each platform-specific platform.h is expected to set them appropriately.
#ifndef PLATFORM_ENABLE_VECTORINTRINSICS
	#define PLATFORM_ENABLE_VECTORINTRINSICS	0
#endif
// If PLATFORM_MAYBE_HAS_### is 1, then ### intrinsics are compilable.
// This does not guarantee that the intrinsics are runnable on all instances of the platform however; a runtime check such as cpuid may be required to confirm availability.
// If PLATFORM_ALWAYS_HAS_### is 1, then ## intrinsics will compile and run on all instances of the platform.  PLATFORM_ALWAYS_HAS_### == 1 implies PLATFORM_MAYBE_HAS_### == 1.
#ifndef PLATFORM_MAYBE_HAS_SSE4_1
	#define PLATFORM_MAYBE_HAS_SSE4_1			0
#endif
#ifndef PLATFORM_ALWAYS_HAS_SSE4_1
	#define PLATFORM_ALWAYS_HAS_SSE4_1			0
#endif
#ifndef PLATFORM_MAYBE_HAS_AVX
	#define PLATFORM_MAYBE_HAS_AVX				0
#endif
#ifndef PLATFORM_ALWAYS_HAS_AVX
	#define PLATFORM_ALWAYS_HAS_AVX				0
#endif
#ifndef PLATFORM_ALWAYS_HAS_AVX_2
#define PLATFORM_ALWAYS_HAS_AVX_2				0
#endif
#ifndef PLATFORM_ALWAYS_HAS_FMA3
	#define PLATFORM_ALWAYS_HAS_FMA3			0
#endif


#ifndef PLATFORM_HAS_CPUID
	#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined (__amd64__)
		#define PLATFORM_HAS_CPUID				1
	#else
		#define PLATFORM_HAS_CPUID				0
	#endif
#endif
#ifndef PLATFORM_ENABLE_POPCNT_INTRINSIC
	// PC is disabled by default, but linux and mac are enabled
	// if your min spec is an AMD cpu mid-2007 or Intel 2008, you should enable this
	#define PLATFORM_ENABLE_POPCNT_INTRINSIC 0
#endif
#ifndef PLATFORM_ENABLE_VECTORINTRINSICS_NEON
	#define PLATFORM_ENABLE_VECTORINTRINSICS_NEON	0
#endif
#ifndef PLATFORM_USE_LS_SPEC_FOR_WIDECHAR
	#define PLATFORM_USE_LS_SPEC_FOR_WIDECHAR	1
#endif
#ifndef PLATFORM_USE_SYSTEM_VSWPRINTF
	#define PLATFORM_USE_SYSTEM_VSWPRINTF		1
#endif
#ifndef PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG
	#define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG			0
#endif
#ifdef _MSC_VER
	#define PLATFORM_COMPILER_HAS_AUTO_RETURN_TYPES 1
	#ifndef __clang__
		#pragma deprecated("PLATFORM_COMPILER_HAS_AUTO_RETURN_TYPES")
	#endif
#else
	#define PLATFORM_COMPILER_HAS_AUTO_RETURN_TYPES 1 DEPRECATED_MACRO(4.19, "PLATFORM_COMPILER_HAS_AUTO_RETURN_TYPES has been deprecated and should be replaced with 1.")
#endif
#ifndef PLATFORM_COMPILER_HAS_GENERIC_KEYWORD
	#define PLATFORM_COMPILER_HAS_GENERIC_KEYWORD	0
#endif
#ifdef _MSC_VER
	#define PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS 1
	#ifndef __clang__
		#pragma deprecated("PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS")
	#endif
#else
	#define PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS 1 DEPRECATED_MACRO(4.19, "PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS has been deprecated and should be replaced with 1.")
#endif
#ifndef PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION
	#define PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION 0
#endif
#ifndef PLATFORM_COMPILER_HAS_TCHAR_WMAIN
	#define PLATFORM_COMPILER_HAS_TCHAR_WMAIN 0
#endif
#ifndef PLATFORM_COMPILER_HAS_DECLTYPE_AUTO
	#define PLATFORM_COMPILER_HAS_DECLTYPE_AUTO 1
#endif
#ifndef PLATFORM_COMPILER_HAS_IF_CONSTEXPR
	#define PLATFORM_COMPILER_HAS_IF_CONSTEXPR 1
#endif
#ifndef PLATFORM_COMPILER_HAS_FOLD_EXPRESSIONS
	#define PLATFORM_COMPILER_HAS_FOLD_EXPRESSIONS 0
#endif
#ifndef PLATFORM_TCHAR_IS_4_BYTES
	#define PLATFORM_TCHAR_IS_4_BYTES			0
#endif
#ifndef PLATFORM_WCHAR_IS_4_BYTES
	#define PLATFORM_WCHAR_IS_4_BYTES			0
#endif
// The PLATFORM_TCHAR_IS_CHAR16 macro really represents PLATFORM_WIDECHAR_IS_CHAR16 - it should be deprecated
#ifndef PLATFORM_TCHAR_IS_CHAR16
	#define PLATFORM_TCHAR_IS_CHAR16			0
#endif
#define PLATFORM_WIDECHAR_IS_CHAR16 PLATFORM_TCHAR_IS_CHAR16
#ifndef PLATFORM_TCHAR_IS_UTF8CHAR
	#define PLATFORM_TCHAR_IS_UTF8CHAR			0
#endif
#ifndef PLATFORM_UCS2CHAR_IS_UTF16CHAR
	// Currently true, but we don't want it to be true
	#define PLATFORM_UCS2CHAR_IS_UTF16CHAR		1
#endif
#ifndef PLATFORM_HAS_BSD_TIME
	#define PLATFORM_HAS_BSD_TIME				1
#endif
#ifndef PLATFORM_HAS_BSD_THREAD_CPUTIME
	#define PLATFORM_HAS_BSD_THREAD_CPUTIME		0
#endif
#ifndef PLATFORM_HAS_BSD_SOCKETS
	#define PLATFORM_HAS_BSD_SOCKETS			1
#endif
#ifndef PLATFORM_HAS_BSD_IPV6_SOCKETS
	#define PLATFORM_HAS_BSD_IPV6_SOCKETS			0
#endif
#ifndef PLATFORM_SUPPORTS_UDP_MULTICAST_GROUP
	#define PLATFORM_SUPPORTS_UDP_MULTICAST_GROUP	1
#endif
#ifndef PLATFORM_USE_PTHREADS
	#define PLATFORM_USE_PTHREADS				1
#endif
#ifndef PLATFORM_MAX_FILEPATH_LENGTH_DEPRECATED
	#define PLATFORM_MAX_FILEPATH_LENGTH_DEPRECATED		128			// Deprecated - prefer FPlatformMisc::GetMaxPathLength() instead.
#endif
#ifndef PLATFORM_SUPPORTS_TEXTURE_STREAMING
	#define PLATFORM_SUPPORTS_TEXTURE_STREAMING	1
#endif
#ifndef PLATFORM_SUPPORTS_VIRTUAL_TEXTURE_STREAMING
	#define PLATFORM_SUPPORTS_VIRTUAL_TEXTURE_STREAMING	0
#endif
#ifndef PLATFORM_SUPPORTS_VIRTUAL_TEXTURES
	#define PLATFORM_SUPPORTS_VIRTUAL_TEXTURES		0
#endif
#ifndef PLATFORM_SUPPORTS_VARIABLE_RATE_SHADING
	#define PLATFORM_SUPPORTS_VARIABLE_RATE_SHADING		0
#endif
#ifndef PLATFORM_REQUIRES_FILESERVER
	#define PLATFORM_REQUIRES_FILESERVER		0
#endif
#ifndef PLATFORM_SUPPORTS_MULTITHREADED_GC
	#define PLATFORM_SUPPORTS_MULTITHREADED_GC	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_SELECT
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_SELECT	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS	0
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_GETADDRINFO
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_GETADDRINFO	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_GETNAMEINFO
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_GETNAMEINFO 1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_CLOSE_ON_EXEC
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_CLOSE_ON_EXEC	0
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_MSG_DONTWAIT
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_MSG_DONTWAIT	0
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_RECVMMSG
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_RECVMMSG	0
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_TIMESTAMP
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_TIMESTAMP 0
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_NODELAY
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_NODELAY	1
#endif
#ifndef PLATFORM_HAS_NO_EPROCLIM
	#define PLATFORM_HAS_NO_EPROCLIM			0
#endif
#ifndef PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS
	#define PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS 0
#endif

#ifndef PLATFORM_SUPPORTS_DRAW_MESH_EVENTS
	#define PLATFORM_SUPPORTS_DRAW_MESH_EVENTS	1
#endif

#ifndef PLATFORM_USES_GLES
	#define PLATFORM_USES_GLES					0
#endif

#ifndef PLATFORM_SUPPORTS_GEOMETRY_SHADERS
	#define PLATFORM_SUPPORTS_GEOMETRY_SHADERS		1
#endif

#ifndef PLATFORM_SUPPORTS_MESH_SHADERS
	#define PLATFORM_SUPPORTS_MESH_SHADERS 0
#endif

#ifndef PLATFORM_BUILTIN_VERTEX_HALF_FLOAT
	#define PLATFORM_BUILTIN_VERTEX_HALF_FLOAT	1
#endif

#ifndef PLATFORM_SUPPORTS_TBB
	#define PLATFORM_SUPPORTS_TBB 0
#endif

#ifndef PLATFORM_SUPPORTS_BORDERLESS_WINDOW
	#define PLATFORM_SUPPORTS_BORDERLESS_WINDOW		0
#endif

#ifndef PLATFORM_MAX_CACHED_SYNC_FILE_HANDLES_PER_GENERIC_ASYNC_FILE_HANDLE
	#define PLATFORM_MAX_CACHED_SYNC_FILE_HANDLES_PER_GENERIC_ASYNC_FILE_HANDLE 4
#endif

#ifndef PLATFORM_FORCE_SINGLE_SYNC_FILE_HANDLE_PER_GENERIC_ASYNC_FILE_HANDLE
	#define PLATFORM_FORCE_SINGLE_SYNC_FILE_HANDLE_PER_GENERIC_ASYNC_FILE_HANDLE 0
#endif

#ifndef PLATFORM_SUPPORTS_JEMALLOC
	#define PLATFORM_SUPPORTS_JEMALLOC 0
#endif

#ifndef PLATFORM_SUPPORTS_MIMALLOC
	#define PLATFORM_SUPPORTS_MIMALLOC 0
#endif

#ifndef PLATFORM_CAN_SUPPORT_EDITORONLY_DATA
	#define PLATFORM_CAN_SUPPORT_EDITORONLY_DATA 0
#endif

#ifndef PLATFORM_SUPPORTS_NAMED_PIPES
	#define PLATFORM_SUPPORTS_NAMED_PIPES		0
#endif

#ifndef PLATFORM_USES_FIXED_RHI_CLASS
	#define PLATFORM_USES_FIXED_RHI_CLASS		0
#endif

#ifndef PLATFORM_USES_FIXED_GMalloc_CLASS
	#define PLATFORM_USES_FIXED_GMalloc_CLASS		0
#endif

#ifndef PLATFORM_USES_STACKBASED_MALLOC_CRASH
	#define PLATFORM_USES_STACKBASED_MALLOC_CRASH	0
#endif

#ifndef PLATFORM_SUPPORTS_MULTIPLE_NATIVE_WINDOWS
	#define PLATFORM_SUPPORTS_MULTIPLE_NATIVE_WINDOWS	1
#endif

#ifndef PLATFORM_HAS_TOUCH_MAIN_SCREEN
	#define PLATFORM_HAS_TOUCH_MAIN_SCREEN		0
#endif

#ifndef PLATFORM_SUPPORTS_STACK_SYMBOLS
	#define PLATFORM_SUPPORTS_STACK_SYMBOLS 0
#endif

#ifndef PLATFORM_RUNTIME_MALLOCPROFILER_SYMBOLICATION
	#define PLATFORM_RUNTIME_MALLOCPROFILER_SYMBOLICATION PLATFORM_SUPPORTS_STACK_SYMBOLS
#endif

#ifndef PLATFORM_HAS_128BIT_ATOMICS
	#define PLATFORM_HAS_128BIT_ATOMICS 0
#endif

#ifndef PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING
	#define PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING	1
#endif

#ifndef PLATFORM_IMPLEMENTS_BeginNamedEventStatic
	#define PLATFORM_IMPLEMENTS_BeginNamedEventStatic			0
#endif

#ifndef PLATFORM_RHITHREAD_DEFAULT_BYPASS
	#define PLATFORM_RHITHREAD_DEFAULT_BYPASS					1
#endif

#ifndef PLATFORM_HAS_UMA
	#define PLATFORM_HAS_UMA 0
#endif

#ifndef PLATFORM_NUM_AUDIODECOMPRESSION_PRECACHE_BUFFERS
	#define PLATFORM_NUM_AUDIODECOMPRESSION_PRECACHE_BUFFERS 2
#endif

#ifndef PLATFORM_SUPPORTS_EARLY_MOVIE_PLAYBACK
	#define PLATFORM_SUPPORTS_EARLY_MOVIE_PLAYBACK			0
#endif

#ifndef PLATFORM_UI_HAS_MOBILE_SCROLLBARS
	#define PLATFORM_UI_HAS_MOBILE_SCROLLBARS					0
#endif

#ifndef PLATFORM_UI_NEEDS_TOOLTIPS
	#define PLATFORM_UI_NEEDS_TOOLTIPS							1
#endif

#ifndef PLATFORM_UI_NEEDS_FOCUS_OUTLINES
	#define PLATFORM_UI_NEEDS_FOCUS_OUTLINES					1
#endif

#ifndef PLATFORM_LIMIT_MOBILE_BONE_MATRICES
	#define PLATFORM_LIMIT_MOBILE_BONE_MATRICES					0
#endif

#ifndef PLATFORM_WEAKLY_CONSISTENT_MEMORY
	#define PLATFORM_WEAKLY_CONSISTENT_MEMORY PLATFORM_CPU_ARM_FAMILY
#endif

#ifndef PLATFORM_HAS_CRC_INTRINSICS
	#define PLATFORM_HAS_CRC_INTRINSICS							0
#endif

#ifndef PLATFORM_NEEDS_RHIRESOURCELIST
	#define PLATFORM_NEEDS_RHIRESOURCELIST 1
#endif

#ifndef PLATFORM_USE_FULL_TASK_GRAPH
	#define PLATFORM_USE_FULL_TASK_GRAPH						1
#endif

#ifndef PLATFORM_USES_ANSI_MALLOC
	#define PLATFORM_USES_ANSI_MALLOC							1
#endif

#ifndef PLATFORM_USE_ANSI_POSIX_MALLOC
	#define PLATFORM_USE_ANSI_POSIX_MALLOC						0
#endif

#ifndef PLATFORM_USES__ALIGNED_MALLOC
	#define PLATFORM_USES__ALIGNED_MALLOC						0
#endif

#ifndef PLATFORM_USE_ANSI_MEMALIGN
	#define PLATFORM_USE_ANSI_MEMALIGN							0
#endif

#ifndef PLATFORM_USE_ANSI_POSIX_MALLOC
	#define PLATFORM_USE_ANSI_POSIX_MALLOC						0
#endif

#ifndef PLATFORM_IS_ANSI_MALLOC_THREADSAFE
	#define PLATFORM_IS_ANSI_MALLOC_THREADSAFE					0
#endif

#ifndef PLATFORM_SUPPORTS_OPUS_CODEC
	#define PLATFORM_SUPPORTS_OPUS_CODEC						1
#endif

#ifndef PLATFORM_SUPPORTS_VORBIS_CODEC
	#define PLATFORM_SUPPORTS_VORBIS_CODEC						1
#endif


#ifndef PLATFORM_USE_MINIMAL_HANG_DETECTION
	#define PLATFORM_USE_MINIMAL_HANG_DETECTION					0
#endif

#ifndef PLATFORM_PRESENT_HANG_DETECTION_ON_BY_DEFAULT
	#define PLATFORM_PRESENT_HANG_DETECTION_ON_BY_DEFAULT		0
#endif

#ifndef PLATFORM_USE_GENERIC_STRING_IMPLEMENTATION
	#define PLATFORM_USE_GENERIC_STRING_IMPLEMENTATION			1
#endif

#ifndef PLATFORM_SUPPORTS_LLM
	#define PLATFORM_SUPPORTS_LLM								1
#endif

#ifndef PLATFORM_ALLOW_ALLOCATIONS_IN_FASYNCWRITER_SERIALIZEBUFFERTOARCHIVE
	#define	PLATFORM_ALLOW_ALLOCATIONS_IN_FASYNCWRITER_SERIALIZEBUFFERTOARCHIVE 1
#endif

#ifndef PLATFORM_HAS_FPlatformVirtualMemoryBlock
	#define	PLATFORM_HAS_FPlatformVirtualMemoryBlock 1
#endif
#ifndef PLATFORM_BYPASS_PAK_PRECACHE
	#define PLATFORM_BYPASS_PAK_PRECACHE 0
#endif

#ifndef PLATFORM_SUPPORTS_FLIP_TRACKING
	#define PLATFORM_SUPPORTS_FLIP_TRACKING 0
#endif

#ifndef PLATFORM_USE_FULL_TASK_GRAPH
	#define PLATFORM_USE_FULL_TASK_GRAPH						1
#endif

#ifndef PLATFORM_USE_ANSI_POSIX_MALLOC
	#define PLATFORM_USE_ANSI_POSIX_MALLOC						0
#endif

#ifndef PLATFORM_USE_ANSI_MEMALIGN
	#define PLATFORM_USE_ANSI_MEMALIGN							0
#endif

#ifndef PLATFORM_USE_ANSI_POSIX_MALLOC
	#define PLATFORM_USE_ANSI_POSIX_MALLOC						0
#endif

#ifndef PLATFORM_IS_ANSI_MALLOC_THREADSAFE
	#define PLATFORM_IS_ANSI_MALLOC_THREADSAFE					0
#endif

#ifndef PLATFORM_SUPPORTS_OPUS_CODEC
	#define PLATFORM_SUPPORTS_OPUS_CODEC						1
#endif

#ifndef PLATFORM_SUPPORTS_VORBIS_CODEC
	#define PLATFORM_SUPPORTS_VORBIS_CODEC						1
#endif

#ifndef PLATFORM_USE_MINIMAL_HANG_DETECTION
	#define PLATFORM_USE_MINIMAL_HANG_DETECTION					0
#endif

#ifndef PLATFORM_USE_GENERIC_STRING_IMPLEMENTATION
	#define PLATFORM_USE_GENERIC_STRING_IMPLEMENTATION			1
#endif

#ifndef PLATFORM_USE_SHOWFLAGS_ALWAYS_BITFIELD
	#define	PLATFORM_USE_SHOWFLAGS_ALWAYS_BITFIELD				1
#endif

#ifndef PLATFORM_SUPPORTS_LLM
	#define PLATFORM_SUPPORTS_LLM								1
#endif

#ifndef PLATFORM_ALLOW_ALLOCATIONS_IN_FASYNCWRITER_SERIALIZEBUFFERTOARCHIVE
	#define	PLATFORM_ALLOW_ALLOCATIONS_IN_FASYNCWRITER_SERIALIZEBUFFERTOARCHIVE 1
#endif

#ifndef PLATFORM_HAS_FPlatformVirtualMemoryBlock
	#define	PLATFORM_HAS_FPlatformVirtualMemoryBlock 1
#endif

#ifndef PLATFORM_SUPPORTS_LANDSCAPE_VISUAL_MESH_LOD_STREAMING
	#define PLATFORM_SUPPORTS_LANDSCAPE_VISUAL_MESH_LOD_STREAMING 0
#endif

#ifndef PLATFORM_USE_GENERIC_LAUNCH_IMPLEMENTATION
	#define PLATFORM_USE_GENERIC_LAUNCH_IMPLEMENTATION			0
#endif

#ifndef PLATFORM_USES_FIXED_HDR_SETTING
	#define PLATFORM_USES_FIXED_HDR_SETTING 0
#endif

#ifndef PLATFORM_MANAGES_HDR_SETTING
	#define PLATFORM_MANAGES_HDR_SETTING 0
#endif

#ifndef PLATFORM_SUPPORTS_COLORIZED_OUTPUT_DEVICE
	#define PLATFORM_SUPPORTS_COLORIZED_OUTPUT_DEVICE PLATFORM_DESKTOP
#endif

#ifndef PLATFORM_USE_BACKBUFFER_WRITE_TRANSITION_TRACKING
	#define PLATFORM_USE_BACKBUFFER_WRITE_TRANSITION_TRACKING 0
#endif

#ifndef PLATFORM_USE_SEPARATE_BACKBUFFER_WRITE_TRANSITION
	#define PLATFORM_USE_SEPARATE_BACKBUFFER_WRITE_TRANSITION 0
#endif

#ifndef PLATFORM_USE_PLATFORM_FILE_MANAGED_STORAGE_WRAPPER
	#define PLATFORM_USE_PLATFORM_FILE_MANAGED_STORAGE_WRAPPER	0
#endif

#ifndef PLATFORM_REQUIRES_UAV_TO_RTV_TEXTURE_CACHE_FLUSH_WORKAROUND
	#define PLATFORM_REQUIRES_UAV_TO_RTV_TEXTURE_CACHE_FLUSH_WORKAROUND 0
#endif // #ifndef PLATFORM_REQUIRES_UAV_TO_RTV_TEXTURE_CACHE_FLUSH_WORKAROUND

#ifndef PLATFORM_HAS_DIRECT_TEXTURE_MEMORY_ACCESS
	#define PLATFORM_HAS_DIRECT_TEXTURE_MEMORY_ACCESS 0
#endif

#ifndef PLATFORM_DIRECT_TEXTURE_MEMORY_ACCESS_LOCK_MODE 
	#define PLATFORM_DIRECT_TEXTURE_MEMORY_ACCESS_LOCK_MODE RLM_ReadOnly 
#endif

#ifndef PLATFORM_NEEDS_GPU_UAV_RESOURCE_INIT_WORKAROUND
	#define PLATFORM_NEEDS_GPU_UAV_RESOURCE_INIT_WORKAROUND 0
#endif

#ifndef PLATFORM_USE_REPORT_ENSURE
	#define PLATFORM_USE_REPORT_ENSURE PLATFORM_DESKTOP
#endif

#ifndef PLATFORM_USE_FALLBACK_PSO
	#define PLATFORM_USE_FALLBACK_PSO 0
#endif

#ifndef PLATFORM_USES_UNFAIR_LOCKS
	#define PLATFORM_USES_UNFAIR_LOCKS 0
#endif

#ifndef PLATFORM_SUPPORTS_GPU_FRAMETIME_WITHOUT_MGPU
	#define PLATFORM_SUPPORTS_GPU_FRAMETIME_WITHOUT_MGPU 0
#endif

#ifndef PLATFORM_REQUIRES_TYPELESS_RESOURCE_DISCARD_WORKAROUND
	#define PLATFORM_REQUIRES_TYPELESS_RESOURCE_DISCARD_WORKAROUND 0
#endif

// deprecated, do not use
#define PLATFORM_HAS_THREADSAFE_RHIGetRenderQueryResult	#
#define PLATFORM_SUPPORTS_RHI_THREAD #
#define PLATFORM_RHI_USES_CONTEXT_OBJECT # // deprecated, do not use; all platforms must use a context object
#define PLATFORM_SUPPORTS_PARALLEL_RHI_EXECUTE # // deprecated, do not use; see GRHISupportsParallelRHIExecute


//These are deprecated old defines that we want to make sure are not used
#define CONSOLE (#)
#define MOBILE (#)
#define PLATFORM_CONSOLE (#)

// These is computed, not predefined
#define PLATFORM_32BITS					(!PLATFORM_64BITS)

// not supported by the platform system yet or maybe ever
#define PLATFORM_VTABLE_AT_END_OF_CLASS 0

#ifndef VARARGS
	#define VARARGS									/* Functions with variable arguments */
#endif
#ifndef CDECL
	#define CDECL									/* Standard C function */
#endif
#ifndef STDCALL
	#define STDCALL									/* Standard calling convention */
#endif
#ifndef FORCEINLINE
	#define FORCEINLINE 							/* Force code to be inline */
#endif
#ifndef FORCENOINLINE
	#define FORCENOINLINE 							/* Force code to NOT be inline */
#endif
#ifndef RESTRICT
	#define RESTRICT __restrict						/* no alias hint */
#endif

/* Wrap a function signature in these to warn that callers should not ignore the return value */
#ifndef FUNCTION_CHECK_RETURN_START
	#define FUNCTION_CHECK_RETURN_START \
		DEPRECATED_MACRO(4.26, "FUNCTION_CHECK_RETURN_START has been deprecated - please use UE_NODISCARD")
#endif
#ifndef FUNCTION_CHECK_RETURN_END
	#define FUNCTION_CHECK_RETURN_END \
		DEPRECATED_MACRO(4.26, "FUNCTION_CHECK_RETURN_END has been deprecated - please use UE_NODISCARD")
#endif

/* Wrap a function signature in these to indicate that the function never returns */
#ifndef FUNCTION_NO_RETURN_START
	#define FUNCTION_NO_RETURN_START \
		DEPRECATED_MACRO(4.26, "FUNCTION_NO_RETURN_START has been deprecated - please use UE_NORETURN")
#endif
#ifndef FUNCTION_NO_RETURN_END
	#define FUNCTION_NO_RETURN_END \
		DEPRECATED_MACRO(4.26, "FUNCTION_NO_RETURN_END has been deprecated - please use UE_NORETURN")
#endif

/* Use before a function declaration to warn that callers should not ignore the return value */
#if !defined(UE_NODISCARD) && defined(__has_cpp_attribute)
	#if __has_cpp_attribute(nodiscard)
		#define UE_NODISCARD [[nodiscard]]
	#endif
#endif
#ifndef UE_NODISCARD
	#define UE_NODISCARD
#endif

/* Use before a function declaration to indicate that the function never returns */
#if !defined(UE_NORETURN) && defined(__has_cpp_attribute)
	#if __has_cpp_attribute(noreturn)
		#define UE_NORETURN [[noreturn]]
	#endif
#endif
#ifndef UE_NORETURN
	#define UE_NORETURN
#endif

/* Macro wrapper for the consteval keyword which isn't yet present on all compilers - constexpr
   can be used as a workaround but is less strict and so may let some non-consteval code pass */
#if defined(__cpp_consteval)
	#define UE_CONSTEVAL consteval
#else
	#define UE_CONSTEVAL constexpr
#endif

/* Wrap a function signature in these to indicate that the function never returns nullptr */
#ifndef FUNCTION_NON_NULL_RETURN_START
	#define FUNCTION_NON_NULL_RETURN_START
#endif
#ifndef FUNCTION_NON_NULL_RETURN_END
	#define FUNCTION_NON_NULL_RETURN_END
#endif

#ifndef FUNCTION_CHECK_RETURN
	#define FUNCTION_CHECK_RETURN(...) DEPRECATED_MACRO(4.12, "FUNCTION_CHECK_RETURN has been deprecated and should be replaced with FUNCTION_CHECK_RETURN_START and FUNCTION_CHECK_RETURN_END.") FUNCTION_CHECK_RETURN_START __VA_ARGS__ FUNCTION_CHECK_RETURN_END
#endif

/** Promise expression is true. Compiler can optimize accordingly with undefined behavior if wrong. Static analyzers understand this.  */
#ifndef UE_ASSUME
	#if defined(__clang__)
		#define UE_ASSUME(x) __builtin_assume(x)
	#elif defined(_MSC_VER)
		#define UE_ASSUME(x) __assume(x)
	#else
		#define UE_ASSUME(x)
	#endif
#endif

#define ASSUME(x) UE_ASSUME(x) DEPRECATED_MACRO(4.25, "Please use UE_ASSUME instead.")

/** Branch prediction hints */
#ifndef LIKELY						/* Hints compiler that expression is likely to be true, much softer than UE_ASSUME - allows (penalized by worse performance) expression to be false */
	#if ( defined(__clang__) || defined(__GNUC__) ) && (PLATFORM_UNIX)	// effect of these on non-Linux platform has not been analyzed as of 2016-03-21
		#define LIKELY(x)			__builtin_expect(!!(x), 1)
	#else
		// the additional "!!" is added to silence "warning: equality comparison with exteraneous parenthese" messages on android
		#define LIKELY(x)			(!!(x))
	#endif
#endif

#ifndef UNLIKELY					/* Hints compiler that expression is unlikely to be true, allows (penalized by worse performance) expression to be true */
	#if ( defined(__clang__) || defined(__GNUC__) ) && (PLATFORM_UNIX)	// effect of these on non-Linux platform has not been analyzed as of 2016-03-21
		#define UNLIKELY(x)			__builtin_expect(!!(x), 0)
	#else
		// the additional "!!" is added to silence "warning: equality comparison with exteraneous parenthese" messages on android
		#define UNLIKELY(x)			(!!(x))
	#endif
#endif

// Optimization macros (uses __pragma to enable inside a #define).
#ifndef PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
	#define PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
	#define PRAGMA_ENABLE_OPTIMIZATION_ACTUAL
#endif

// Disable optimization of a specific function
#ifndef DISABLE_FUNCTION_OPTIMIZATION
	#define DISABLE_FUNCTION_OPTIMIZATION
#endif

// Enable/disable optimizations for a specific function to improve build times
#define BEGIN_FUNCTION_BUILD_OPTIMIZATION PRAGMA_DISABLE_OPTIMIZATION
#define END_FUNCTION_BUILD_OPTIMIZATION   PRAGMA_ENABLE_OPTIMIZATION

#ifndef FORCEINLINE_DEBUGGABLE_ACTUAL
	#define FORCEINLINE_DEBUGGABLE_ACTUAL inline
#endif

#ifndef DECLARE_UINT64
	#define DECLARE_UINT64(x) x##ULL	/* Define a 64 bit immediate int **/
#endif

// Backwater of the spec. All compilers support this except microsoft, and they will soon
#ifndef TYPENAME_OUTSIDE_TEMPLATE
	#define TYPENAME_OUTSIDE_TEMPLATE	typename
#endif

// Method modifiers
#ifndef ABSTRACT
	#define ABSTRACT
#endif
#ifndef CONSTEXPR
	#define CONSTEXPR constexpr
#endif

// String constants
#ifndef LINE_TERMINATOR
	#define LINE_TERMINATOR TEXT("\n")
#endif
#ifndef LINE_TERMINATOR_ANSI
	#define LINE_TERMINATOR_ANSI "\n"
#endif

// Alignment.
#ifndef GCC_PACK
	#define GCC_PACK(n)
#endif
#ifndef GCC_ALIGN
	#define GCC_ALIGN(n)
#endif
#ifndef MS_ALIGN
	#define MS_ALIGN(n)
#endif

// MSVC pragmas - used so other platforms can remove them easily (by not defining this)
#ifndef MSVC_PRAGMA
	#define MSVC_PRAGMA(...)
#endif


// Inlining
#ifndef PRAGMA_DISABLE_INLINING
	#define PRAGMA_DISABLE_INLINING
	#define PRAGMA_ENABLE_INLINING
#endif

// Cache control
#ifndef FLUSH_CACHE_LINE
	#define FLUSH_CACHE_LINE(x)
#endif

// Prefetch
#ifndef PLATFORM_CACHE_LINE_SIZE
	#define PLATFORM_CACHE_LINE_SIZE	64
#endif

// Compile-time warnings and errors. Use these as "#pragma COMPILER_WARNING("XYZ")". GCC does not expand macro parameters to _Pragma, so we can't wrap the #pragma part.
#ifdef _MSC_VER
	#define MSC_FORMAT_DIAGNOSTIC_HELPER_2(x) #x
	#define MSC_FORMAT_DIAGNOSTIC_HELPER(x) MSC_FORMAT_DIAGNOSTIC_HELPER_2(x)
	#define COMPILE_WARNING(x) __pragma(message(__FILE__ "(" MSC_FORMAT_DIAGNOSTIC_HELPER(__LINE__) "): warning: " x))
	#define COMPILE_ERROR(x) __pragma(message(__FILE__ "(" MSC_FORMAT_DIAGNOSTIC_HELPER(__LINE__) "): error: " x))
#else
	#define GCC_DIAGNOSTIC_HELPER(x) _Pragma(#x)
	#define COMPILE_WARNING(x) GCC_DIAGNOSTIC_HELPER(GCC warning x)
	#define COMPILE_ERROR(x) GCC_DIAGNOSTIC_HELPER(GCC error x)
#endif

// Tells the compiler to put the decorated function in a certain section (aka. segment) of the executable.
#ifndef PLATFORM_CODE_SECTION
	#define PLATFORM_CODE_SECTION(Name)
#endif

// These have to be forced inline on some OSes so the dynamic loader will not
// resolve to our allocators for the system libraries.
#ifndef OPERATOR_NEW_INLINE
	#define OPERATOR_NEW_INLINE FORCEINLINE
#endif

#ifndef OPERATOR_NEW_THROW_SPEC
	#define OPERATOR_NEW_THROW_SPEC
#endif
#ifndef OPERATOR_DELETE_THROW_SPEC
	#define OPERATOR_DELETE_THROW_SPEC
#endif
#ifndef OPERATOR_NEW_NOTHROW_SPEC
	#define OPERATOR_NEW_NOTHROW_SPEC throw()
#endif
#ifndef OPERATOR_DELETE_NOTHROW_SPEC
	#define OPERATOR_DELETE_NOTHROW_SPEC throw()
#endif

#ifndef checkAtCompileTime
	#define checkAtCompileTime(expr, msg) \
		EMIT_DEPRECATED_WARNING_MESSAGE("checkAtCompileTime is deprecated. Please use static_assert instead.") \
		static_assert(expr, #msg)
#endif

// DLL export and import definitions
#ifndef DLLEXPORT
	#define DLLEXPORT
	#define DLLIMPORT
#endif

// embedded app is not default (embedding UE4 in a native view, right now just for IOS and Android)
#ifndef BUILD_EMBEDDED_APP
	#define BUILD_EMBEDDED_APP  0
#endif
#ifndef FAST_BOOT_HACKS
	#define FAST_BOOT_HACKS  0
#endif

#ifndef DEPRECATED_FORGAME
	#define DEPRECATED_FORGAME(...) DEPRECATED_MACRO(4.22, "The DEPRECATED_FORGAME macro has been deprecated in favor of UE_DEPRECATED_FORGAME().")
#endif

// Console ANSICHAR/TCHAR command line handling
#if PLATFORM_COMPILER_HAS_TCHAR_WMAIN
#define INT32_MAIN_INT32_ARGC_TCHAR_ARGV() int32 wmain(int32 ArgC, TCHAR* ArgV[])
#else
#define INT32_MAIN_INT32_ARGC_TCHAR_ARGV() \
int32 tchar_main(int32 ArgC, TCHAR* ArgV[]); \
int32 main(int32 ArgC, ANSICHAR* Utf8ArgV[]) \
{ \
	TCHAR** ArgV = new TCHAR*[ArgC]; \
	for (int32 a = 0; a < ArgC; a++) \
	{ \
		FUTF8ToTCHAR ConvertFromUtf8(Utf8ArgV[a]); \
		ArgV[a] = new TCHAR[ConvertFromUtf8.Length() + 1]; \
		FCString::Strcpy(ArgV[a], ConvertFromUtf8.Length() + 1, ConvertFromUtf8.Get()); \
	} \
	int32 Result = tchar_main(ArgC, ArgV); \
	for (int32 a = 0; a < ArgC; a++) \
	{ \
		delete[] ArgV[a]; \
	} \
	delete[] ArgV; \
	return Result; \
} \
int32 tchar_main(int32 ArgC, TCHAR* ArgV[])
#endif

template<typename,typename> struct TAreTypesEqual;

//--------------------------------------------------------------------------------------------
// POD types refactor for porting old code:
//
// Replace 'FLOAT' with 'float'
// Replace 'DOUBLE' with 'double'
// Replace 'INT' with 'int32' (make sure not to replace INT text in localization strings!)
// Replace 'UINT' with 'uint32'
// Replace 'DWORD' with 'uint32' (DWORD is still used in Windows platform source files)
// Replace 'WORD' with 'uint16'
// Replace 'USHORT' with 'uint16'
// Replace 'SWORD' with 'int16'
// Replace 'QWORD' with 'uint64'
// Replace 'SQWORD' with 'int64'
// Replace 'BYTE' with 'uint8'
// Replace 'SBYTE' with 'int8'
// Replace 'BITFIELD' with 'uint32'
// Replace 'UBOOL' with 'bool'
// Replace 'FALSE' with 'false' and 'TRUE' with true.
// Make sure any platform API uses its own bool types.
// For example WinAPI uses BOOL, FALSE and TRUE otherwise sometimes it doesn't work properly
// or it doesn't compile.
// Replace 'ExtractUBOOLFromBitfield' with 'ExtractBoolFromBitfield'
// Replace 'SetUBOOLInBitField' with 'SetBoolInBitField'
// Replace 'ParseUBOOL' with 'FParse::Bool'
// Replace 'MINBYTE', with 'MIN_uint8'
// Replace 'MINWORD', with 'MIN_uint16'
// Replace 'MINDWORD', with 'MIN_uint32'
// Replace 'MINSBYTE', with 'MIN_int8'
// Replace 'MINSWORD', with 'MIN_int16'
// Replace 'MAXINT', with 'MAX_int32'
// Replace 'MAXBYTE', with 'MAX_uint8'
// Replace 'MAXWORD', with 'MAX_uint16'
// Replace 'MAXDWORD', with 'MAX_uint32'
// Replace 'MAXSBYTE', with 'MAX_int8'
// Replace 'MAXSWORD', with 'MAX_int16'
// Replace 'MAXINT', with 'MAX_int32'
//--------------------------------------------------------------------------------------------

//------------------------------------------------------------------
// Transfer the platform types to global types
//------------------------------------------------------------------

//~ Unsigned base types.
/// An 8-bit unsigned integer.
typedef FPlatformTypes::uint8		uint8;
/// A 16-bit unsigned integer.
typedef FPlatformTypes::uint16		uint16;
/// A 32-bit unsigned integer.
typedef FPlatformTypes::uint32		uint32;
/// A 64-bit unsigned integer.
typedef FPlatformTypes::uint64		uint64;

//~ Signed base types.
/// An 8-bit signed integer.
typedef	FPlatformTypes::int8		int8;
/// A 16-bit signed integer.
typedef FPlatformTypes::int16		int16;
/// A 32-bit signed integer.
typedef FPlatformTypes::int32		int32;
/// A 64-bit signed integer.
typedef FPlatformTypes::int64		int64;

//~ Character types.
/// An ANSI character. Normally a signed type.
typedef FPlatformTypes::ANSICHAR	ANSICHAR;
/// A wide character. Normally a signed type.
typedef FPlatformTypes::WIDECHAR	WIDECHAR;
/// Either ANSICHAR or WIDECHAR, depending on whether the platform supports wide characters or the requirements of the licensee.
typedef FPlatformTypes::TCHAR		TCHAR;
/// An 8-bit character containing a UTF8 (Unicode, 8-bit, variable-width) code unit.
typedef FPlatformTypes::UTF8CHAR	UTF8CHAR;
/// A 16-bit character containing a UCS2 (Unicode, 16-bit, fixed-width) code unit, used for compatibility with 'Windows TCHAR' across multiple platforms.
typedef FPlatformTypes::CHAR16		UCS2CHAR;
/// A 16-bit character containing a UTF16 (Unicode, 16-bit, variable-width) code unit.
typedef FPlatformTypes::CHAR16		UTF16CHAR;
/// A 32-bit character containing a UTF32 (Unicode, 32-bit, fixed-width) code unit.
typedef FPlatformTypes::CHAR32		UTF32CHAR;

/// An unsigned integer the same size as a pointer
typedef FPlatformTypes::UPTRINT UPTRINT;
/// A signed integer the same size as a pointer
typedef FPlatformTypes::PTRINT PTRINT;
/// An unsigned integer the same size as a pointer, the same as UPTRINT
typedef FPlatformTypes::SIZE_T SIZE_T;
/// An integer the same size as a pointer, the same as PTRINT
typedef FPlatformTypes::SSIZE_T SSIZE_T;

/// The type of the NULL constant.
typedef FPlatformTypes::TYPE_OF_NULL	TYPE_OF_NULL;
/// The type of the C++ nullptr keyword.
typedef FPlatformTypes::TYPE_OF_NULLPTR	TYPE_OF_NULLPTR;

//------------------------------------------------------------------
// Test the global types
//------------------------------------------------------------------
namespace TypeTests
{
	template <typename A, typename B>
	struct TAreTypesEqual
	{
		static constexpr bool Value = false;
	};

	template <typename T>
	struct TAreTypesEqual<T, T>
	{
		static constexpr bool Value = true;
	};

#if PLATFORM_TCHAR_IS_4_BYTES
	static_assert(sizeof(TCHAR) == 4, "TCHAR size must be 4 bytes.");
#elif PLATFORM_TCHAR_IS_UTF8CHAR
	static_assert(sizeof(TCHAR) == 1, "TCHAR size must be 1 byte.");
#else
	static_assert(sizeof(TCHAR) == 2, "TCHAR size must be 2 bytes.");
#endif

	static_assert(!PLATFORM_WCHAR_IS_4_BYTES || sizeof(wchar_t) == 4, "wchar_t size must be 4 bytes.");
	static_assert(PLATFORM_WCHAR_IS_4_BYTES || sizeof(wchar_t) == 2, "wchar_t size must be 2 bytes.");

	static_assert(PLATFORM_32BITS || PLATFORM_64BITS, "Type tests pointer size failed.");
	static_assert(PLATFORM_32BITS != PLATFORM_64BITS, "Type tests pointer exclusive failed.");
	static_assert(!PLATFORM_64BITS || sizeof(void*) == 8, "Pointer size is 64bit, but pointers are short.");
	static_assert(PLATFORM_64BITS || sizeof(void*) == 4, "Pointer size is 32bit, but pointers are long.");

	static_assert(char(-1) < char(0), "Unsigned char type test failed.");

	static_assert((!TAreTypesEqual<ANSICHAR, WIDECHAR>::Value),  "ANSICHAR and WIDECHAR should be different types.");
	static_assert((!TAreTypesEqual<ANSICHAR, UTF8CHAR>::Value),  "ANSICHAR and UTF8CHAR should be different types.");
#if !PLATFORM_UCS2CHAR_IS_UTF16CHAR
	// We want these types to be different, because we want to be able to determine whether an encoding
	// is fixed-width (UCS2CHAR) or variable-width (UTF16CHAR) at compile time.
	static_assert((!TAreTypesEqual<UCS2CHAR, UTF16CHAR>::Value), "UCS2CHAR and UTF16CHAR should be different types.");
#else
	// We don't want these types to be equal, but while this macro exists, they ought to be equal
	static_assert(TAreTypesEqual<UCS2CHAR, UTF16CHAR>::Value, "UCS2CHAR and UTF16CHAR are expected to be the same type.");
#endif
	static_assert((!TAreTypesEqual<ANSICHAR, UCS2CHAR>::Value),  "ANSICHAR and UCS2CHAR should be different types.");
	static_assert((!TAreTypesEqual<WIDECHAR, UCS2CHAR>::Value),  "WIDECHAR and UCS2CHAR should be different types.");
	static_assert(TAreTypesEqual<TCHAR, ANSICHAR>::Value || TAreTypesEqual<TCHAR, WIDECHAR>::Value || TAreTypesEqual<TCHAR, UTF8CHAR>::Value, "TCHAR should either be ANSICHAR, WIDECHAR or UTF8CHAR.");

#if PLATFORM_WIDECHAR_IS_CHAR16
	static_assert(TAreTypesEqual<WIDECHAR, char16_t>::Value, "WIDECHAR should be char16_t");
#else
	static_assert(TAreTypesEqual<WIDECHAR, wchar_t>::Value, "WIDECHAR should be wchar_t");
#endif

	static_assert(sizeof(uint8) == 1, "uint8 type size test failed.");
	static_assert(int32(uint8(-1)) == 0xFF, "uint8 type sign test failed.");

	static_assert(sizeof(uint16) == 2, "uint16 type size test failed.");
	static_assert(int32(uint16(-1)) == 0xFFFF, "uint16 type sign test failed.");

	static_assert(sizeof(uint32) == 4, "uint32 type size test failed.");
	static_assert(int64(uint32(-1)) == int64(0xFFFFFFFF), "uint32 type sign test failed.");

	static_assert(sizeof(uint64) == 8, "uint64 type size test failed.");
	static_assert(uint64(-1) > uint64(0), "uint64 type sign test failed.");


	static_assert(sizeof(int8) == 1, "int8 type size test failed.");
	static_assert(int32(int8(-1)) == -1, "int8 type sign test failed.");

	static_assert(sizeof(int16) == 2, "int16 type size test failed.");
	static_assert(int32(int16(-1)) == -1, "int16 type sign test failed.");

	static_assert(sizeof(int32) == 4, "int32 type size test failed.");
	static_assert(int64(int32(-1)) == int64(-1), "int32 type sign test failed.");

	static_assert(sizeof(int64) == 8, "int64 type size test failed.");
	static_assert(int64(-1) < int64(0), "int64 type sign test failed.");

	static_assert(sizeof(ANSICHAR) == 1, "ANSICHAR type size test failed.");
	static_assert(int32(ANSICHAR(-1)) == -1, "ANSICHAR type sign test failed.");

	static_assert(sizeof(WIDECHAR) == 2 || sizeof(WIDECHAR) == 4, "WIDECHAR type size test failed.");

	static_assert(sizeof(UTF8CHAR)  == 1, "UTF8CHAR type size test failed.");
	static_assert(sizeof(UCS2CHAR)  == 2, "UCS2CHAR type size test failed.");
	static_assert(sizeof(UTF16CHAR) == 2, "UTF16CHAR type size test failed.");
	static_assert(sizeof(UTF32CHAR) == 4, "UTF32CHAR type size test failed.");

	static_assert(sizeof(PTRINT) == sizeof(void *), "PTRINT type size test failed.");
	static_assert(PTRINT(-1) < PTRINT(0), "PTRINT type sign test failed.");

	static_assert(sizeof(UPTRINT) == sizeof(void *), "UPTRINT type size test failed.");
	static_assert(UPTRINT(-1) > UPTRINT(0), "UPTRINT type sign test failed.");

	static_assert(sizeof(SIZE_T) == sizeof(void *), "SIZE_T type size test failed.");
	static_assert(SIZE_T(-1) > SIZE_T(0), "SIZE_T type sign test failed.");
}

// Platform specific compiler setup.
#include COMPILED_PLATFORM_HEADER(PlatformCompilerSetup.h)


#define UTF8TEXT_PASTE(x)  u8 ## x
#define UTF16TEXT_PASTE(x) u ## x
#if PLATFORM_WIDECHAR_IS_CHAR16
	#define WIDETEXT_PASTE(x)  UTF16TEXT_PASTE(x)
#else
	#define WIDETEXT_PASTE(x)  L ## x
#endif

// If we don't have a platform-specific define for the TEXT macro, define it now.
#if !defined(TEXT) && !UE_BUILD_DOCS
	#if PLATFORM_TCHAR_IS_UTF8CHAR
		#define TEXT_PASTE(x) UTF8TEXT(x)
	#else
		#define TEXT_PASTE(x) WIDETEXT(x)
	#endif
	#define TEXT(x) TEXT_PASTE(x)
#endif

namespace UE::Core::Private
{
	// Can't be constexpr because it involves casts
	template <SIZE_T N>
	FORCEINLINE auto ToUTF8Literal(const char(&Array)[N]) -> const UTF8CHAR(&)[N]
	{
		return (const UTF8CHAR(&)[N])Array;
	}

#if defined(__cpp_char8_t)
	// Can't be constexpr because it involves casts
	template <SIZE_T N>
	FORCEINLINE auto ToUTF8Literal(const char8_t (&Array)[N]) -> const UTF8CHAR(&)[N]
	{
		return (const UTF8CHAR (&)[N])Array;
	}
#endif

	FORCEINLINE constexpr UTF8CHAR ToUTF8Literal(unsigned long long Ch)
	{
		return (UTF8CHAR)Ch;
	}
}

#define UTF8TEXT(x) (UE::Core::Private::ToUTF8Literal(UTF8TEXT_PASTE(x)))

#define WIDETEXT(str) WIDETEXT_PASTE(str)
