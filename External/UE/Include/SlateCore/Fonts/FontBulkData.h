// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Serialization/BulkData.h"
#include "FontBulkData.generated.h"

UCLASS()
class SLATECORE_API UFontBulkData : public UObject
{
	GENERATED_BODY()

public:
	/** Default constructor */
	UFontBulkData();

	/** Construct the bulk font data from the given file */
	void Initialize(const FString& InFontFilename);

	/** Construct the bulk font data from the given data */
	void Initialize(const void* const InFontData, const int64 InFontDataSizeBytes);

	/** Locks the bulk font data and returns a read-only pointer to it */
	const void* Lock(int64& OutFontDataSizeBytes) const;

	/** Locks the bulk font data and returns a read-only pointer to it */
	UE_DEPRECATED(5.0, "Lock with 32 bits is deprecated, use the 64 bits version.")
	const void* Lock(int32& OutFontDataSizeBytes) const;

	/** Unlock the bulk font data, after which point the pointer returned by Lock no longer is valid */
	void Unlock();

	/** Returns the size of the bulk data in bytes */
	int64 GetBulkDataSize() const;

	// UObject interface
	virtual void Serialize(FArchive& Ar) override;

private:
	/** Internal bulk data */
	FByteBulkData BulkData;

	/** Critical section to prevent concurrent access when locking the internal bulk data */
	mutable FCriticalSection CriticalSection;
};
