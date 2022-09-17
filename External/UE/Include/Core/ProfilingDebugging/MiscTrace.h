// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Trace/Config.h"
#include "ProfilingDebugging/FormatArgsTrace.h"

#if !defined(MISCTRACE_ENABLED)
#if UE_TRACE_ENABLED && !UE_BUILD_SHIPPING
#define MISCTRACE_ENABLED 1
#else
#define MISCTRACE_ENABLED 0
#endif
#endif

enum ETraceFrameType
{
	TraceFrameType_Game,
	TraceFrameType_Rendering,

	TraceFrameType_Count
};

struct FTraceUtils
{
	static void Encode7bit(uint64 Value, uint8*& BufferPtr)
	{
		do
		{
			uint8 HasMoreBytes = (uint8)((Value > uint64(0x7F)) << 7);
			*(BufferPtr++) = (uint8)(Value & 0x7F) | HasMoreBytes;
			Value >>= 7;
		} while (Value > 0);
	}

	static void EncodeZigZag(int64 Value, uint8*& BufferPtr)
	{
		Encode7bit((Value << 1) ^ (Value >> 63), BufferPtr);
	}
};

#if MISCTRACE_ENABLED

class FName;

struct FMiscTrace
{
	CORE_API static void OutputBookmarkSpec(const void* BookmarkPoint, const ANSICHAR* File, int32 Line, const TCHAR* Format);
	template <typename... Types>
	static void OutputBookmark(const void* BookmarkPoint, Types... FormatArgs)
	{
		uint8 FormatArgsBuffer[4096];
		uint16 FormatArgsSize = FFormatArgsTrace::EncodeArguments(FormatArgsBuffer, FormatArgs...);
		if (FormatArgsSize)
		{
			OutputBookmarkInternal(BookmarkPoint, FormatArgsSize, FormatArgsBuffer);
		}
	}

	CORE_API static void OutputBeginFrame(ETraceFrameType FrameType);
	CORE_API static void OutputEndFrame(ETraceFrameType FrameType);

private:
	CORE_API static void OutputBookmarkInternal(const void* BookmarkPoint, uint16 EncodedFormatArgsSize, uint8* EncodedFormatArgs);
};

#define TRACE_BOOKMARK(Format, ...) \
	static bool PREPROCESSOR_JOIN(__BookmarkPoint, __LINE__); \
	if (!PREPROCESSOR_JOIN(__BookmarkPoint, __LINE__)) \
	{ \
		FMiscTrace::OutputBookmarkSpec(&PREPROCESSOR_JOIN(__BookmarkPoint, __LINE__), __FILE__, __LINE__, Format); \
		PREPROCESSOR_JOIN(__BookmarkPoint, __LINE__) = true; \
	} \
	FMiscTrace::OutputBookmark(&PREPROCESSOR_JOIN(__BookmarkPoint, __LINE__), ##__VA_ARGS__);

#define TRACE_BEGIN_FRAME(FrameType) \
	FMiscTrace::OutputBeginFrame(FrameType);

#define TRACE_END_FRAME(FrameType) \
	FMiscTrace::OutputEndFrame(FrameType);

#else

#define TRACE_BOOKMARK(...)
#define TRACE_BEGIN_FRAME(...)
#define TRACE_END_FRAME(...)

#endif
