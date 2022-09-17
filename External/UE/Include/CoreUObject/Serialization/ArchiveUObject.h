// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FLazyObjectPtr;
struct FObjectPtr;
struct FSoftObjectPtr;
struct FSoftObjectPath;
struct FWeakObjectPtr;

/**
 * Base FArchive for serializing UObjects. Supports FLazyObjectPtr and FSoftObjectPtr serialization.
 */
class COREUOBJECT_API FArchiveUObject : public FArchive
{
public:

	static FArchive& SerializeLazyObjectPtr(FArchive& Ar, FLazyObjectPtr& Value);
	static FArchive& SerializeObjectPtr(FArchive& Ar, FObjectPtr& Value);
	static FArchive& SerializeSoftObjectPtr(FArchive& Ar, FSoftObjectPtr& Value);
	static FArchive& SerializeSoftObjectPath(FArchive& Ar, FSoftObjectPath& Value);
	static FArchive& SerializeWeakObjectPtr(FArchive& Ar, FWeakObjectPtr& Value);

	using FArchive::operator<<; // For visibility of the overloads we don't override

	//~ Begin FArchive Interface
	virtual FArchive& operator<<(FLazyObjectPtr& Value) override { return SerializeLazyObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FObjectPtr& Value) override { return SerializeObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FSoftObjectPtr& Value) override { return SerializeSoftObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FSoftObjectPath& Value) override { return SerializeSoftObjectPath(*this, Value); }
	virtual FArchive& operator<<(FWeakObjectPtr& Value) override { return SerializeWeakObjectPtr(*this, Value); }
	//~ End FArchive Interface
};
