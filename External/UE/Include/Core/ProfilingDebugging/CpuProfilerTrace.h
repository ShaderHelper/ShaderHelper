// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Trace/Trace.h"
#include "Trace/Detail/Channel.h"
#include "Trace/Detail/Channel.inl"

#if !defined(CPUPROFILERTRACE_ENABLED)
#if UE_TRACE_ENABLED && !UE_BUILD_SHIPPING
#define CPUPROFILERTRACE_ENABLED 1
#else
#define CPUPROFILERTRACE_ENABLED 0
#endif
#endif

#if !defined(CPUPROFILERTRACE_FILE_AND_LINE_ENABLED)
#define CPUPROFILERTRACE_FILE_AND_LINE_ENABLED 1
#endif

#if CPUPROFILERTRACE_ENABLED

UE_TRACE_CHANNEL_EXTERN(CpuChannel, CORE_API);

class FName;

/*
 * Facilities for tracing timed cpu events. Two types of events are supported, static events where the identifier is
 * known at compile time, and dynamic event were identifiers can be constructed in runtime. Static events have lower overhead
 * so always prefer to use them if possible.
 *
 * Events are tracked per thread, so begin/end calls must be matched and called on the same thread. It is possible to use any channel
 * to emit the events, but both that channel and the CpuChannel must then be enabled.
 *
 * Usage of the scope macros is highly encouraged in order to avoid mistakes.
 */
struct FCpuProfilerTrace
{
	/*
	 * Output cpu event definition (spec).
	 * @param Name Event name
	 * @param File Source filename
	 * @param Line Line number in source file
	 * @return Event definition id
	 */
	FORCENOINLINE CORE_API static uint32 OutputEventType(const ANSICHAR* Name, const ANSICHAR* File = nullptr, uint32 Line = 0);
	/*
	 * Output cpu event definition (spec).
	 * @param Name Event name
	 * @param File Source filename
	 * @param Line Line number in source file
	 * @return Event definition id
	 */
	FORCENOINLINE CORE_API static uint32 OutputEventType(const TCHAR* Name, const ANSICHAR* File = nullptr, uint32 Line = 0);
	/*
	 * Output begin event marker for a given spec. Must always be matched with an end event.
	 * @param SpecId Event definition id.
	 */
	CORE_API static void OutputBeginEvent(uint32 SpecId);
	/*
	 * Output begin event marker for a dynamic event name. This is more expensive than statically known event
	 * names using \ref OutputBeginEvent. Must always be matched with an end event.
	 * @param Name Name of event
	 * @param File Source filename
	 * @param Line Line number in source file
	 */
	CORE_API static void OutputBeginDynamicEvent(const ANSICHAR* Name, const ANSICHAR* File = nullptr, uint32 Line = 0);
	/*
	 * Output begin event marker for a dynamic event name. This is more expensive than statically known event
	 * names using \ref OutputBeginEvent. Must always be matched with an end event.
	 * @param Name Name of event
	 * @param File Source filename
	 * @param Line Line number in source file
	 */
	CORE_API static void OutputBeginDynamicEvent(const TCHAR* Name, const ANSICHAR* File = nullptr, uint32 Line = 0);
	/*
	 * Output begin event marker for a dynamic event identified by an FName. This is more expensive than
	 * statically known event names using \ref OutputBeginEvent, but it is faster than \ref OutputBeginDynamicEvent
	 * that receives ANSICHAR* / TCHAR* name. Must always be matched with an end event.
	 * @param Name Name of event
	 * @param File Source filename
	 * @param Line Line number in source file
	 */
	CORE_API static void OutputBeginDynamicEvent(const FName& Name, const ANSICHAR* File = nullptr, uint32 Line = 0);
	/*
	 * Output end event marker for static or dynamic event for the currently open scope.
	 */
	CORE_API static void OutputEndEvent();

	struct FEventScope
	{
		FEventScope(uint32 InSpecId, const UE::Trace::FChannel& Channel, bool bCondition)
			: bEnabled(bCondition && (Channel | CpuChannel))
		{
			if (bEnabled)
			{
				OutputBeginEvent(InSpecId);
			}
		}

		~FEventScope()
		{
			if (bEnabled)
			{
				OutputEndEvent();
			}
		}

		bool bEnabled;
	};

	struct FDynamicEventScope
	{
		FDynamicEventScope(const ANSICHAR* InEventName, const UE::Trace::FChannel& Channel, bool Condition, const ANSICHAR* File = nullptr, uint32 Line = 0)
			: bEnabled(Condition && (Channel | CpuChannel))
		{
			if (bEnabled)
			{
				OutputBeginDynamicEvent(InEventName, File, Line);
			}
		}

		FDynamicEventScope(const TCHAR* InEventName, const UE::Trace::FChannel& Channel, bool Condition, const ANSICHAR* File = nullptr, uint32 Line = 0)
			: bEnabled(Condition && (Channel | CpuChannel))
		{
			if (bEnabled)
			{
				OutputBeginDynamicEvent(InEventName, File, Line);
			}
		}

		~FDynamicEventScope()
		{
			if (bEnabled)
			{
				OutputEndEvent();
			}
		}

		bool bEnabled;
	};
};

// Advanced macro for integrating e.g. stats/named events with cpu trace
// Declares a cpu timing event for future use, conditionally and with a particular variable name for storing the id
#define TRACE_CPUPROFILER_EVENT_DECLARE(NameStr, DeclName, Channel, Condition) \
	static uint32 DeclName; \
	if ((Condition) && bool(Channel|CpuChannel) && (DeclName == 0)) { \
		DeclName = FCpuProfilerTrace::OutputEventType(NameStr, __FILE__, __LINE__); \
	}

// Advanced macro for integrating e.g. stats/named events with cpu trace
// Traces a scoped event previously declared with TRACE_CPUPROFILER_EVENT_DECLARE, conditionally
#define TRACE_CPUPROFILER_EVENT_SCOPE_USE(DeclName, ScopeName, Channel, Condition) \
	FCpuProfilerTrace::FEventScope ScopeName(DeclName, Channel, Condition);

// Trace a scoped cpu timing event providing a static string (const ANSICHAR* or const TCHAR*)
// as the scope name and a trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("My Scoped Timer A", CpuChannel)
// Note: The event will be emitted only if both the given channel and CpuChannel is enabled.
#define TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR(NameStr, Channel) \
	TRACE_CPUPROFILER_EVENT_DECLARE(NameStr, PREPROCESSOR_JOIN(__CpuProfilerEventSpecId, __LINE__), Channel, true); \
	TRACE_CPUPROFILER_EVENT_SCOPE_USE(PREPROCESSOR_JOIN(__CpuProfilerEventSpecId, __LINE__), PREPROCESSOR_JOIN(__CpuProfilerEventScope, __LINE__), Channel, true); 

// Trace a scoped cpu timing event providing a scope name (plain text) and a trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(MyScopedTimer::A, CpuChannel)
// Note: Do not use this macro with a static string because, in that case, additional quotes will
//       be added around the event scope name.
// Note: The event will be emitted only if both the given channel and CpuChannel is enabled.
#define TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(Name, Channel) \
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR(#Name, Channel)

// Trace a scoped cpu timing event providing a static string (const ANSICHAR* or const TCHAR*)
// as the scope name. It will use the Cpu trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_STR("My Scoped Timer A")
#define TRACE_CPUPROFILER_EVENT_SCOPE_STR(NameStr) \
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR(NameStr, CpuChannel)

// Trace a scoped cpu timing event providing a scope name (plain text).
// It will use the Cpu trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE(MyScopedTimer::A)
// Note: Do not use this macro with a static string because, in that case, additional quotes will
//       be added around the event scope name.
#define TRACE_CPUPROFILER_EVENT_SCOPE(Name) \
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(Name, CpuChannel)

// Trace a scoped cpu timing event providing a dynamic string (const ANSICHAR* or const TCHAR*)
// as the scope name and a trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(*MyScopedTimerNameString, CpuChannel)
// Note: This macro has a larger overhead compared to macro that accepts a plain text name
//       or a static string. Use it only if scope name really needs to be a dynamic string.
#define TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(Name, Channel) \
	FCpuProfilerTrace::FDynamicEventScope PREPROCESSOR_JOIN(__CpuProfilerEventScope, __LINE__)(Name, Channel, true, __FILE__, __LINE__);

// Trace a scoped cpu timing event providing a dynamic string (const ANSICHAR* or const TCHAR*)
// as the scope name. It will use the Cpu trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*MyScopedTimerNameString)
// Note: This macro has a larger overhead compared to macro that accepts a plain text name
//       or a static string. Use it only if scope name really needs to be a dynamic string.
#define TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(Name) \
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(Name, CpuChannel)

#else

#define TRACE_CPUPROFILER_EVENT_DECLARE(NameStr, DeclName, Channel, Condition)
#define TRACE_CPUPROFILER_EVENT_SCOPE_USE(DeclName, ScopeName, Channel, Condition)
#define TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR(NameStr, Channel)
#define TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(Name, Channel)
#define TRACE_CPUPROFILER_EVENT_SCOPE_STR(NameStr)
#define TRACE_CPUPROFILER_EVENT_SCOPE(Name)
#define TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(Name, Channel)
#define TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(Name)

#endif
