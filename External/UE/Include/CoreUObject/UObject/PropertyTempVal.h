// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/UnrealType.h"

// A helper struct which owns a single instance of the type pointed to by a property.
// The instance is properly constructed, destructed and can be serialized and have other
// functions called on it.
struct COREUOBJECT_API FPropertyTempVal
{
	explicit FPropertyTempVal(FProperty* InProp);
	~FPropertyTempVal();

	// Uncopyable
	FPropertyTempVal(const FPropertyTempVal&) = delete;
	FPropertyTempVal& operator=(const FPropertyTempVal&) = delete;

	// Serializes the instance
	void Serialize(FArchive& Ar, const void* Defaults = nullptr);

	// Exports the text of the instance
	void ExportText(FString& ValueStr, const void* Defaults = nullptr, UObject* Parent = nullptr, int32 PortFlags = 0, UObject* ExportRootScope = nullptr);

	// Returns a pointer to the internal instance
	FORCEINLINE void* Get()
	{
		return Value;
	}

	// Returns a pointer to the internal instance
	FORCEINLINE const void* Get() const
	{
		return Value;
	}

private:
	// The property which is used to manage to underlying instance.
	FProperty* Prop;

	// The memory of 
	void* Value;
};
