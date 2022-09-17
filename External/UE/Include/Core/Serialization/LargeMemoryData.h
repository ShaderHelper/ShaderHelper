// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

/**
* Data storage for the large memory reader and writer.
*/

class CORE_API FLargeMemoryData
{
public:

	explicit FLargeMemoryData(const int64 PreAllocateBytes = 0);
	~FLargeMemoryData();

	/** Write data at the given offset. Returns true if the data was written. */
	bool Write(void* InData, int64 InOffset, int64 InNum);

	/** Append data at the given offset. */
	FORCEINLINE void Append(void* InData, int64 InNum)
	{
		Write(InData, NumBytes, InNum);
	}

	/** Read data at the given offset. Returns true if the data was read. */
	bool Read(void* OutData, int64 InOffset, int64 InNum) const;

	/** Gets the size of the data written. */
	FORCEINLINE int64 GetSize() const
	{
		return NumBytes;
	}

	/** Returns the written data.  */
	FORCEINLINE uint8* GetData()
	{
		return Data;
	}

	/** Returns the written data.  */
	FORCEINLINE const uint8* GetData() const
	{
		return Data;
	}

	/** Releases ownership of the written data. */
	uint8* ReleaseOwnership();

	/** Check whether data is allocated or if the ownership was released. */
	bool HasData() const 
	{
		return Data != nullptr;
	}

	void Reserve(int64 Size);

private:

	/** Non-copyable */
	FLargeMemoryData(const FLargeMemoryData&) = delete;
	FLargeMemoryData& operator=(const FLargeMemoryData&) = delete;

	/** Memory owned by this archive. Ownership can be released by calling ReleaseOwnership() */
	uint8* Data;

	/** Number of bytes currently written to our data buffer */
	int64 NumBytes;

	/** Number of bytes currently allocated for our data buffer */
	int64 MaxBytes;

	/** Resizes the data buffer to at least NumBytes with some slack */
	void GrowBuffer();
};
