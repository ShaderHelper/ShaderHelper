// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Serialization/Archive.h"
#include "HAL/IConsoleManager.h"

/** CVar specifying the maximum serialization size for strings sent/received by the netcode */
extern CORE_API TAutoConsoleVariable<int32> CVarMaxNetStringSize;

/**
* Base class for serializing bitstreams.
*/
class CORE_API FBitArchive : public FArchive
{
public:
	/**
	 * Default Constructor
	 */
	FBitArchive()
	{
		ArMaxSerializeSize = CVarMaxNetStringSize.GetValueOnAnyThread();
	}

	virtual void SerializeBitsWithOffset( void* Src, int32 SourceBit, int64 LengthBits ) PURE_VIRTUAL(FBitArchive::SerializeBitsWithOffset,);
};