// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/RefCounting.h"

/**
 * Represents a block of memory, but abstracts away the underlying layout
 */
class CORE_API IMemoryReadStream
{
public:
	static TRefCountPtr<IMemoryReadStream> CreateFromCopy(const void* InMemory, int64 InSize);
	static TRefCountPtr<IMemoryReadStream> CreateFromCopy(IMemoryReadStream* InStream);
	static TRefCountPtr<IMemoryReadStream> CreateFromBuffer(void* InMemory, int64 InSize, bool bOwnPointer);

	virtual const void* Read(int64& OutSize, int64 InOffset, int64 InSize) = 0;
	virtual int64 GetSize() = 0;
	virtual void CopyTo(void* Buffer, int64 InOffset, int64 InSize);
	virtual void EnsureReadNonBlocking() {};		// Incur any potential wait such that future Read() or CopyTo() calls are wait and thread switch free.

	FORCEINLINE uint32 AddRef() const { return uint32(NumRefs.Increment()); }

	FORCEINLINE uint32 Release() const
	{
		const int32 Refs = NumRefs.Decrement();
		if (Refs == 0)
		{
			delete this;
		}
		return uint32(Refs);
	}

	FORCEINLINE uint32 GetRefCount() const
	{
		return uint32(NumRefs.GetValue());
	}

protected:
	virtual ~IMemoryReadStream() {}

private:
	mutable FThreadSafeCounter NumRefs;
};
typedef TRefCountPtr<IMemoryReadStream> IMemoryReadStreamRef;
