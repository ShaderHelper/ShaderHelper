// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/ArchiveUObject.h"
#include "UObject/UObjectAnnotation.h"
#include "Serialization/DuplicatedObject.h"
#include "Serialization/LargeMemoryData.h"
#include "Templates/RefCounting.h"
#include "UObject/UObjectThreadContext.h"

/*----------------------------------------------------------------------------
	FDuplicateDataReader.
----------------------------------------------------------------------------*/

/**
 * Reads duplicated objects from a memory buffer, replacing object references to duplicated objects.
 */
class FDuplicateDataReader : public FArchiveUObject
{
private:

	class FUObjectAnnotationSparse<FDuplicatedObject,false>&	DuplicatedObjectAnnotation;
	const FLargeMemoryData&					ObjectData;
	int64									Offset;

	/** Context for duplication */
	TRefCountPtr<FUObjectSerializeContext> DuplicateContext;

	//~ Begin FArchive Interface.

	virtual FArchive& operator<<(FName& N);
	virtual FArchive& operator<<(UObject*& Object);
	virtual FArchive& operator<<(FLazyObjectPtr& LazyObjectPtr);
	virtual FArchive& operator<<(FSoftObjectPath& SoftObjectPath);
	
	void SerializeFail();

	virtual void Serialize(void* Data,int64 Num)
	{
		if (ObjectData.Read(Data, Offset, Num))
		{
			Offset += Num;
		}
		else
		{
			SerializeFail();
		}
	}

	virtual void Seek(int64 InPos)
	{
		Offset = InPos;
	}

public:
	/**
	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FString GetArchiveName() const { return TEXT("FDuplicateDataReader"); }

	virtual int64 Tell()
	{
		return Offset;
	}
	virtual int64 TotalSize()
	{
		return ObjectData.GetSize();
	}

	virtual void SetSerializeContext(FUObjectSerializeContext* InLoadContext) override
	{
		DuplicateContext = InLoadContext;
	}

	virtual FUObjectSerializeContext* GetSerializeContext() override
	{
		return DuplicateContext;
	}

	/**
	 * Constructor
	 * 
	 * @param	InDuplicatedObjectAnnotation		Annotation for storing a mapping from source to duplicated object
	 * @param	InObjectData					Object data to read from
	 */
	FDuplicateDataReader( FUObjectAnnotationSparse<FDuplicatedObject,false>& InDuplicatedObjectAnnotation, const FLargeMemoryData& InObjectData, uint32 InPortFlags, UObject* InDestOuter );
};