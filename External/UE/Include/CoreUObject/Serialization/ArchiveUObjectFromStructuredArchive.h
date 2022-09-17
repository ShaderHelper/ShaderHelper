// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Serialization/StructuredArchive.h"
#include "Serialization/ArchiveUObject.h"
#include "Serialization/FileRegions.h"
#include "UObject/ObjectResource.h"

#include "UObject/ObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/LazyObjectPtr.h"
#include "UObject/WeakObjectPtr.h"

#if WITH_TEXT_ARCHIVE_SUPPORT

class COREUOBJECT_API FArchiveUObjectFromStructuredArchiveImpl : public FArchiveFromStructuredArchiveImpl
{
	using Super = FArchiveFromStructuredArchiveImpl;

public:

	FArchiveUObjectFromStructuredArchiveImpl(FStructuredArchive::FSlot Slot);

	using FArchive::operator<<; // For visibility of the overloads we don't override

	//~ Begin FArchive Interface
	virtual FArchive& operator<<(FLazyObjectPtr& Value) override;
	virtual FArchive& operator<<(FSoftObjectPtr& Value) override;
	virtual FArchive& operator<<(FSoftObjectPath& Value) override;
	virtual FArchive& operator<<(FWeakObjectPtr& Value) override;
	virtual FArchive& operator<<(FObjectPtr& Value) override;
	//~ End FArchive Interface

	virtual void PushFileRegionType(EFileRegionType Type) override;
	virtual void PopFileRegionType() override;

private:

	int64 FileRegionStart = 0;
	EFileRegionType CurrentFileRegionType = EFileRegionType::None;

	TArray<FLazyObjectPtr> LazyObjectPtrs;
	TArray<FWeakObjectPtr> WeakObjectPtrs;
	TArray<FSoftObjectPtr> SoftObjectPtrs;
	TArray<FSoftObjectPath> SoftObjectPaths;

	TMap<FLazyObjectPtr, int32> LazyObjectPtrToIndex;
	TMap<FWeakObjectPtr, int32> WeakObjectPtrToIndex;
	TMap<FSoftObjectPtr, int32> SoftObjectPtrToIndex;
	TMap<FSoftObjectPath, int32> SoftObjectPathToIndex;

	virtual bool Finalize(FStructuredArchive::FRecord Record) override;
};

class FArchiveUObjectFromStructuredArchive
{
public:
	explicit FArchiveUObjectFromStructuredArchive(FStructuredArchive::FSlot InSlot)
		: Impl(InSlot)
	{
	}

	      FArchive& GetArchive()       { return Impl; }
	const FArchive& GetArchive() const { return Impl; }

	void Close() { Impl.Close(); }

private:
	FArchiveUObjectFromStructuredArchiveImpl Impl;
};

#else

class COREUOBJECT_API FArchiveUObjectFromStructuredArchive : public FArchiveFromStructuredArchive
{
public:

	FArchiveUObjectFromStructuredArchive(FStructuredArchive::FSlot InSlot)
		: FArchiveFromStructuredArchive(InSlot)
	{

	}
};

#endif