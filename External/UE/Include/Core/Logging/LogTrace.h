// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Trace/Config.h"

#if !defined(LOGTRACE_ENABLED)
#if UE_TRACE_ENABLED && !UE_BUILD_SHIPPING
#define LOGTRACE_ENABLED 1
#else
#define LOGTRACE_ENABLED 0
#endif
#endif

#if LOGTRACE_ENABLED
#include "ProfilingDebugging/FormatArgsTrace.h"

UE_TRACE_CHANNEL_EXTERN(LogChannel, CORE_API)

struct FLogCategoryBase;

struct FLogTrace
{
	CORE_API static void OutputLogCategory(const FLogCategoryBase* Category, const TCHAR* Name, ELogVerbosity::Type DefaultVerbosity);
	CORE_API static void OutputLogMessageSpec(const void* LogPoint, const FLogCategoryBase* Category, ELogVerbosity::Type Verbosity, const ANSICHAR* File, int32 Line, const TCHAR* Format);

	template <typename... Types>
	FORCENOINLINE static void OutputLogMessage(const void* LogPoint, Types... FormatArgs)
	{
		uint8 FormatArgsBuffer[3072];
		uint16 FormatArgsSize = FFormatArgsTrace::EncodeArguments(FormatArgsBuffer, FormatArgs...);
		if (FormatArgsSize)
		{
			OutputLogMessageInternal(LogPoint, FormatArgsSize, FormatArgsBuffer);
		}
	}

private:
	CORE_API static void OutputLogMessageInternal(const void* LogPoint, uint16 EncodedFormatArgsSize, uint8* EncodedFormatArgs);
};

#define TRACE_LOG_CATEGORY(Category, Name, DefaultVerbosity) \
	FLogTrace::OutputLogCategory(Category, Name, DefaultVerbosity);

#define TRACE_LOG_MESSAGE(Category, Verbosity, Format, ...) \
	if (UE_TRACE_CHANNELEXPR_IS_ENABLED(LogChannel)) \
	{ \
		static bool PREPROCESSOR_JOIN(__LogPoint, __LINE__); \
		if (!PREPROCESSOR_JOIN(__LogPoint, __LINE__)) \
		{ \
			FLogTrace::OutputLogMessageSpec(&PREPROCESSOR_JOIN(__LogPoint, __LINE__), &Category, ELogVerbosity::Verbosity, __FILE__, __LINE__, Format); \
			PREPROCESSOR_JOIN(__LogPoint, __LINE__) = true; \
		} \
		FLogTrace::OutputLogMessage(&PREPROCESSOR_JOIN(__LogPoint, __LINE__), ##__VA_ARGS__); \
	}

#else
#define TRACE_LOG_CATEGORY(Category, Name, DefaultVerbosity)
#define TRACE_LOG_MESSAGE(Category, Verbosity, Format, ...)
#endif
