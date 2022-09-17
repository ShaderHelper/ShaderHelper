// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Detail/Trace.h"

////////////////////////////////////////////////////////////////////////////////
#if UE_TRACE_ENABLED
#	define UE_TRACE_IMPL(...)
#	define UE_TRACE_API			TRACELOG_API
#else
#	define UE_TRACE_IMPL(...)	{ return __VA_ARGS__; }
#	define UE_TRACE_API			inline
#endif

////////////////////////////////////////////////////////////////////////////////
namespace UE {
namespace Trace {

// Field types
enum AnsiString {};
enum WideString {};

struct FInitializeDesc
{
	uint32			TailSizeBytes		= 4 << 20;
	bool			bUseWorkerThread	= true;
};

typedef void*		AllocFunc(SIZE_T, uint32);
typedef void		FreeFunc(void*, SIZE_T);
typedef void		ChannelIterFunc(const ANSICHAR*, bool, void*);	

struct FStatistics
{
	uint64		BytesSent;
	uint64		BytesTraced;
	uint32		MemoryUsed;
	uint32		CacheUsed;
	uint32		CacheWaste;
};

UE_TRACE_API void	SetMemoryHooks(AllocFunc Alloc, FreeFunc Free) UE_TRACE_IMPL();
UE_TRACE_API void	Initialize(const FInitializeDesc& Desc) UE_TRACE_IMPL();
UE_TRACE_API void	StartWorkerThread() UE_TRACE_IMPL();	
UE_TRACE_API void	Shutdown() UE_TRACE_IMPL();
UE_TRACE_API void	Update() UE_TRACE_IMPL();
UE_TRACE_API void	GetStatistics(FStatistics& Out) UE_TRACE_IMPL();
UE_TRACE_API bool	SendTo(const TCHAR* Host, uint32 Port=0) UE_TRACE_IMPL(false);
UE_TRACE_API bool	WriteTo(const TCHAR* Path) UE_TRACE_IMPL(false);
UE_TRACE_API bool	IsTracing() UE_TRACE_IMPL(false);
UE_TRACE_API bool	Stop() UE_TRACE_IMPL(false);
UE_TRACE_API bool	IsChannel(const TCHAR* ChanneName) UE_TRACE_IMPL(false);
UE_TRACE_API bool	ToggleChannel(const TCHAR* ChannelName, bool bEnabled) UE_TRACE_IMPL(false);
UE_TRACE_API void	EnumerateChannels(ChannelIterFunc IterFunc, void* User);
UE_TRACE_API void	ThreadRegister(const TCHAR* Name, uint32 SystemId, int32 SortHint) UE_TRACE_IMPL();
UE_TRACE_API void	ThreadGroupBegin(const TCHAR* Name) UE_TRACE_IMPL();
UE_TRACE_API void	ThreadGroupEnd() UE_TRACE_IMPL();

} // namespace Trace
} // namespace UE

////////////////////////////////////////////////////////////////////////////////
#define UE_TRACE_EVENT_DEFINE(LoggerName, EventName)					TRACE_PRIVATE_EVENT_DEFINE(LoggerName, EventName)
#define UE_TRACE_EVENT_BEGIN(LoggerName, EventName, ...)				TRACE_PRIVATE_EVENT_BEGIN(LoggerName, EventName, ##__VA_ARGS__)
#define UE_TRACE_EVENT_BEGIN_EXTERN(LoggerName, EventName, ...)			TRACE_PRIVATE_EVENT_BEGIN_EXTERN(LoggerName, EventName, ##__VA_ARGS__)
#define UE_TRACE_EVENT_FIELD(FieldType, FieldName)						TRACE_PRIVATE_EVENT_FIELD(FieldType, FieldName)
#define UE_TRACE_EVENT_END()											TRACE_PRIVATE_EVENT_END()
#define UE_TRACE_LOG(LoggerName, EventName, ChannelsExpr, ...)			TRACE_PRIVATE_LOG(LoggerName, EventName, ChannelsExpr, ##__VA_ARGS__)
#define UE_TRACE_LOG_SCOPED(LoggerName, EventName, ChannelsExpr, ...)	TRACE_PRIVATE_LOG_SCOPED(LoggerName, EventName, ChannelsExpr, ##__VA_ARGS__)
#define UE_TRACE_LOG_SCOPED_T(LoggerName, EventName, ChannelsExpr, ...)	TRACE_PRIVATE_LOG_SCOPED_T(LoggerName, EventName, ChannelsExpr, ##__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
#define UE_TRACE_CHANNEL(ChannelName, ...)				TRACE_PRIVATE_CHANNEL(ChannelName, ##__VA_ARGS__)
#define UE_TRACE_CHANNEL_EXTERN(ChannelName, ...)		TRACE_PRIVATE_CHANNEL_EXTERN(ChannelName, ##__VA_ARGS__)
#define UE_TRACE_CHANNEL_DEFINE(ChannelName, ...)		TRACE_PRIVATE_CHANNEL_DEFINE(ChannelName, ##__VA_ARGS__)
#define UE_TRACE_CHANNELEXPR_IS_ENABLED(ChannelsExpr)	TRACE_PRIVATE_CHANNELEXPR_IS_ENABLED(ChannelsExpr)
