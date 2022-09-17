// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if UE_TRACE_ENABLED

namespace UE {
namespace Trace {
namespace Private {

////////////////////////////////////////////////////////////////////////////////
struct FWriteBuffer
{
	uint8						Overflow[6];
	uint16						Size;
	uint64						PrevTimestamp;
	FWriteBuffer* __restrict	NextThread;
	FWriteBuffer* __restrict	NextBuffer;
	uint8* __restrict			Cursor;
	uint8* __restrict volatile	Committed;
	uint8* __restrict			Reaped;
	int32 volatile				EtxOffset;
	int16						Partial;
	uint16						ThreadId;
};

////////////////////////////////////////////////////////////////////////////////
TRACELOG_API uint64				TimeGetTimestamp();
TRACELOG_API FWriteBuffer*		Writer_NextBuffer(int32);
TRACELOG_API FWriteBuffer*		Writer_GetBuffer();

////////////////////////////////////////////////////////////////////////////////
#if IS_MONOLITHIC
extern thread_local FWriteBuffer* GTlsWriteBuffer;
inline FWriteBuffer* Writer_GetBuffer()
{
	return GTlsWriteBuffer;
}
#endif // IS_MONOLITHIC

////////////////////////////////////////////////////////////////////////////////
inline uint64 Writer_GetTimestamp(FWriteBuffer* Buffer)
{
	uint64 Ret = TimeGetTimestamp() - Buffer->PrevTimestamp;
	Buffer->PrevTimestamp += Ret;
	return Ret;
}

} // namespace Private
} // namespace Trace
} // namespace UE

#endif // UE_TRACE_ENABLED
