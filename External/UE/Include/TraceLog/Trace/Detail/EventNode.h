// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Trace/Config.h"

#if UE_TRACE_ENABLED

#include "Field.h"

namespace UE {
namespace Trace {
namespace Private {

////////////////////////////////////////////////////////////////////////////////
struct FEventInfo
{
	enum
	{
		Flag_None			= 0,
		Flag_Important		= 1 << 0,
		Flag_MaybeHasAux	= 1 << 1,
		Flag_NoSync			= 1 << 2,
	};

	FLiteralName			LoggerName;
	FLiteralName			EventName;
	const FFieldDesc*		Fields;
	uint16					FieldCount;
	uint16					Flags;
};

////////////////////////////////////////////////////////////////////////////////
class FEventNode
{
public:
	struct FIter
	{
		const FEventNode*	GetNext();
		void*				Inner;
	};

	static FIter			ReadNew();
	static void				OnConnect();
	TRACELOG_API uint32		Initialize(const FEventInfo* InInfo);
	void					Describe() const;
	uint32					GetUid() const { return Uid; }

private:
	FEventNode*				Next;
	const FEventInfo*		Info;
	uint32					Uid;
};

} // namespace Private
} // namespace Trace
} // namespace UE

#endif // UE_TRACE_ENABLED
