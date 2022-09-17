// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Trace/Config.h"

#if UE_TRACE_ENABLED

#include <type_traits>

namespace UE {
namespace Trace {

class FChannel;

} // namespace Trace
} // namespace UE

#define TRACE_PRIVATE_STATISTICS (!UE_BUILD_SHIPPING)

#define TRACE_PRIVATE_CHANNEL_DEFAULT_ARGS false, "None"

#define TRACE_PRIVATE_CHANNEL_DECLARE(LinkageType, ChannelName) \
	static UE::Trace::FChannel ChannelName##Object; \
	LinkageType UE::Trace::FChannel& ChannelName = ChannelName##Object;

#define TRACE_PRIVATE_CHANNEL_IMPL(ChannelName, ...) \
	struct F##ChannelName##Registrator \
	{ \
		F##ChannelName##Registrator() \
		{ \
			ChannelName##Object.Setup(#ChannelName, { __VA_ARGS__ } ); \
		} \
	}; \
	static F##ChannelName##Registrator ChannelName##Reg = F##ChannelName##Registrator();

#define TRACE_PRIVATE_CHANNEL(ChannelName, ...) \
	TRACE_PRIVATE_CHANNEL_DECLARE(static, ChannelName) \
	TRACE_PRIVATE_CHANNEL_IMPL(ChannelName, ##__VA_ARGS__)

#define TRACE_PRIVATE_CHANNEL_DEFINE(ChannelName, ...) \
	TRACE_PRIVATE_CHANNEL_DECLARE(, ChannelName) \
	TRACE_PRIVATE_CHANNEL_IMPL(ChannelName, ##__VA_ARGS__)

#define TRACE_PRIVATE_CHANNEL_EXTERN(ChannelName, ...) \
	__VA_ARGS__ extern UE::Trace::FChannel& ChannelName;

#define TRACE_PRIVATE_CHANNELEXPR_IS_ENABLED(ChannelsExpr) \
	bool(ChannelsExpr)

#define TRACE_PRIVATE_EVENT_DEFINE(LoggerName, EventName) \
	UE::Trace::Private::FEventNode LoggerName##EventName##Event;

#define TRACE_PRIVATE_EVENT_BEGIN(LoggerName, EventName, ...) \
	TRACE_PRIVATE_EVENT_BEGIN_IMPL(static, LoggerName, EventName, ##__VA_ARGS__)

#define TRACE_PRIVATE_EVENT_BEGIN_EXTERN(LoggerName, EventName, ...) \
	TRACE_PRIVATE_EVENT_BEGIN_IMPL(extern, LoggerName, EventName, ##__VA_ARGS__)

#define TRACE_PRIVATE_EVENT_BEGIN_IMPL(LinkageType, LoggerName, EventName, ...) \
	LinkageType TRACE_PRIVATE_EVENT_DEFINE(LoggerName, EventName) \
	struct F##LoggerName##EventName##Fields \
	{ \
		enum \
		{ \
			Important			= UE::Trace::Private::FEventInfo::Flag_Important, \
			NoSync				= UE::Trace::Private::FEventInfo::Flag_NoSync, \
			PartialEventFlags	= (0, ##__VA_ARGS__), \
		}; \
		enum : bool { bIsImportant = ((0, ##__VA_ARGS__) & Important) != 0, }; \
		static constexpr uint32 GetSize() { return EventProps_Meta::Size; } \
		static uint32 GetUid() { static uint32 Uid = 0; return (Uid = Uid ? Uid : Initialize()); } \
		static uint32 FORCENOINLINE Initialize() \
		{ \
			static const uint32 Uid_ThreadSafeInit = [] () \
			{ \
				using namespace UE::Trace; \
				static F##LoggerName##EventName##Fields Fields; \
				static UE::Trace::Private::FEventInfo Info = \
				{ \
					FLiteralName(#LoggerName), \
					FLiteralName(#EventName), \
					(FFieldDesc*)(&Fields), \
					uint16(sizeof(Fields) / sizeof(FFieldDesc)), \
					uint16(EventFlags), \
				}; \
				return LoggerName##EventName##Event.Initialize(&Info); \
			}(); \
			return Uid_ThreadSafeInit; \
		} \
		typedef UE::Trace::TField<0 /*Index*/, 0 /*Offset*/,

#define TRACE_PRIVATE_EVENT_FIELD(FieldType, FieldName) \
		FieldType> FieldName##_Meta; \
		FieldName##_Meta const FieldName##_Field = UE::Trace::FLiteralName(#FieldName); \
		template <typename... Ts> auto FieldName(Ts... ts) const { \
			LogScopeType::FFieldSet<FieldName##_Meta, FieldType>::Impl((LogScopeType*)this, Forward<Ts>(ts)...); \
			return true; \
		} \
		typedef UE::Trace::TField< \
			FieldName##_Meta::Index + 1, \
			FieldName##_Meta::Offset + FieldName##_Meta::Size,

#define TRACE_PRIVATE_EVENT_END() \
		UE::Trace::EventProps> EventProps_Meta; \
		EventProps_Meta const EventProps_Private = {}; \
		typedef std::conditional<bIsImportant, UE::Trace::Private::FImportantLogScope, UE::Trace::Private::FLogScope>::type LogScopeType; \
		explicit operator bool () const { return true; } \
		enum { EventFlags = PartialEventFlags|(EventProps_Meta::NumAuxFields ? UE::Trace::Private::FEventInfo::Flag_MaybeHasAux : 0), }; \
		static_assert( \
			!bIsImportant || (EventFlags & UE::Trace::Private::FEventInfo::Flag_NoSync), \
			"Trace events flagged as Important events must be marked NoSync" \
		); \
	};

#define TRACE_PRIVATE_LOG_PRELUDE(EnterFunc, LoggerName, EventName, ChannelsExpr, ...) \
	if (TRACE_PRIVATE_CHANNELEXPR_IS_ENABLED(ChannelsExpr)) \
		if (auto LogScope = F##LoggerName##EventName##Fields::LogScopeType::EnterFunc<F##LoggerName##EventName##Fields>(__VA_ARGS__)) \
			if (const auto& __restrict EventName = *(F##LoggerName##EventName##Fields*)(&LogScope))

#define TRACE_PRIVATE_LOG_EPILOG() \
				LogScope += LogScope

#define TRACE_PRIVATE_LOG(LoggerName, EventName, ChannelsExpr, ...) \
	TRACE_PRIVATE_LOG_PRELUDE(Enter, LoggerName, EventName, ChannelsExpr, ##__VA_ARGS__) \
		TRACE_PRIVATE_LOG_EPILOG()

#define TRACE_PRIVATE_LOG_SCOPED(LoggerName, EventName, ChannelsExpr, ...) \
	UE::Trace::Private::FScopedLogScope PREPROCESSOR_JOIN(TheScope, __LINE__); \
	TRACE_PRIVATE_LOG_PRELUDE(ScopedEnter, LoggerName, EventName, ChannelsExpr, ##__VA_ARGS__) \
		PREPROCESSOR_JOIN(TheScope, __LINE__).SetActive(), \
		TRACE_PRIVATE_LOG_EPILOG()

#define TRACE_PRIVATE_LOG_SCOPED_T(LoggerName, EventName, ChannelsExpr, ...) \
	UE::Trace::Private::FScopedStampedLogScope PREPROCESSOR_JOIN(TheScope, __LINE__); \
	TRACE_PRIVATE_LOG_PRELUDE(ScopedStampedEnter, LoggerName, EventName, ChannelsExpr, ##__VA_ARGS__) \
		PREPROCESSOR_JOIN(TheScope, __LINE__).SetActive(), \
		TRACE_PRIVATE_LOG_EPILOG()

#else

#define TRACE_PRIVATE_CHANNEL(ChannelName, ...)

#define TRACE_PRIVATE_CHANNEL_EXTERN(ChannelName, ...)

#define TRACE_PRIVATE_CHANNEL_DEFINE(ChannelName, ...)

#define TRACE_PRIVATE_CHANNELEXPR_IS_ENABLED(ChannelsExpr) \
	false

#define TRACE_PRIVATE_EVENT_DEFINE(LoggerName, EventName)

#define TRACE_PRIVATE_EVENT_BEGIN(LoggerName, EventName, ...) \
	TRACE_PRIVATE_EVENT_BEGIN_IMPL(LoggerName, EventName)

#define TRACE_PRIVATE_EVENT_BEGIN_EXTERN(LoggerName, EventName, ...) \
	TRACE_PRIVATE_EVENT_BEGIN_IMPL(LoggerName, EventName)

#define TRACE_PRIVATE_EVENT_BEGIN_IMPL(LoggerName, EventName) \
	struct F##LoggerName##EventName##Dummy \
	{ \
		struct FTraceDisabled \
		{ \
			const FTraceDisabled& operator () (...) const { return *this; } \
		}; \
		const F##LoggerName##EventName##Dummy& operator << (const FTraceDisabled&) const \
		{ \
			return *this; \
		} \
		explicit operator bool () const { return false; }

#define TRACE_PRIVATE_EVENT_FIELD(FieldType, FieldName) \
		const FTraceDisabled& FieldName;

#define TRACE_PRIVATE_EVENT_END() \
	};

#define TRACE_PRIVATE_LOG(LoggerName, EventName, ...) \
	if (const auto& EventName = *(F##LoggerName##EventName##Dummy*)1) \
		EventName

#define TRACE_PRIVATE_LOG_SCOPED(LoggerName, EventName, ...) \
	if (const auto& EventName = *(F##LoggerName##EventName##Dummy*)1) \
		EventName

#define TRACE_PRIVATE_LOG_SCOPED_T(LoggerName, EventName, ...) \
	if (const auto& EventName = *(F##LoggerName##EventName##Dummy*)1) \
		EventName

#endif // UE_TRACE_ENABLED
