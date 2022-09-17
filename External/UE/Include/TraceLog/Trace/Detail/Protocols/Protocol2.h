// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

namespace UE {
namespace Trace {

#if defined(TRACE_PRIVATE_PROTOCOL_2)
inline
#endif
namespace Protocol2
{

////////////////////////////////////////////////////////////////////////////////
enum EProtocol : uint8 { Id = 2 };

////////////////////////////////////////////////////////////////////////////////
using Protocol1::EFieldType;
using Protocol1::FNewEventEvent;
using Protocol1::EEventFlags;
using Protocol1::EKnownEventUids;
using Protocol1::FAuxHeader;

////////////////////////////////////////////////////////////////////////////////
struct FEventHeader
{
	uint16		Uid;
	uint16		Size;
};

////////////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)
struct FEventHeaderSync
	: public FEventHeader
{
	uint16		SerialLow;		// 24-bit...
	uint8		SerialHigh;		// ...serial no.
	uint8		EventData[];
};
#pragma pack(pop)
static_assert(sizeof(FEventHeaderSync) == 7, "Packing assumption doesn't hold");

} // namespace Protocol2
} // namespace Trace
} // namespace UE
