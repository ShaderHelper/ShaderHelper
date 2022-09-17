// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Math/NumericLimits.h"
#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"
#include "Logging/LogMacros.h"
#include "CoreGlobals.h"
#include "Serialization/MemoryArchive.h"

/**
 * Archive for storing arbitrary data to the specified memory location
 */
template <int IndexSize>
class TMemoryWriter : public FMemoryArchive
{
	static_assert(IndexSize == 32 || IndexSize == 64, "Only 32-bit and 64-bit index sizes supported");
	using IndexSizeType = typename TBitsToSizeType<IndexSize>::Type;

public:
	TMemoryWriter( TArray<uint8, TSizedDefaultAllocator<IndexSize>>& InBytes, bool bIsPersistent = false, bool bSetOffset = false, const FName InArchiveName = NAME_None )
	: FMemoryArchive()
	, Bytes(InBytes)
	, ArchiveName(InArchiveName)
	{
		this->SetIsSaving(true);
		this->SetIsPersistent(bIsPersistent);
		if (bSetOffset)
		{
			Offset = InBytes.Num();
		}
	}

	virtual void Serialize(void* Data, int64 Num) override
	{
		const int64 NumBytesToAdd = Offset + Num - Bytes.Num();
		if( NumBytesToAdd > 0 )
		{
			const int64 NewArrayCount = Bytes.Num() + NumBytesToAdd;
			if constexpr (IndexSize == 32)
			{
				if (NewArrayCount >= MAX_int32)
				{
					UE_LOG(LogSerialization, Fatal, TEXT("FMemoryWriter does not support data larger than 2GB. Archive name: %s."), *ArchiveName.ToString());
				}
			}

			Bytes.AddUninitialized((IndexSizeType)NumBytesToAdd);
		}

		check((Offset + Num) <= Bytes.Num());
		
		if( Num )
		{
			FMemory::Memcpy(&Bytes[(IndexSizeType)Offset], Data, Num);
			Offset += Num;
		}
	}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FString GetArchiveName() const override
	{
		if constexpr (IndexSize == 64)
		{
			return TEXT("FMemoryWriter64");
		}
		else
		{
			return TEXT("FMemoryWriter");
		}
	}

	int64 TotalSize() override
	{
		return Bytes.Num();
	}

protected:

	TArray<uint8, TSizedDefaultAllocator<IndexSize>>&	Bytes;

	/** Archive name, used to debugging, by default set to NAME_None. */
	const FName ArchiveName;
};


// FMemoryWriter and FMemoryWriter64 are implemented as derived classes rather than aliases
// so that forward declarations will work.

class FMemoryWriter : public TMemoryWriter<32>
{
	using Super = TMemoryWriter<32>;

public:
	using Super::Super;
};

class FMemoryWriter64 : public TMemoryWriter<64>
{
	using Super = TMemoryWriter<64>;

public:
	using Super::Super;
};
