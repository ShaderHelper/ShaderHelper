// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GCObject.h: Abstract base class to allow non-UObject objects reference
				UObject instances with proper handling of them by the
				Garbage Collector.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

class FGCObject;

class COREUOBJECT_API FGCObject;

/**
 * This nested class is used to provide a UObject interface between non
 * UObject classes and the UObject system. It handles forwarding all
 * calls of AddReferencedObjects() to objects/ classes that register with it.
 */
class COREUOBJECT_API UGCObjectReferencer : public UObject
{
	/**
	 * This is the list of objects that are referenced
	 */
	TArray<FGCObject*> ReferencedObjects;
	/** Critical section used when adding and removing objects */
	FCriticalSection ReferencedObjectsCritical;
	/** True if we are currently inside AddReferencedObjects */
	bool bIsAddingReferencedObjects = false;
	/** True if the list of referenced objects has changed since we last ran VerifyGCObjectNames */
	bool bReferencedObjectsChangedSinceLastNameVerify = false;
	/** Currently serializing FGCObject*, only valid if bIsAddingReferencedObjects */
	FGCObject* CurrentlySerializingObject = nullptr;

public:
	DECLARE_CASTED_CLASS_INTRINSIC_WITH_API_NO_CTOR(UGCObjectReferencer, UObject, CLASS_Transient, TEXT("/Script/CoreUObject"), CASTCLASS_None, NO_API);

	UGCObjectReferencer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UGCObjectReferencer(FVTableHelper& Helper) : Super(Helper) {};

	/**
	 * Adds an object to the referencer list
	 *
	 * @param Object The object to add to the list
	 */
	void AddObject(FGCObject* Object);

	/**
	 * Removes an object from the referencer list
	 *
	 * @param Object The object to remove from the list
	 */
	void RemoveObject(FGCObject* Object);

	/**
	 * Get the name of the first FGCObject that owns this object.
	 *
	 * @param Object The object that we're looking for.
	 * @param OutName the name of the FGCObject that reports this object.
	 * @param bOnlyIfAddingReferenced Only try to find the name if we are currently inside AddReferencedObjects
	 * @return true if the object was found.
	 */
	bool GetReferencerName(UObject* Object, FString& OutName, bool bOnlyIfAddingReferenced = false) const;

	/**
	 * Forwards this call to all registered objects so they can reference
	 * any UObjects they depend upon
	 *
	 * @param InThis This UGCObjectReferencer object.
	 * @param Collector The collector of referenced objects.
	 */
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	
	/**
	 * Destroy function that gets called before the object is freed. This might
	 * be as late as from the destructor.
	 */
	virtual void FinishDestroy() override;

	/**
	 * Verifies that all FGCObjects have specified names
	 */
	void VerifyGCObjectNames();

	/**
	 * Returns the currently serializing object
	 */
	FGCObject* GetCurrentlySerializingObject() const
	{
		return CurrentlySerializingObject;
	}

#if WITH_EDITORONLY_DATA
	/** Called when a new FGCObject is added to this referencer */
	DECLARE_MULTICAST_DELEGATE_OneParam(FGCObjectAddedDelegate, FGCObject*);

private:

	/** Called when a new FGCObject is added to this referencer */
	FGCObjectAddedDelegate OnGCObjectAdded;

public:

	/** Returns a delegate called when a new FGCObject is added to this referencer */
	FGCObjectAddedDelegate& GetGCObjectAddedDelegate()
	{
		return OnGCObjectAdded;
	}
#endif // WITH_EDITORONLY_DATA
};


/**
 * This class provides common registration for garbage collection for
 * non-UObject classes. It is an abstract base class requiring you to implement
 * the AddReferencedObjects() method.
 */
class COREUOBJECT_API FGCObject
{
	bool bReferenceAdded = false;

	void Init()
	{
		// Some objects can get created after the engine started shutting down (lazy init of singletons etc).
		if (!IsEngineExitRequested())
		{
			StaticInit();
			check(GGCObjectReferencer);
			// Add this instance to the referencer's list
			GGCObjectReferencer->AddObject(this);
			bReferenceAdded = true;
		}
	}

public:
	/**
	 * The static object referencer object that is shared across all
	 * garbage collectible non-UObject objects.
	 */
	static UGCObjectReferencer* GGCObjectReferencer;

	/** Default name for unnamed FGCObjects */
	static const TCHAR* UnknownGCObjectName;

	/**
	 * Initializes the global object referencer and adds it to the root set.
	 */
	static void StaticInit(void)
	{
		if (GGCObjectReferencer == NULL)
		{
			GGCObjectReferencer = NewObject<UGCObjectReferencer>();
			GGCObjectReferencer->AddToRoot();
		}
	}

	/**
	 * Tells the global object that forwards AddReferencedObjects calls on to objects
	 * that a new object is requiring AddReferencedObjects call.
	 */
	FGCObject(void)
	{
		Init();
	}

	/** Copy constructor */
	FGCObject(FGCObject const&)
	{
		Init();
	}

	/** Move constructor */
	FGCObject(FGCObject&&)
	{
		Init();
	}

	/**
	 * Removes this instance from the global referencer's list
	 */
	virtual ~FGCObject(void)
	{
		// GObjectSerializer will be NULL if this object gets destroyed after the exit purge.
		// We want to make sure we remove any objects that were added to the GGCObjectReferencer during Init when exiting
		if (GGCObjectReferencer && bReferenceAdded)
		{
			// Remove this instance from the referencer's list
			GGCObjectReferencer->RemoveObject(this);
		}
	}

	/**
	 * Pure virtual that must be overloaded by the inheriting class. Use this
	 * method to serialize any UObjects contained that you wish to keep around.
	 *
	 * @param Collector The collector of referenced objects.
	 */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) = 0;

	/**
	 * Use this method to report a name for your referencer.
	 */
	virtual FString GetReferencerName() const
	{
		return UnknownGCObjectName;
	}

	/**
	 * Use this method to report how the specified object is referenced, if necessary
	 */
	virtual bool GetReferencerPropertyName(UObject* Object, FString& OutPropertyName) const
	{
		return false;
	}
};

