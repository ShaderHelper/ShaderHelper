// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Information about a duplicated object
 * For use with a dense object annotation
 */
struct FDuplicatedObject
{
	/** The duplicated object */
	TWeakObjectPtr<UObject> DuplicatedObject;

	FDuplicatedObject()
	{
	}

	FDuplicatedObject( UObject* InDuplicatedObject )
		: DuplicatedObject( InDuplicatedObject )
	{
	}

	/**
	 * @return true if this is the default annotation and holds no information about a duplicated object
	 */
	FORCEINLINE bool IsDefault()
	{
		return DuplicatedObject.IsExplicitlyNull();
	}
};

template <> struct TIsPODType<FDuplicatedObject> { enum { Value = true }; };

