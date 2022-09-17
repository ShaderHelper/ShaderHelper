// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GarbageCollectionHistory.h: Unreal realtime garbage collection history
=============================================================================*/

#pragma once

#include "UObject/GarbageCollection.h"
#include "UObject/Class.h"

/**
 * Structure containing information about a UObject participating in Garbage Collection.
 * It's purpose is to avoid holding onto direct references to UObjects which may have already been Garbage Collected.
 * FGCObjectInfo interface mimics that of UObject.
 **/
class COREUOBJECT_API FGCObjectInfo
{
public:

	FGCObjectInfo() = default;
	explicit FGCObjectInfo(const UObject* Obj)
		: Name(Obj->GetFName())
		, Flags(Obj->GetFlags())
		, InternalFlags(Obj->GetInternalFlags())
		, ClusterRootIndex(GUObjectArray.ObjectToObjectItem(Obj)->ClusterRootIndex)
		, bDisregardForGC(GUObjectArray.IsDisregardForGC(Obj))
	{

	}

private:

	/** Name of the object */
	FName Name;
	/** Pointer to class info */
	FGCObjectInfo* Class = nullptr;
	/** Pointer to Outer info */
	FGCObjectInfo* Outer = nullptr;
	/** Captured Object flags */
	EObjectFlags Flags = RF_NoFlags;
	/** Captured Internal flags */
	EInternalObjectFlags InternalFlags = EInternalObjectFlags::None;
	/** Object's FGCObjectItem cluster root index */
	int32 ClusterRootIndex = -1;
	/** True if the object was inside of the disregard for GC set */
	bool bDisregardForGC = false;

public:

	/**
	 * Tries to find an existing object matching the stored Path.
	 * @returns Pointer to an object this info struct represents if the object is still alive, nullptr otherwise
	 */
	UObject* TryResolveObject();

	FGCObjectInfo* GetClass() const
	{
		return Class;
	}

	FGCObjectInfo* GetOuter() const
	{
		return Outer;
	}

	inline bool IsIn(const FGCObjectInfo* MaybeOuter) const
	{		
		for (const FGCObjectInfo* Obj = this; Obj; Obj = Obj->Outer)
		{
			if (Obj == MaybeOuter)
			{
				return true;
			}
		}
		return false;
	}

	bool HasAnyFlags(EObjectFlags InFlags) const
	{
		return !!(Flags & InFlags);
	}

	bool HasAnyInternalFlags(EInternalObjectFlags InFlags) const
	{
		return !!(InternalFlags & InFlags);
	}

	bool IsRooted() const
	{
		return HasAnyInternalFlags(EInternalObjectFlags::RootSet);
	}

	bool IsNative() const
	{
		return HasAnyInternalFlags(EInternalObjectFlags::Native);
	}

	int32 GetOwnerIndex() const
	{
		return ClusterRootIndex;
	}

	FString GetClassName() const
	{
		check(Class);
		return Class->Name.ToString();
	}

	bool IsValid() const
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return !HasAnyInternalFlags(EInternalObjectFlags::Garbage | EInternalObjectFlags::PendingKill);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	bool IsDisregardForGC() const
	{
		return bDisregardForGC;
	}

	void GetPathName(FStringBuilderBase& ResultString) const;
	FString GetPathName() const;
	FString GetFullName() const;

	friend uint32 GetTypeHash(const FGCObjectInfo& Info)
	{
		return HashCombine(GetTypeHash(Info.Outer), GetTypeHash(Info.Name));
	}

	/** Helper function for adding info about an UObject into UObject to FGCObjectInfo map */
	static FGCObjectInfo* FindOrAddInfoHelper(UObject* InObject, TMap<UObject*, FGCObjectInfo*>& InOutObjectToInfoMap);
};
