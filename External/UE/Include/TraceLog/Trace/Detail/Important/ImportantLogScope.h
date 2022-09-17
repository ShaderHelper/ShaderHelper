// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Trace/Config.h"

#if UE_TRACE_ENABLED

namespace UE {
namespace Trace {
namespace Private {

////////////////////////////////////////////////////////////////////////////////
class FImportantLogScope
{
public:
	template <typename EventType>
	static FImportantLogScope	Enter();
	template <typename EventType>
	static FImportantLogScope	Enter(uint32 ArrayDataSize);
	void						operator += (const FImportantLogScope&) const;
	const FImportantLogScope&	operator << (bool) const	{ return *this; }
	constexpr explicit			operator bool () const		{ return true; }

	template <typename FieldMeta, typename Type>
	struct FFieldSet;

private:
	static FImportantLogScope	EnterImpl(uint32 Uid, uint32 Size);
	uint8*						Ptr;
	int32						BufferOffset;
	int32						AuxCursor;
};

} // namespace Private
} // namespace Trace
} // namespace UE

#endif // UE_TRACE_ENABLED
