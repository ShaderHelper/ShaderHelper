// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/ArchiveUObject.h"
#include "Serialization/MemoryLayout.h"

/**
* Calculates a checksum from the input provided to the archive.
*/
class COREUOBJECT_API FArchiveCrc32 : public FArchiveUObject
{
public:
	/**
	* Default constructor.
	*/
	FArchiveCrc32(uint32 InCRC = 0, UObject* InRootObject = nullptr);

	/**
	 * @return The checksum computed so far.
	 */
	uint32 GetCrc() const { return CRC; }

	//~ Begin FArchive Interface
	virtual void Serialize(void* Data, int64 Num) override;
	virtual FArchive& operator<<(class FName& Name);
	virtual FArchive& operator<<(class UObject*& Object);
	virtual FString GetArchiveName() const { return TEXT("FArchiveCrc32"); }
	//~ End FArchive Interface

	/**
	 * Serializes a USTRUCT value from or into an archive.
	 *
	 * @param Ar The archive to serialize from or to.
	 * @param Value The value to serialize.
	 */
	template <typename StructType>
	FORCEINLINE friend typename TEnableIf<TProvidesStaticStruct<StructType>::Value, FArchive&>::Type operator <<(FArchive& Ar, const StructType& Value)
	{
		StructType* MutableValue = const_cast<StructType*>(&Value);
		StructType::StaticStruct()->SerializeItem(Ar, MutableValue, nullptr);
		return Ar;
	}

private:
	uint32 CRC;
	UObject* RootObject;
};
