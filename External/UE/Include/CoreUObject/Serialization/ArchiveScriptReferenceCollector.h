// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "Serialization/ArchiveUObject.h"

/** Simple reference processor and collector for collecting all UObjects referenced by FProperties */
class FPropertyReferenceCollector : public FReferenceCollector
{
	/** The owner object for properties we collect references (to make sure we don't add it to the references list as it's not needed) */
	UObject* ExcludeOwner;
	/** List of unique references */
	TArray<UObject*>& UniqueReferences;
public:
	FPropertyReferenceCollector(UObject* InExcludeOwner, TArray<UObject*>& InUniqueReferences)
		: ExcludeOwner(InExcludeOwner)
		, UniqueReferences(InUniqueReferences)
	{
	}

	virtual bool IsIgnoringArchetypeRef() const override { return false; }
	virtual bool IsIgnoringTransient() const override { return false; }
	virtual void HandleObjectReference(UObject*& InObject, const UObject* InReferencingObject, const FProperty* InReferencingProperty) override
	{
		// Skip nulls and the owner object
		if (InObject != nullptr && InObject != ExcludeOwner)
		{
			// Don't collect objects that will never be GC'd anyway
			if (!InObject->HasAnyInternalFlags(EInternalObjectFlags::Native) && !GUObjectArray.IsDisregardForGC(InObject) && !UniqueReferences.Contains(InObject))
			{
				UniqueReferences.Add(InObject);
			}
		}
	}
};

/*******************************************************************************
 * FArchiveScriptReferenceCollector
 ******************************************************************************/

class FArchiveScriptReferenceCollector : public FArchiveUObject
{
public:
	/**
	* Constructor
	*
	* @param	InObjectArray			Array to add object references to
	*/
	FArchiveScriptReferenceCollector(TArray<UObject*>& InObjectArray)
		: ObjectArray(InObjectArray)
	{
		ArIsObjectReferenceCollector = true;
		this->SetIsPersistent(false);
		ArIgnoreArchetypeRef = false;
	}
protected:
	/**
	* UObject serialize operator implementation
	*
	* @param Object	reference to Object reference
	* @return reference to instance of this class
	*/
	FArchive& operator<<(UObject*& Object)
	{
		// Avoid duplicate entries.
		if (Object != nullptr && !ObjectArray.Contains(Object))
		{
			check(Object->IsValidLowLevel());
			ObjectArray.Add(Object);
		}
		return *this;
	}

	/**
	* FField serialize operator implementation
	*
	* @param Field	reference to field reference
	* @return reference to instance of this class
	*/
	FArchive& operator<<(FField*& Field)
	{
		if (Field != nullptr)
		{
			// It's faster to collect references via AddReferencedObjects than serialization
			FPropertyReferenceCollector Collector(Field->GetOwnerUObject(), ObjectArray);
			Field->AddReferencedObjects(Collector);
		}
		return *this;
	}

	/** Stored reference to array of objects we add object references to */
	TArray<UObject*>&		ObjectArray;
};
