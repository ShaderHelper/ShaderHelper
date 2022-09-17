// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UObjectBaseUtility.h: Unreal UObject functions that only depend on UObjectBase
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectBase.h"
#include "UObject/UObjectArray.h"
#include "UObject/UObjectMarks.h"

#if defined(_MSC_VER) && _MSC_VER == 1900
	#ifdef PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS
		PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS
	#endif
#endif

/**
* Enum which specifies the mode in which full object names are constructed
*/
enum class EObjectFullNameFlags
{
	// Standard object full name (i.e. "Type PackageName.ObjectName:SubobjectName")
	None = 0,

	// Adds package to the type portion (i.e. "TypePackage.TypeName PackageName.ObjectName:SubobjectName")
	IncludeClassPackage = 1,
};

ENUM_CLASS_FLAGS(EObjectFullNameFlags);

/**
 * Provides utility functions for UObject, this class should not be used directly
 */
class COREUOBJECT_API UObjectBaseUtility : public UObjectBase
{
	FORCEINLINE void MarkPendingKillOnlyInternal()
	{
		SetFlagsTo(GetFlags() | RF_InternalPendingKill);
		GUObjectArray.IndexToObject(InternalIndex)->SetPendingKill();
	}
	FORCEINLINE void ClearPendingKillOnlyInternal()
	{
		SetFlagsTo(GetFlags() & ~RF_InternalPendingKill);
		GUObjectArray.IndexToObject(InternalIndex)->ClearPendingKill();
	}
	FORCEINLINE void MarkAsGarbageOnlyInternal()
	{
		SetFlagsTo(GetFlags() | RF_InternalGarbage);
		GUObjectArray.IndexToObject(InternalIndex)->ThisThreadAtomicallySetFlag(EInternalObjectFlags::Garbage);
	}
	FORCEINLINE void ClearGarbageOnlyInternal()
	{
		SetFlagsTo(GetFlags() & ~RF_InternalGarbage);
		GUObjectArray.IndexToObject(InternalIndex)->ThisThreadAtomicallyClearedFlag(EInternalObjectFlags::Garbage);
	}

	/** If true, objects will never be marked as PendingKill so references to them will not be nulled automatically by the garbage collector */
	static bool bPendingKillDisabled;

	friend void InitNoPendingKill();
	friend struct FInternalUObjectBaseUtilityIsValidFlagsChecker;

public:
	// Constructors.
	UObjectBaseUtility() {}
	UObjectBaseUtility( EObjectFlags InFlags )
		: UObjectBase(InFlags)
	{
	}


	/*-------------------
			Flags
	-------------------*/

	/** Modifies object flags for a specific object */
	FORCEINLINE void SetFlags( EObjectFlags NewFlags )
	{
		checkSlow(!(NewFlags & (RF_MarkAsNative | RF_MarkAsRootSet | RF_InternalPendingKill | RF_InternalGarbage))); // These flags can't be used outside of constructors / internal code
		checkf(!(NewFlags & RF_InternalMirroredFlags) || (GetFlags() & (NewFlags & RF_InternalMirroredFlags)) == (NewFlags & RF_InternalMirroredFlags), TEXT("RF_PendingKill and RF_garbage can not be set through SetFlags function. Use MarkAsGarbage() instead"));
		SetFlagsTo(GetFlags() | NewFlags);
	}

	/** Clears subset of flags for a specific object */
	FORCEINLINE void ClearFlags( EObjectFlags NewFlags )
	{
		checkSlow(!(NewFlags & (RF_MarkAsNative | RF_MarkAsRootSet | RF_InternalPendingKill | RF_InternalGarbage)) || NewFlags == RF_AllFlags); // These flags can't be used outside of constructors / internal code
		checkf(!(NewFlags & RF_InternalMirroredFlags) || (GetFlags() & (NewFlags & RF_InternalMirroredFlags)) == RF_NoFlags, TEXT("RF_PendingKill and RF_garbage can not be cleared through ClearFlags function. Use ClearGarbage() instead"));
		SetFlagsTo(GetFlags() & ~NewFlags);
	}

	/**
	 * Used to safely check whether any of the passed in flags are set. 
	 *
	 * @param FlagsToCheck	Object flags to check for.
	 * @return				true if any of the passed in flags are set, false otherwise  (including no flags passed in).
	 */
	FORCEINLINE bool HasAnyFlags( EObjectFlags FlagsToCheck ) const
	{
		checkSlow(!(FlagsToCheck & (RF_MarkAsNative | RF_MarkAsRootSet)) || FlagsToCheck == RF_AllFlags); // These flags can't be used outside of constructors / internal code
		return (GetFlags() & FlagsToCheck) != 0;
	}

	/**
	 * Used to safely check whether all of the passed in flags are set. 
	 *
	 * @param FlagsToCheck	Object flags to check for
	 * @return true if all of the passed in flags are set (including no flags passed in), false otherwise
	 */
	FORCEINLINE bool HasAllFlags( EObjectFlags FlagsToCheck ) const
	{
		checkSlow(!(FlagsToCheck & (RF_MarkAsNative | RF_MarkAsRootSet)) || FlagsToCheck == RF_AllFlags); // These flags can't be used outside of constructors / internal code
		return ((GetFlags() & FlagsToCheck) == FlagsToCheck);
	}

	/**
	 * Returns object flags that are both in the mask and set on the object.
	 *
	 * @param Mask	Mask to mask object flags with
	 * @param Objects flags that are set in both the object and the mask
	 */
	FORCEINLINE EObjectFlags GetMaskedFlags( EObjectFlags Mask = RF_AllFlags ) const
	{
		return EObjectFlags(GetFlags() & Mask);
	}

	/*----------------------------------------------------
			Marks, implemented in UObjectMarks.cpp
	----------------------------------------------------*/

	/**
	 * Adds marks to an object
	 *
	 * @param	Marks	Logical OR of OBJECTMARK_'s to apply 
	 */
	FORCEINLINE void Mark(EObjectMark Marks) const
	{
		MarkObject(this,Marks);
	}

	/**
	 * Removes marks from and object
	 *
	 * @param	Marks	Logical OR of OBJECTMARK_'s to remove 
	 */
	FORCEINLINE void UnMark(EObjectMark Marks) const
	{
		UnMarkObject(this,Marks);
	}

	/**
	 * Tests an object for having ANY of a set of marks
	 *
	 * @param	Marks	Logical OR of OBJECTMARK_'s to test
	 * @return	true if the object has any of the given marks.
	 */
	FORCEINLINE bool HasAnyMarks(EObjectMark Marks) const
	{
		return ObjectHasAnyMarks(this,Marks);
	}

	/**
	 * Tests an object for having ALL of a set of marks
	 *
	 * @param	Marks	Logical OR of OBJECTMARK_'s to test
	 * @return	true if the object has any of the given marks.
	 */
	FORCEINLINE bool HasAllMarks(EObjectMark Marks) const
	{
		return ObjectHasAllMarks(this,Marks);
	}

	/**
	 * Returns all of the object marks on a specific object
	 *
	 * @param	Object	Object to get marks for
	 * @return	all Marks for an object
	 */
	FORCEINLINE EObjectMark GetAllMarks() const
	{
		return ObjectGetAllMarks(this);
	}

	/**
	 * Checks the PendingKill flag to see if it is dead but memory still valid
	 */
	UE_DEPRECATED(5.0, "IsPendingKill() should no longer be used. Use IsValid(Object), IsValidChecked(Object) or GetValid(Object) instead.")
	FORCEINLINE bool IsPendingKill() const
	{
		if (bPendingKillDisabled)
		{
			checkSlow(GUObjectArray.IndexToObject(InternalIndex)->HasAnyFlags(EInternalObjectFlags::Garbage) == HasAnyFlags(RF_InternalGarbage));
			return HasAnyFlags(RF_InternalGarbage);
		}
		else
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			checkSlow(GUObjectArray.IndexToObject(InternalIndex)->HasAnyFlags(EInternalObjectFlags::PendingKill) == HasAnyFlags(RF_InternalPendingKill));
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
			return HasAnyFlags(RF_InternalPendingKill);
		}
	}

	/**
	 * Marks this object as PendingKill.
	 */
	UE_DEPRECATED(5.0, "MarkPendingKill() should no longer be used. Use MarkAsGarbage() which will work just like MarkPendingKill() if Pending Kill support is enabled.")
	FORCEINLINE void MarkPendingKill()
	{
		check(!IsRooted());
		if (bPendingKillDisabled)
		{
			MarkAsGarbageOnlyInternal();
		}
		else
		{
			MarkPendingKillOnlyInternal();
		}
	}

	/**
	 * Unmarks this object as PendingKill.
	 */
	UE_DEPRECATED(5.0, "ClearPendingKill() should no longer be used. Use ClearGarbage() which will work just like ClearPendingKill() if Pending Kill support is enabled.")
	FORCEINLINE void ClearPendingKill()
	{
		if (bPendingKillDisabled)
		{
			ClearGarbageOnlyInternal();
		}
		else
		{
			ClearPendingKillOnlyInternal();
		}
	}

	/**
	 * Marks this object as Garbage.
	 */
	FORCEINLINE void MarkAsGarbage()
	{
		check(!IsRooted());
		if (bPendingKillDisabled)
		{
			MarkAsGarbageOnlyInternal();
		}
		else
		{
			MarkPendingKillOnlyInternal();
		}
	}

	/**
	 * Unmarks this object as Garbage.
	 */
	FORCEINLINE void ClearGarbage()
	{
		if (bPendingKillDisabled)
		{
			ClearGarbageOnlyInternal();
		}
		else
		{
			ClearPendingKillOnlyInternal();
		}
	}

	/**
	 * Add an object to the root set. This prevents the object and all
	 * its descendants from being deleted during garbage collection.
	 */
	FORCEINLINE void AddToRoot()
	{
		GUObjectArray.IndexToObject(InternalIndex)->SetRootSet();
	}

	/** Remove an object from the root set. */
	FORCEINLINE void RemoveFromRoot()
	{
		GUObjectArray.IndexToObject(InternalIndex)->ClearRootSet();
	}

	/**
	 * Returns true if this object is explicitly rooted
	 *
	 * @return true if the object was explicitly added as part of the root set.
	 */
	FORCEINLINE bool IsRooted() const
	{
		return GUObjectArray.IndexToObject(InternalIndex)->IsRootSet();
	}

	 /**
	 * Atomically clear the unreachable flag
	 *
	 * @return true if we are the thread that cleared RF_Unreachable
	 */
	FORCEINLINE bool ThisThreadAtomicallyClearedRFUnreachable()
	{
		return GUObjectArray.IndexToObject(InternalIndex)->ThisThreadAtomicallyClearedRFUnreachable();
	}

	/** Checks if the object is unreachable. */
	FORCEINLINE bool IsUnreachable() const
	{
		return GUObjectArray.IndexToObject(InternalIndex)->IsUnreachable();
	}

	/** Checks if the object is pending kill or unreachable. INTERNAL USE ONLY! If you want to check if your object is valid use IsValid(Object)/IsValidObjectChecked(Object)/GetValid(Object) instead. */
	UE_DEPRECATED(5.0, "IsPendingKillOrUnreachable() should no longer be used. Use IsValid(Object), IsValidChecked(Object), GetValid(Object) and/or IsUnreachable() instead.")
	FORCEINLINE bool IsPendingKillOrUnreachable() const
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return GUObjectArray.IndexToObject(InternalIndex)->HasAnyFlags(EInternalObjectFlags::PendingKill | EInternalObjectFlags::Garbage | EInternalObjectFlags::Unreachable);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	/** Checks if the object is native. */
	FORCEINLINE bool IsNative() const
	{
		return GUObjectArray.IndexToObject(InternalIndex)->HasAnyFlags(EInternalObjectFlags::Native);
	}

	/**
	 * Clears passed in internal flags.
	 *
	 * @param FlagsToClear	Object flags to clear.
	 * @return				true if any of the passed in flags are set, false otherwise  (including no flags passed in).
	 */
	FORCEINLINE void SetInternalFlags(EInternalObjectFlags FlagsToSet) const
	{
		FUObjectItem* ObjectItem = GUObjectArray.IndexToObject(InternalIndex);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		checkf(!(FlagsToSet & (EInternalObjectFlags::PendingKill | EInternalObjectFlags::Garbage)) || int32(FlagsToSet & (EInternalObjectFlags::PendingKill | EInternalObjectFlags::Garbage)) == (ObjectItem->Flags & int32(EInternalObjectFlags::PendingKill | EInternalObjectFlags::Garbage)), TEXT("SetInternalFlags should not set the PendingKill or Garbage flag. Use MarkPendingKill or MarkAsGarbage instead"));
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		ObjectItem->SetFlags(FlagsToSet);
	}

	/**
	 * Gets internal flags.
	 *
	 * @param FlagsToClear	Object flags to clear.
	 * @return				true if any of the passed in flags are set, false otherwise  (including no flags passed in).
	 */
	FORCEINLINE EInternalObjectFlags GetInternalFlags() const
	{
		return GUObjectArray.IndexToObject(InternalIndex)->GetFlags();
	}

	/**
	 * Used to safely check whether any of the passed in internal flags are set.
	 *
	 * @param FlagsToCheck	Object flags to check for.
	 * @return				true if any of the passed in flags are set, false otherwise  (including no flags passed in).
	 */
	FORCEINLINE bool HasAnyInternalFlags(EInternalObjectFlags FlagsToCheck) const
	{
		return GUObjectArray.IndexToObject(InternalIndex)->HasAnyFlags(FlagsToCheck);
	}

	/**
	 * Clears passed in internal flags.
	 *
	 * @param FlagsToClear	Object flags to clear.
	 * @return				true if any of the passed in flags are set, false otherwise  (including no flags passed in).
	 */
	FORCEINLINE void ClearInternalFlags(EInternalObjectFlags FlagsToClear) const
	{
		FUObjectItem* ObjectItem = GUObjectArray.IndexToObject(InternalIndex);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		checkf(!(FlagsToClear & (EInternalObjectFlags::PendingKill | EInternalObjectFlags::Garbage)) || (ObjectItem->Flags & int32(FlagsToClear & (EInternalObjectFlags::PendingKill | EInternalObjectFlags::Garbage))) == 0, TEXT("ClearInternalFlags should not clear PendingKill or Garbage flag. Use ClearGarbage() instead"));
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		ObjectItem->ClearFlags(FlagsToClear);
	}

	/**
	 * Atomically clears passed in internal flags.
	 *
	 * @param FlagsToClear	Object flags to clear.
	 * @return				true if any of the passed in flags are set, false otherwise  (including no flags passed in).
	 */
	FORCEINLINE bool AtomicallyClearInternalFlags(EInternalObjectFlags FlagsToClear) const
	{
		FUObjectItem* ObjectItem = GUObjectArray.IndexToObject(InternalIndex);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		checkf((ObjectItem->Flags & int32(FlagsToClear & (EInternalObjectFlags::PendingKill | EInternalObjectFlags::Garbage))) == 0, TEXT("ClearInternalFlags should not clear PendingKill or Garbage flag. Use ClearGarbage() instead"));
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		return ObjectItem->ThisThreadAtomicallyClearedFlag(FlagsToClear);
	}


	/*-------------------
			Names
	-------------------*/

	/**
	 * Returns the fully qualified pathname for this object as well as the name of the class, in the format:
	 * 'ClassName Outermost[.Outer].Name'.
	 *
	 * @param	StopOuter	if specified, indicates that the output string should be relative to this object.  if StopOuter
	 *						does not exist in this object's Outer chain, the result would be the same as passing NULL.
	 * @param	Flags		flags that control the behavior of full name generation
	 *
	 * @note	safe to call on NULL object pointers!
	 */
	FString GetFullName( const UObject* StopOuter=NULL, EObjectFullNameFlags Flags = EObjectFullNameFlags::None ) const;

	/**
	 * Version of GetFullName() that eliminates unnecessary copies.
	 */
	void GetFullName( const UObject* StopOuter, FString& ResultString, EObjectFullNameFlags Flags = EObjectFullNameFlags::None ) const;

	/**
	 * Returns the fully qualified pathname for this object, in the format:
	 * 'Outermost[.Outer].Name'
	 *
	 * @param	StopOuter	if specified, indicates that the output string should be relative to this object.  if StopOuter
	 *						does not exist in this object's Outer chain, the result would be the same as passing NULL.
	 *
	 * @note	safe to call on NULL object pointers!
	 */
	FString GetPathName( const UObject* StopOuter=NULL ) const;

	/**
	 * Versions of GetPathName() that eliminates unnecessary copies and allocations.
	 */
	void GetPathName(const UObject* StopOuter, FString& ResultString) const;
	void GetPathName(const UObject* StopOuter, FStringBuilderBase& ResultString) const;

	/** Helper function to access the private bPendingKillDisabled variable */
	static inline bool IsPendingKillEnabled()
	{
		return !bPendingKillDisabled;
	}

	/** Helper function that sets the appropriate flag based on PK being enabled or not */
	FORCEINLINE static EInternalObjectFlags FixGarbageOrPendingKillInternalObjectFlags(const EInternalObjectFlags InFlags)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		if (!(InFlags & (EInternalObjectFlags::Garbage | EInternalObjectFlags::PendingKill)))
		{
			// Pass through
			return InFlags;
		}
		else
		{
			return bPendingKillDisabled ?
				((InFlags & ~EInternalObjectFlags::PendingKill) | EInternalObjectFlags::Garbage) : // Replace PK with Garbage
				((InFlags & ~EInternalObjectFlags::Garbage) | EInternalObjectFlags::PendingKill); // Replace Garbage with PK
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

public:
	/**
	* Called after load to determine if the object can be a cluster root
	*
	* @return	true if this object can be a cluster root
	*/
	virtual bool CanBeClusterRoot() const
	{
		return false;
	}

	/**
	* Called during cluster construction if the object can be added to a cluster
	*
	* @return	true if this object can be inside of a cluster
	*/
	virtual bool CanBeInCluster() const;

	/**
	* Called after PostLoad to create UObject cluster
	*/
	virtual void CreateCluster();

	/**
	* Called during Garbage Collection to perform additional cleanup when the cluster is about to be destroyed due to PendingKill flag being set on it.
	*/
	virtual void OnClusterMarkedAsPendingKill() {}

	/**
	* Adds this objects to a GC cluster that already exists
	* @param ClusterRootOrObjectFromCluster Object that belongs to the cluster we want to add this object to.
	* @param Add this object to the target cluster as a mutable object without adding this object's references.
	*/
	void AddToCluster(UObjectBaseUtility* ClusterRootOrObjectFromCluster, bool bAddAsMutableObject = false);

protected:
	/** Helper function to create a cluster from UObject */
	static void CreateClusterFromObject(UObjectBaseUtility* ClusterRootObject, UObjectBaseUtility* ReferencingObject);

public:
	/**
	 * Walks up the chain of packages until it reaches the top level, which it ignores.
	 *
	 * @param	bStartWithOuter		whether to include this object's name in the returned string
	 * @return	string containing the path name for this object, minus the outermost-package's name
	 */
	FString GetFullGroupName( bool bStartWithOuter ) const;

	/**
	 * Returns the name of this object (with no path information)
	 * 
	 * @return Name of the object.
	 */
	FORCEINLINE FString GetName() const
	{
		return GetFName().ToString();
	}

	/** Optimized version of GetName that overwrites an existing string */
	FORCEINLINE void GetName(FString &ResultString) const
	{
		GetFName().ToString(ResultString);
	}
	/** Optimized version of GetName that appends to an existing string */
	FORCEINLINE void AppendName(FString& ResultString) const
	{
		GetFName().AppendString(ResultString);
	}


	/*-------------------
		Outer & Package
	-------------------*/

	/**
	 * Get the object packaging mode.
	 * @return true if object has a different package than its outer's package
	 */
	bool IsPackageExternal() const;

	/**
	 * Utility function to temporarily detach the object external package, if any
	 * GetPackage will report the outer's package once detached
	 */
	void DetachExternalPackage();

	/**
	 * Utility function to reattach the object external package, if any
	 * GetPackage will report the object external package if set after this call
	 */
	void ReattachExternalPackage();

	/**
	 * Walks up the list of outers until it finds the top-level one that isn't a package.
	 * Will return null if called on a package
	 * @return outermost non package Outer.
	 */
	UObject* GetOutermostObject() const;

	/**
	 * Walks up the list of outers until it finds a package directly associated with the object.
	 *
	 * @return the package the object is in.
	 */
	UPackage* GetPackage() const;

	/** 
	 * Legacy function, has the same behavior as GetPackage
	 * use GetPackage instead.
	 * @return the package the object is in.
	 * @see GetPackage
	 */
	UPackage* GetOutermost() const;

	/** 
	 * Finds the outermost package and marks it dirty. 
	 * The editor suppresses this behavior during load as it is against policy to dirty packages simply by loading them.
	 *
	 * @return false if the request to mark the package dirty was suppressed by the editor and true otherwise.
	 */
	bool MarkPackageDirty() const;

	/**
	* Determines whether this object is a template object
	*
	* @return	true if this object is a template object (owned by a UClass)
	*/
	bool IsTemplate( EObjectFlags TemplateTypes = RF_ArchetypeObject|RF_ClassDefaultObject ) const;

	/**
	 * Traverses the outer chain searching for the next object of a certain type.  (T must be derived from UObject)
	 *
	 * @param	Target class to search for
	 * @return	a pointer to the first object in this object's Outer chain which is of the correct type.
	 */
	UObject* GetTypedOuter(UClass* Target) const;

	/**
	 * Traverses the outer chain searching for the next object of a certain type.  (T must be derived from UObject)
	 *
	 * @return	a pointer to the first object in this object's Outer chain which is of the correct type.
	 */
	template<typename T>
	T* GetTypedOuter() const
	{
		return (T *)GetTypedOuter(T::StaticClass());
	}

	/** 
	 * Return the dispatch to `IsInOuter` or `IsInPackage` depending on SomeOuter's class. 
	 * Legacy function, preferably use IsInOuter or IsInPackage depending on use case.
	 */
	bool IsIn( const UObject* SomeOuter ) const;

	/** 
	 * Overload to determine if an object is in the specified package which can now be different than its outer chain.
	 * Calls IsInPackage.
	 */
	bool IsIn(const UPackage* SomePackage) const;

	/** Returns true if the object is contained in the specified outer. */
	bool IsInOuter(const UObject* SomeOuter) const;

	/** Returns true if the object is contained in the specified package. */
	bool IsInPackage(const UPackage* SomePackage) const;

	/**
	 * Find out if this object is inside (has an outer) that is of the specified class
	 * @param SomeBaseClass	The base class to compare against
	 * @return True if this object is in an object of the given type.
	 */
	bool IsInA( const UClass* SomeBaseClass ) const;

	/**
	 * Checks whether this object's top-most package has any of the specified flags
	 *
	 * @param	CheckFlagMask	a bitmask of EPackageFlags values to check for
	 *
	 * @return	true if the PackageFlags member of this object's top-package has any bits from the mask set.
	 */
	bool RootPackageHasAnyFlags( uint32 CheckFlagMask ) const;


	/*-------------------
			Class
	-------------------*/

private:
	template <typename ClassType>
	static FORCEINLINE bool IsChildOfWorkaround(const ClassType* ObjClass, const ClassType* TestCls)
	{
		return ObjClass->IsChildOf(TestCls);
	}

public:
	/** Returns true if this object is of the specified type. */
	template <typename OtherClassType>
	FORCEINLINE bool IsA( OtherClassType SomeBase ) const
	{
		// We have a cyclic dependency between UObjectBaseUtility and UClass,
		// so we use a template to allow inlining of something we haven't yet seen, because it delays compilation until the function is called.

		// 'static_assert' that this thing is actually a UClass pointer or convertible to it.
		const UClass* SomeBaseClass = SomeBase;
		(void)SomeBaseClass;
		checkfSlow(SomeBaseClass, TEXT("IsA(NULL) cannot yield meaningful results"));

		const UClass* ThisClass = GetClass();

		// Stop the compiler doing some unnecessary branching for nullptr checks
		UE_ASSUME(SomeBaseClass);
		UE_ASSUME(ThisClass);

		return IsChildOfWorkaround(ThisClass, SomeBaseClass);
	}

	/** Returns true if this object is of the template type. */
	template<class T>
	bool IsA() const
	{
		return IsA(T::StaticClass());
	}

	/**
	 * Finds the most-derived class which is a parent of both TestClass and this object's class.
	 *
	 * @param	TestClass	the class to find the common base for
	 */
	const UClass* FindNearestCommonBaseClass( const UClass* TestClass ) const;

	/**
	 * Returns a pointer to this object safely converted to a pointer of the specified interface class.
	 *
	 * @param	InterfaceClass	the interface class to use for the returned type
	 *
	 * @return	a pointer that can be assigned to a variable of the interface type specified, or NULL if this object's
	 *			class doesn't implement the interface indicated.  Will be the same value as 'this' if the interface class
	 *			isn't native.
	 */
	void* GetInterfaceAddress( UClass* InterfaceClass );

	/** 
	 *	Returns a pointer to the I* native interface object that this object implements.
	 *	Returns NULL if this object does not implement InterfaceClass, or does not do so natively.
	 */
	void* GetNativeInterfaceAddress(UClass* InterfaceClass);

	/** 
	 *	Returns a pointer to the const I* native interface object that this object implements.
	 *	Returns NULL if this object does not implement InterfaceClass, or does not do so natively.
	 */
	const void* GetNativeInterfaceAddress(UClass* InterfaceClass) const
	{
		return const_cast<UObjectBaseUtility*>(this)->GetNativeInterfaceAddress(InterfaceClass);
	}

	/**
	 * Returns whether this component was instanced from a component/subobject template, or if it is a component/subobject template.
	 * This is based on a name comparison with the outer class instance lookup table
	 *
	 * @return	true if this component was instanced from a template.  false if this component was created manually at runtime.
	 */
	bool IsDefaultSubobject() const;
	

	/*--------------------------------------------------
			Linker, defined in UObjectLinker.cpp
	--------------------------------------------------*/

	/**
	 * Returns the linker for this object.
	 *
	 * @return	a pointer to the linker for this object, or NULL if this object has no linker
	 */
	class FLinkerLoad* GetLinker() const;

	/**
	 * Returns this object's LinkerIndex.
	 *
	 * @return	the index into my linker's ExportMap for the FObjectExport
	 *			corresponding to this object.
	 */
	int32 GetLinkerIndex() const;

	/**
	 * Returns the UE version of the linker for this object.
	 *
	 * @return	the UE version of the engine's package file when this object
	 *			was last saved, or GPackageFileUEVersion (current version) if
	 *			this object does not have a linker, which indicates that
	 *			a) this object is a native only class, or
	 *			b) this object's linker has been detached, in which case it is already fully loaded
	 */
	FPackageFileVersion GetLinkerUEVersion() const;
		
	UE_DEPRECATED(5.0, "Use GetLinkerUEVersion instead which returns the version as a FPackageFileVersion. See the @FPackageFileVersion documentation for further details")
	inline int32 GetLinkerUE4Version() const 
	{ 
		// Existing code calling GetLinkerUE4Version will be testing against UE4 version numbers so 
		// we can just return the UE4 version number.
		// All new code that might actually need the UE5 version as well should be calling ::GetLinkerUEVersion
		return GetLinkerUEVersion().FileVersionUE4;
	}

	/**
	 * Returns the licensee version of the linker for this object.
	 *
	 * @return	the licensee version of the engine's package file when this object
	 *			was last saved, or GPackageFileLicenseeVersion (current version) if
	 *			this object does not have a linker, which indicates that
	 *			a) this object is a native only class, or
	 *			b) this object's linker has been detached, in which case it is already fully loaded
	 */
	int32 GetLinkerLicenseeUEVersion() const;

	UE_DEPRECATED(5.0, "Use GetLinkerLicenseeUEVersion instead")
	inline int32 GetLinkerLicenseeUE4Version() const { return GetLinkerLicenseeUEVersion(); }


	/**
	 * Returns the custom version of the linker for this object corresponding to the given custom version key.
	 *
	 * @return	the custom version of the engine's package file when this object
	 *			was last saved, or the current version if
	 *			this object does not have a linker, which indicates that
	 *			a) this object is a native only class, or
	 *			b) this object's linker has been detached, in which case it is already fully loaded
	 */
	int32 GetLinkerCustomVersion(FGuid CustomVersionKey) const;

	/** 
	 * Overloaded < operator. Compares objects by name.
	 *
	 * @return true if this object's name is lexicographically smaller than the other object's name
	 */
	FORCEINLINE bool operator<( const UObjectBaseUtility& Other ) const
	{
		return GetName() < Other.GetName();
	}



	/*******
	 * Stats
	 *******/
#if STATS || ENABLE_STATNAMEDEVENTS_UOBJECT
	FORCEINLINE void ResetStatID()
	{
		GUObjectArray.IndexToObject(InternalIndex)->StatID = TStatId();
#if ENABLE_STATNAMEDEVENTS_UOBJECT
		GUObjectArray.IndexToObject(InternalIndex)->StatIDStringStorage = nullptr;
#endif
	}
#endif
	/**
	  * Returns the stat ID of the object, used for profiling. This will create a stat ID if needed.
	  *
	  * @param bForDeferred If true, a stat ID will be created even if a group is disabled
	  */
	FORCEINLINE TStatId GetStatID(bool bForDeferredUse = false) const
	{
#if STATS
		const TStatId& StatID = GUObjectArray.IndexToObject(InternalIndex)->StatID;

		// this is done to avoid even registering stats for a disabled group (unless we plan on using it later)
		if (bForDeferredUse || FThreadStats::IsCollectingData(GET_STATID(STAT_UObjectsStatGroupTester)))
		{
			if (!StatID.IsValidStat())
			{
				CreateStatID();
			}
			return StatID;
		}
#elif ENABLE_STATNAMEDEVENTS_UOBJECT
		const TStatId& StatID = GUObjectArray.IndexToObject(InternalIndex)->StatID;
		if (!StatID.IsValidStat() && (bForDeferredUse || GCycleStatsShouldEmitNamedEvents))
		{
			CreateStatID();
		}
		return StatID;
#endif // STATS
		return TStatId(); // not doing stats at the moment, or ever
	}

private:
#if STATS || ENABLE_STATNAMEDEVENTS_UOBJECT
	/** Creates a stat ID for this object */
	void CreateStatID() const
	{
		GUObjectArray.IndexToObject(InternalIndex)->CreateStatID();
	}
#endif

};

/** Returns false if this pointer cannot be a valid pointer to a UObject */
FORCEINLINE bool IsPossiblyAllocatedUObjectPointer(UObject* Ptr)
{
	auto CountByteValues = [](UPTRINT Val, UPTRINT ByteVal) -> int32
	{
		int32 Result = 0;

		for (int32 I = 0; I != sizeof(UPTRINT); ++I)
		{
			if ((Val & 0xFF) == ByteVal)
			{
				++Result;
			}
			Val >>= 8;
		}

		return Result;
	};

	UPTRINT PtrVal = (UPTRINT)Ptr;
	return PtrVal >= 0x1000 && CountByteValues(PtrVal, 0xCD) < sizeof(UPTRINT) / 2;
}

/**
 * Returns the name of this object (with no path information)
 * @param Object object to retrieve the name for; NULL gives "None"
 * @return Name of the object.
*/
FORCEINLINE FString GetNameSafe(const UObjectBaseUtility *Object)
{
	if( Object == NULL )
	{
		return TEXT("None");
	}
	else
	{
		return Object->GetName();
	}
}

/**
 * Returns the path name of this object
 * @param Object object to retrieve the path name for; NULL gives "None"
 * @return path name of the object.
*/
FORCEINLINE FString GetPathNameSafe(const UObjectBaseUtility *Object)
{
	if( Object == NULL )
	{
		return TEXT("None");
	}
	else
	{
		return Object->GetPathName();
	}
}

/**
 * Returns the full name of this object
 * @param Object object to retrieve the full name for; NULL (or a null class!) gives "None"
 * @return full name of the object.
*/
FORCEINLINE FString GetFullNameSafe(const UObjectBaseUtility *Object)
{
	if( !Object || !Object->GetClass())
	{
		return TEXT("None");
	}
	else
	{
		return Object->GetFullName();
	}
}

/**
 *	Returns the native (C++) parent class of the supplied class
 *	If supplied class is native, it will be returned.
 */
COREUOBJECT_API UClass* GetParentNativeClass(UClass* Class);

#if !defined(USE_LIGHTWEIGHT_UOBJECT_STATS_FOR_HITCH_DETECTION)
#define USE_LIGHTWEIGHT_UOBJECT_STATS_FOR_HITCH_DETECTION (1)
#endif

#if STATS

/** Structure used to track time spent by a UObject */
class FScopeCycleCounterUObject : public FCycleCounter
{
public:
#if USE_MALLOC_PROFILER
	/** Package path being tracked */
	FName PackageTag;
	/** Class path being tracked */
	FName ClassTag;
	/** Object path being tracked */
	FName ObjectTag;
#endif

	/**
	 * Constructor, starts timing
	 */
	FORCEINLINE_STATS FScopeCycleCounterUObject(const UObjectBaseUtility *Object)
	{
		if (Object)
		{
			bool bStarted = false;
			TStatId ObjectStatId = Object->GetStatID();
			if (FThreadStats::IsCollectingData(ObjectStatId))
			{
				Start(ObjectStatId);
				bStarted = true;
			}

#if CPUPROFILERTRACE_ENABLED
			if (!bStarted && UE_TRACE_CHANNELEXPR_IS_ENABLED(CpuChannel))
			{
				StartTrace(Object->GetFName());
			}
#endif
		}

#if USE_MALLOC_PROFILER
		if (Object)
		{
			TrackObjectForMallocProfiling(Object);
		}
#endif
	}
	/**
	 * Constructor, starts timing with an alternate enable stat to use high performance disable for only SOME UObject stats
	 */
	FORCEINLINE_STATS FScopeCycleCounterUObject(const UObjectBaseUtility *Object, TStatId OtherStat)
	{
		if (Object)
		{
			bool bStarted = false;
			if (FThreadStats::IsCollectingData(OtherStat))
			{
				TStatId ObjectStatId = Object->GetStatID();
				if (!ObjectStatId.IsNone())
				{
					Start(ObjectStatId);
					bStarted = true;
				}
			}

#if CPUPROFILERTRACE_ENABLED
			if (!bStarted && UE_TRACE_CHANNELEXPR_IS_ENABLED(CpuChannel))
			{
				StartTrace(Object->GetFName());
			}
#endif
		}

#if USE_MALLOC_PROFILER
		if (Object)
		{
			TrackObjectForMallocProfiling(Object);
		}
#endif
	}

	/**
	 * Updates the stat with the time spent
	 */
	FORCEINLINE_STATS ~FScopeCycleCounterUObject()
	{
		Stop();

#if USE_MALLOC_PROFILER
		UntrackObjectForMallocProfiling();
#endif
	}

#if USE_MALLOC_PROFILER
	COREUOBJECT_API void TrackObjectForMallocProfiling(const UObjectBaseUtility *InObject);
	COREUOBJECT_API void TrackObjectForMallocProfiling(const FName InPackageName, const FName InClassName, const FName InObjectName);
	COREUOBJECT_API void UntrackObjectForMallocProfiling();
#endif
};

/** Declares a scope cycle counter for a specific object with a Name context */
#define SCOPE_CYCLE_UOBJECT(Name, Object) \
	FScopeCycleCounterUObject ObjCycleCount_##Name(Object);

#elif ENABLE_STATNAMEDEVENTS

class FScopeCycleCounterUObject
{
public:
	FScopeCycleCounter ScopeCycleCounter;
#if ENABLE_STATNAMEDEVENTS_UOBJECT && CPUPROFILERTRACE_ENABLED
	bool bPop;
#endif

	FORCEINLINE_STATS FScopeCycleCounterUObject(const UObjectBaseUtility *Object)
		: ScopeCycleCounter(Object ? Object->GetStatID().StatString : nullptr)
#if ENABLE_STATNAMEDEVENTS_UOBJECT && CPUPROFILERTRACE_ENABLED
		, bPop(false)
#endif
	{
#if ENABLE_STATNAMEDEVENTS_UOBJECT && CPUPROFILERTRACE_ENABLED
		if (GCycleStatsShouldEmitNamedEvents && UE_TRACE_CHANNELEXPR_IS_ENABLED(CpuChannel) && Object)
		{
			const TStatId ObjectStatId = Object->GetStatID();
			if (ObjectStatId.IsValidStat())
			{
				bPop = true;
				FCpuProfilerTrace::OutputBeginDynamicEvent(ObjectStatId.StatString);
			}
		}
#endif
	}

	FORCEINLINE_STATS FScopeCycleCounterUObject(const UObjectBaseUtility *Object, TStatId OtherStat)
		: FScopeCycleCounterUObject(Object)
	{
	}

	FORCEINLINE_STATS ~FScopeCycleCounterUObject()
	{
#if ENABLE_STATNAMEDEVENTS_UOBJECT && CPUPROFILERTRACE_ENABLED
		if (bPop)
		{
			FCpuProfilerTrace::OutputEndEvent();
		}
#endif
	}
};

/** Declares a scope cycle counter for a specific object with a Name context */
#define SCOPE_CYCLE_UOBJECT(Name, Object) \
	FScopeCycleCounterUObject ObjCycleCount_##Name(Object);
#elif USE_LIGHTWEIGHT_STATS_FOR_HITCH_DETECTION && USE_HITCH_DETECTION && USE_LIGHTWEIGHT_UOBJECT_STATS_FOR_HITCH_DETECTION
extern CORE_API TSAN_ATOMIC(bool) GHitchDetected;

class FScopeCycleCounterUObject
{
	const UObject* StatObject;
public:
	FORCEINLINE  FScopeCycleCounterUObject(const UObject* InStatObject, TStatId OtherStat = TStatId())
	{
		StatObject = GHitchDetected ? nullptr : InStatObject;
	}

	FORCEINLINE ~FScopeCycleCounterUObject()
	{
		if (GHitchDetected &&  StatObject)
		{
			ReportHitch();
		}
	}

	COREUOBJECT_API void ReportHitch();
};

/** Declares a scope cycle counter for a specific object with a Name context */
#define SCOPE_CYCLE_UOBJECT(Name, Object) \
	FScopeCycleCounterUObject ObjCycleCount_##Name(Object);
#else
class FScopeCycleCounterUObject
{
public:
	FORCEINLINE_STATS FScopeCycleCounterUObject(const UObjectBaseUtility *Object)
	{
	}
	FORCEINLINE_STATS FScopeCycleCounterUObject(const UObjectBaseUtility *Object, TStatId OtherStat)
	{
	}
};

#define SCOPE_CYCLE_UOBJECT(Name, Object)

#endif

#if defined(_MSC_VER) && _MSC_VER == 1900
	#ifdef PRAGMA_ENABLE_SHADOW_VARIABLE_WARNINGS
		PRAGMA_ENABLE_SHADOW_VARIABLE_WARNINGS
	#endif
#endif
