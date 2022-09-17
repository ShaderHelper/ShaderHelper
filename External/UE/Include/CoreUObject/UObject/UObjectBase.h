// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UObjectBase.h: Base class for UObject, defines low level functionality
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "HAL/LowLevelMemTracker.h"

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("STAT_UObjectsStatGroupTester"), STAT_UObjectsStatGroupTester, STATGROUP_UObjects, COREUOBJECT_API);

/** 
 * Low level implementation of UObject, should not be used directly in game code 
 */
class COREUOBJECT_API UObjectBase
{
	friend class UObjectBaseUtility;
	friend struct Z_Construct_UClass_UObject_Statics;
	friend class FUObjectArray; // for access to InternalIndex without revealing it to anyone else
	friend class FUObjectAllocator; // for access to destructor without revealing it to anyone else
	friend struct FInternalUObjectBaseUtilityIsValidFlagsChecker; // for access to InternalIndex
	friend COREUOBJECT_API void UObjectForceRegistration(UObjectBase* Object, bool bCheckForModuleRelease);
	friend COREUOBJECT_API void InitializePrivateStaticClass(
		class UClass* TClass_Super_StaticClass,
		class UClass* TClass_PrivateStaticClass,
		class UClass* TClass_WithinClass_StaticClass,
		const TCHAR* PackageName,
		const TCHAR* Name
		);
protected:
	UObjectBase() :
		 NamePrivate(NoInit)  // screwy, but the name was already set and we don't want to set it again
	{
	}

	/**
	 * Constructor used for bootstrapping
	 * @param	InFlags			RF_Flags to assign
	 */
	UObjectBase( EObjectFlags InFlags );
public:

	/**
	 * Constructor used by StaticAllocateObject
	 * @param	InClass				non NULL, this gives the class of the new object, if known at this time
	 * @param	InFlags				RF_Flags to assign
	 * @param	InInternalFlags EInternalObjectFlags to assign
	 * @param	InOuter				outer for this object
	 * @param	InName				name of the new object
	 */
	UObjectBase( UClass* InClass, EObjectFlags InFlags, EInternalObjectFlags InInternalFlags, UObject *InOuter, FName InName );

	/**
	 * Final destructor, removes the object from the object array, and indirectly, from any annotations
	 **/
	virtual ~UObjectBase();

	/**
	 * Emit GC tokens for UObjectBase, this might be UObject::StaticClass or Default__Class
	 **/
	static void EmitBaseReferences(UClass *RootClass);

protected:
	/**
	 * Just change the FName and Outer and rehash into name hash tables. For use by higher level rename functions.
	 *
	 * @param NewName	new name for this object
	 * @param NewOuter	new outer for this object, if NULL, outer will be unchanged
	 */
	void LowLevelRename(FName NewName,UObject *NewOuter = NULL);

	/** Force any base classes to be registered first */
	virtual void RegisterDependencies() {}

	/** Enqueue the registration for this object. */
	void Register(const TCHAR* PackageName,const TCHAR* Name);
	
	/**
	 * Convert a boot-strap registered class into a real one, add to uobject array, etc
	 *
	 * @param UClassStaticClass Now that it is known, fill in UClass::StaticClass() as the class
	 */
	virtual void DeferredRegister(UClass *UClassStaticClass,const TCHAR* PackageName,const TCHAR* Name);

private:
	/**
	 * Add a newly created object to the name hash tables and the object array
	 *
	 * @param Name name to assign to this uobject
	 * @param InSetInternalFlags Internal object flags to be set on the object once it's been added to the array
	 */
	void AddObject(FName Name, EInternalObjectFlags InSetInternalFlags);

public:
	/**
	 * Checks to see if the object appears to be valid
	 * @return true if this appears to be a valid object
	 */
	bool IsValidLowLevel() const;

	/**
	 * Faster version of IsValidLowLevel.
	 * Checks to see if the object appears to be valid by checking pointers and their alignment.
	 * Name and InternalIndex checks are less accurate than IsValidLowLevel.
	 * @param bRecursive true if the Class pointer should be checked with IsValidLowLevelFast
	 * @return true if this appears to be a valid object
	 */
	bool IsValidLowLevelFast(bool bRecursive = true) const;

	/** 
	 * Returns the unique ID of the object...these are reused so it is only unique while the object is alive.
	 * Useful as a tag.
	**/
	FORCEINLINE uint32 GetUniqueID() const
	{
		return (uint32)InternalIndex;
	}

	/** Returns the UClass that defines the fields of this object */
	FORCEINLINE UClass* GetClass() const
	{
		return ClassPrivate;
	}
	
	/** Returns the UObject this object resides in */
	FORCEINLINE UObject* GetOuter() const
	{
		return OuterPrivate;
	}

	/** Returns the logical name of this object */
	FORCEINLINE FName GetFName() const
	{
		return NamePrivate;
	}

	/** Removes the class prefix from the given string */
	static FString RemoveClassPrefix(const TCHAR* ClassName);

	/** Returns the external UPackage associated with this object, if any */
	UPackage* GetExternalPackage() const;
	
	/** Associate an external package directly to this object. */
	void SetExternalPackage(UPackage* InPackage);

	/** Returns the external UPackage for this object, if any, NOT THREAD SAFE, used by internal gc reference collecting. */
	UPackage* GetExternalPackageInternal() const;

protected:
	/**
	 * Set the object flags directly
	 *
	 **/
	FORCEINLINE void SetFlagsTo( EObjectFlags NewFlags )
	{
		checkfSlow((NewFlags & ~RF_AllFlags) == 0, TEXT("%s flagged as 0x%x but is trying to set flags to RF_AllFlags"), *GetFName().ToString(), (int)ObjectFlags);
		ObjectFlags = NewFlags;
	}

public:
	/**
	 * Retrieve the object flags directly
	 *
	 * @return Flags for this object
	 **/
	FORCEINLINE EObjectFlags GetFlags() const
	{
		checkfSlow((ObjectFlags & ~RF_AllFlags) == 0, TEXT("%s flagged as RF_AllFlags"), *GetFName().ToString());
		return ObjectFlags;
	}

	/**
	 *	Atomically adds the specified flags.
	 *	Do not use unless you know what you are doing.
	 *	Designed to be used only by parallel GC and UObject loading thread.
	 */
	FORCENOINLINE void AtomicallySetFlags( EObjectFlags FlagsToAdd )
	{
		int32 OldFlags = 0;
		int32 NewFlags = 0;
		do 
		{
			OldFlags = ObjectFlags;
			NewFlags = OldFlags | FlagsToAdd;
		}
		while( FPlatformAtomics::InterlockedCompareExchange( (int32*)&ObjectFlags, NewFlags, OldFlags) != OldFlags );
	}

	/**
	 *	Atomically clears the specified flags.
	 *	Do not use unless you know what you are doing.
	 *	Designed to be used only by parallel GC and UObject loading thread.
	 */
	FORCENOINLINE void AtomicallyClearFlags( EObjectFlags FlagsToClear )
	{
		int32 OldFlags = 0;
		int32 NewFlags = 0;
		do 
		{
			OldFlags = ObjectFlags;
			NewFlags = OldFlags & ~FlagsToClear;
		}
		while( FPlatformAtomics::InterlockedCompareExchange( (int32*)&ObjectFlags, NewFlags, OldFlags) != OldFlags );
	}

private:

	/** Flags used to track and report various object states. This needs to be 8 byte aligned on 32-bit
	    platforms to reduce memory waste */
	EObjectFlags					ObjectFlags;

	/** Index into GObjectArray...very private. */
	int32							InternalIndex;

	/** Class the object belongs to. */
	UClass*							ClassPrivate;

	/** Name of this object */
	FName							NamePrivate;

	/** Object this object resides in. */
	UObject*						OuterPrivate;
	
	friend class FBlueprintCompileReinstancer;
	friend class FContextObjectManager;

#if WITH_EDITOR
	/** This is used by the reinstancer to re-class and re-archetype the current instances of a class before recompiling */
	void SetClass(UClass* NewClass);
#endif

#if HACK_HEADER_GENERATOR
	// Required by UHT makefiles for internal data serialization.
	friend struct FObjectBaseArchiveProxy;
#endif // HACK_HEADER_GENERATOR
};

/**
 * Checks to see if the UObject subsystem is fully bootstrapped and ready to go.
 * If true, then all objects are registered and auto registration of natives is over, forever.
 *
 * @return true if the UObject subsystem is initialized.
 */
COREUOBJECT_API bool UObjectInitialized();

/**
 * Force a pending registrant to register now instead of in the natural order
 */
COREUOBJECT_API void UObjectForceRegistration(UObjectBase* Object, bool bCheckForModuleRelease = true);

/**
 * Structure that represents the registration information for a given class, structure, or enumeration
 */
template <typename T, typename V>
struct TRegistrationInfo
{
	using TType = T;
	using TVersion = V;

	TType* InnerSingleton = nullptr;
	TType* OuterSingleton = nullptr;
	TVersion ReloadVersionInfo;
};

/**
 * Helper class to perform registration of object information.  It blindly forwards a call to RegisterCompiledInInfo
 */
struct FRegisterCompiledInInfo
{
	template <typename ... Args>
	FRegisterCompiledInInfo(Args&& ... args)
	{
		RegisterCompiledInInfo(std::forward<Args>(args)...);
	}
};

/**
 * Reload version information for classes
 */
struct FClassReloadVersionInfo
{
#if WITH_RELOAD
	SIZE_T Size = 0;
	uint32 Hash = 0;
#endif
};

/**
 * Registration information for classes
 */
using FClassRegistrationInfo = TRegistrationInfo<UClass, FClassReloadVersionInfo>;

/**
 * Composite class register compiled in info
 */
struct FClassRegisterCompiledInInfo
{
	class UClass* (*OuterRegister)();
	class UClass* (*InnerRegister)();
	const TCHAR* Name;
	FClassRegistrationInfo* Info;
	FClassReloadVersionInfo VersionInfo;
};

/**
 * Adds a class registration and version information. The InInfo parameter must be static.
 */
COREUOBJECT_API void RegisterCompiledInInfo(class UClass* (*InOuterRegister)(), class UClass* (*InInnerRegister)(), const TCHAR* InPackageName, const TCHAR* InName, FClassRegistrationInfo& InInfo, const FClassReloadVersionInfo& InVersionInfo);

/**
 * Reload version information for structures
 */
struct FStructReloadVersionInfo
{
#if WITH_RELOAD
	SIZE_T Size = 0;
	uint32 Hash = 0;
#endif
};

/**
 * Registration information for structures
 */
using FStructRegistrationInfo = TRegistrationInfo<UScriptStruct, FStructReloadVersionInfo>;

/**
 * Composite structures register compiled in info
 */
struct FStructRegisterCompiledInInfo
{
	class UScriptStruct* (*OuterRegister)();
	void* (*CreateCppStructOps)();
	const TCHAR* Name;
	FStructRegistrationInfo* Info;
	FStructReloadVersionInfo VersionInfo;
};

/**
 * Adds a struct registration and version information. The InInfo parameter must be static.
 */
COREUOBJECT_API void RegisterCompiledInInfo(class UScriptStruct* (*InOuterRegister)(), const TCHAR* InPackageName, const TCHAR* InName, FStructRegistrationInfo& InInfo, const FStructReloadVersionInfo& InVersionInfo);

/**
 * Invoke the registration method wrapped in notifications.
 */
COREUOBJECT_API class UScriptStruct* GetStaticStruct(class UScriptStruct* (*InRegister)(), UObject* StructOuter, const TCHAR* StructName);

UE_DEPRECATED(5.0, "GetStaticStruct with size and hash has been deprecated, use the version without the size and hash.")
inline class UScriptStruct* GetStaticStruct(class UScriptStruct* (*InRegister)(), UObject* StructOuter, const TCHAR* StructName, SIZE_T Size, uint32 Hash)
{
	return GetStaticStruct(InRegister, StructOuter, StructName);
}

/**
 * Reload version information for enumerations
 */
struct FEnumReloadVersionInfo
{
#if WITH_RELOAD
	uint32 Hash = 0;
#endif
};

/**
 * Registration information for enums
 */
using FEnumRegistrationInfo = TRegistrationInfo<UEnum, FEnumReloadVersionInfo>;

/**
 * Composite enumeration register compiled in info
 */
struct FEnumRegisterCompiledInInfo
{
	class UEnum* (*OuterRegister)();
	const TCHAR* Name;
	FEnumRegistrationInfo* Info;
	FEnumReloadVersionInfo VersionInfo;
};

/**
 * Adds a static enum registration and version information. The InInfo parameter must be static.
 */
COREUOBJECT_API void RegisterCompiledInInfo(class UEnum* (*InOuterRegister)(), const TCHAR* InPackageName, const TCHAR* InName, FEnumRegistrationInfo& InInfo, const FEnumReloadVersionInfo& InVersionInfo);

/**
 * Invoke the registration method wrapped in notifications.
 */
COREUOBJECT_API class UEnum* GetStaticEnum(class UEnum* (*InRegister)(), UObject* EnumOuter, const TCHAR* EnumName);

/**
 * Reload version information for packages 
 */
struct FPackageReloadVersionInfo
{
#if WITH_RELOAD
	uint32 BodyHash = 0;
	uint32 DeclarationsHash = 0;
#endif
};

/**
 * Registration information for packages
 */
using FPackageRegistrationInfo = TRegistrationInfo<UPackage, FPackageReloadVersionInfo>;

/**
 * Adds a static package registration and version information. The InInfo parameter must be static.
 */
COREUOBJECT_API void RegisterCompiledInInfo(UPackage* (*InOuterRegister)(), const TCHAR* InPackageName, FPackageRegistrationInfo& InInfo, const FPackageReloadVersionInfo& InVersionInfo);


/**
 * Register compiled in information for multiple classes, structures, and enumerations
 */
COREUOBJECT_API void RegisterCompiledInInfo(const TCHAR* PackageName, const FClassRegisterCompiledInInfo* ClassInfo, size_t NumClassInfo, const FStructRegisterCompiledInInfo* StructInfo, size_t NumStructInfo, const FEnumRegisterCompiledInInfo* EnumInfo, size_t NumEnumInfo);

/**
 * Base class for deferred native class registration
 */
struct UE_DEPRECATED(5.0, "FFieldCompiledInInfo has been deprecated, use the RegistrationInfo structures below.") FFieldCompiledInInfo
{
	FFieldCompiledInInfo(SIZE_T InClassSize, uint32 InCrc)
		: Size(InClassSize)
		, Crc(InCrc)
		, OldClass(nullptr)
		, bHasChanged(false)
	{
	}

	/** Registers the native class (constructs a UClass object) */
	virtual UClass* Register() const = 0;

	/** Return the package the class belongs in */
	virtual const TCHAR* ClassPackage() const = 0;

	/** Size of the class */
	SIZE_T Size;
	/** CRC of the generated code for this class */
	uint32 Crc;
	/** Old UClass object */
	UClass* OldClass;
	/** True if this class has changed after hot-reload (or new class) */
	bool bHasChanged;
};

/**
* Adds a class to deferred registration queue.
*/
PRAGMA_DISABLE_DEPRECATION_WARNINGS
UE_DEPRECATED(5.0, "UClassCompiledInDefer has been deprecated, use RegisterCompiledInInfo.")
COREUOBJECT_API void UClassCompiledInDefer(FFieldCompiledInInfo* Class, const TCHAR* Name, SIZE_T ClassSize, uint32 Crc);
PRAGMA_ENABLE_DEPRECATION_WARNINGS

/**
 * Specialized version of the deferred class registration structure.
 */
PRAGMA_DISABLE_DEPRECATION_WARNINGS
template <typename TClass>
struct UE_DEPRECATED(5.0, "TClassCompiledInDefer has been deprecated, use FRegisterCompiledInInfo.") TClassCompiledInDefer : public FFieldCompiledInInfo
{
	TClassCompiledInDefer(const TCHAR* InName, SIZE_T InClassSize, uint32 InCrc)
	: FFieldCompiledInInfo(InClassSize, InCrc)
	{
		UClassCompiledInDefer(this, InName, InClassSize, InCrc);
	}
	virtual UClass* Register() const override
	{
        LLM_SCOPE(ELLMTag::UObject);
		return TClass::StaticClass();
	}
	virtual const TCHAR* ClassPackage() const override
	{
		return TClass::StaticPackage();
	}
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS

/**
 * Stashes the singleton function that builds a compiled in class. Later, this is executed.
 */
UE_DEPRECATED(5.0, "UObjectCompiledInDefer has been deprecated, use RegisterCompiledInInfo.")
COREUOBJECT_API void UObjectCompiledInDefer(class UClass *(*InRegister)(), class UClass *(*InStaticClass)(), const TCHAR* Name, const TCHAR* PackageName, bool bDynamic, const TCHAR* DynamicPathName, void (*InInitSearchableValues)(TMap<FName, FName>&));

struct UE_DEPRECATED(5.0, "FCompiledInDefer has been deprecated, use FRegisterCompiledInInfo.") FCompiledInDefer
{
	FCompiledInDefer(class UClass *(*InRegister)(), class UClass *(*InStaticClass)(), const TCHAR* PackageName, const TCHAR* Name, bool bDynamic, const TCHAR* DynamicPackageName = nullptr, const TCHAR* DynamicPathName = nullptr, void (*InInitSearchableValues)(TMap<FName, FName>&) = nullptr)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		UObjectCompiledInDefer(InRegister, InStaticClass, Name, PackageName, bDynamic, DynamicPathName, InInitSearchableValues);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
};

/**
 * Stashes the singleton function that builds a compiled in struct (StaticStruct). Later, this is executed.
 */
UE_DEPRECATED(5.0, "UObjectCompiledInDeferStruct has been deprecated, use RegisterCompiledInInfo.")
COREUOBJECT_API void UObjectCompiledInDeferStruct(class UScriptStruct *(*InRegister)(), const TCHAR* PackageName, const TCHAR* ObjectName, bool bDynamic, const TCHAR* DynamicPathName);

struct UE_DEPRECATED(5.0, "FCompiledInDeferStruct has been deprecated, use FRegisterCompiledInInfo.") FCompiledInDeferStruct
{
	FCompiledInDeferStruct(class UScriptStruct *(*InRegister)(), const TCHAR* PackageName, const TCHAR* Name, bool bDynamic, const TCHAR* DynamicPackageName, const TCHAR* DynamicPathName)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		UObjectCompiledInDeferStruct(InRegister, PackageName, Name, bDynamic, DynamicPathName);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
};

/**
 * Stashes the singleton function that builds a compiled in enum. Later, this is executed.
 */
UE_DEPRECATED(5.0, "UObjectCompiledInDeferEnum has been deprecated, use RegisterCompiledInInfo.")
COREUOBJECT_API void UObjectCompiledInDeferEnum(class UEnum *(*InRegister)(), const TCHAR* PackageName, const TCHAR* ObjectName, bool bDynamic, const TCHAR* DynamicPathName);

struct UE_DEPRECATED(5.0, "FCompiledInDeferEnum has been deprecated, use FRegisterCompiledInInfo.") FCompiledInDeferEnum
{
	FCompiledInDeferEnum(class UEnum *(*InRegister)(), const TCHAR* PackageName, const TCHAR* Name, bool bDynamic, const TCHAR* DynamicPackageName, const TCHAR* DynamicPathName)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		UObjectCompiledInDeferEnum(InRegister, PackageName, Name, bDynamic, DynamicPathName);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
};

// @todo: BP2CPP_remove
/** Called during HotReload to hook up an existing structure */
UE_DEPRECATED(5.0, "This API is no longer in use and will be removed.")
COREUOBJECT_API class UScriptStruct* FindExistingStructIfHotReloadOrDynamic(UObject* Outer, const TCHAR* StructName, SIZE_T Size, uint32 Crc, bool bIsDynamic);

// @todo: BP2CPP_remove
/** Called during HotReload to hook up an existing enum */
UE_DEPRECATED(5.0, "This API is no longer in use and will be removed.")
COREUOBJECT_API class UEnum* FindExistingEnumIfHotReloadOrDynamic(UObject* Outer, const TCHAR* EnumName, SIZE_T Size, uint32 Crc, bool bIsDynamic);

/** Must be called after a module has been loaded that contains UObject classes */
COREUOBJECT_API void ProcessNewlyLoadedUObjects(FName Package = NAME_None, bool bCanProcessNewlyLoadedObjects = true);

/**
 * Final phase of UObject initialization. all auto register objects are added to the main data structures.
 */
void UObjectBaseInit();

/**
 * Final phase of UObject shutdown
 */
void UObjectBaseShutdown();

