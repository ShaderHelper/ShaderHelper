// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UObjectGlobals.h: Unreal object system globals.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "UObject/ObjectMacros.h"
#include "UObject/PrimaryAssetId.h"
#include "UObject/LinkerInstancingContext.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/PackagePath.h"
#include "Containers/ArrayView.h"
#include "Containers/StringView.h"
#include "Templates/Function.h"
#include "Templates/IsArrayOrRefOfType.h"
#include "Serialization/ArchiveUObject.h"

struct FCustomPropertyListNode;
struct FObjectInstancingGraph;
class FObjectPreSaveContext;
struct FObjectPtr;
struct FStaticConstructObjectParameters;
template <typename T>
struct TObjectPtr;
struct FWorldContext;

COREUOBJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogUObjectGlobals, Log, All);

DECLARE_CYCLE_STAT_EXTERN(TEXT("ConstructObject"),STAT_ConstructObject,STATGROUP_Object, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("AllocateObject"),STAT_AllocateObject,STATGROUP_ObjectVerbose, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("PostConstructInitializeProperties"),STAT_PostConstructInitializeProperties,STATGROUP_ObjectVerbose, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("LoadConfig"),STAT_LoadConfig,STATGROUP_Object, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("LoadObject"),STAT_LoadObject,STATGROUP_Object, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("InitProperties"),STAT_InitProperties,STATGROUP_Object, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("NameTable Entries"),STAT_NameTableEntries,STATGROUP_Object, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("NameTable ANSI Entries"),STAT_NameTableAnsiEntries,STATGROUP_Object, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("NameTable Wide Entries"),STAT_NameTableWideEntries,STATGROUP_Object, );
DECLARE_MEMORY_STAT_EXTERN(TEXT("NameTable Memory Size"),STAT_NameTableMemorySize,STATGROUP_Object, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("~UObject"),STAT_DestroyObject,STATGROUP_Object, );

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("FindObject"),STAT_FindObject,STATGROUP_ObjectVerbose, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("FindObjectFast"),STAT_FindObjectFast,STATGROUP_ObjectVerbose, );

#define	INVALID_OBJECT	(UObject*)-1
#define PERF_TRACK_DETAILED_ASYNC_STATS (0)


// Private system wide variables.

/** 
 * Set while in SavePackage() to advertise that a package is being saved
 * Deprecated, use `IsSavingPackage` instead
 * @see IsSavingPackage()
 */
//UE_DEPRECATED(5.0, "Use the IsSavingPackage() function instead.")
extern COREUOBJECT_API bool					GIsSavingPackage;
/** This allows loading unversioned cooked content in the editor */
extern COREUOBJECT_API int32				GAllowUnversionedContentInEditor;
/** This allows loading cooked content in the editor */
extern COREUOBJECT_API int32				GAllowCookedDataInEditorBuilds;

/** Enum used in StaticDuplicateObject() and related functions to describe why something is being duplicated */
namespace EDuplicateMode
{
	enum Type
	{
		/** No specific information about the reason for duplication */
		Normal,
		/** Object is being duplicated as part of a world duplication */
		World,
		/** Object is being duplicated as part of the process for entering Play In Editor */
		PIE
	};
};

/*-----------------------------------------------------------------------------
	FObjectDuplicationParameters.
-----------------------------------------------------------------------------*/

/**
 * This struct is used for passing parameter values to the StaticDuplicateObject() method.  Only the constructor parameters are required to
 * be valid - all other members are optional.
 */
struct FObjectDuplicationParameters
{
	/**
	 * The object to be duplicated
	 */
	UObject*		SourceObject;

	/**
	 * The object to use as the Outer for the duplicate of SourceObject.
	 */
	UObject*		DestOuter;

	/**
	 * The name to use for the duplicate of SourceObject
	 */
	FName			DestName;

	/**
	 * A bitmask of EObjectFlags to propagate to the duplicate of SourceObject (and its subobjects).
	 */
	EObjectFlags	FlagMask;

	/**
	 * A bitmask of EInternalObjectFlags to propagate to the duplicate of SourceObject (and its subobjects).
	*/
	EInternalObjectFlags InternalFlagMask;

	/**
	 * A bitmask of EObjectFlags to set on each duplicate object created.  Different from FlagMask in that only the bits
	 * from FlagMask which are also set on the source object will be set on the duplicate, while the flags in this value
	 * will always be set.
	 */
	EObjectFlags	ApplyFlags;

	/**
	 * A bitmask of EInternalObjectFlags to set on each duplicate object created.  Different from FlagMask in that only the bits
	* from FlagMask which are also set on the source object will be set on the duplicate, while the flags in this value
	* will always be set.
	*/
	EInternalObjectFlags	ApplyInternalFlags;

	/**
	 * Any PortFlags to be applied when serializing.
	 */
	uint32			PortFlags;

	EDuplicateMode::Type DuplicateMode;

	/**
	 * if an object being duplicated as an assigned external package, the duplicated object will try to assign an associated package to itself.
	 * The associated package should come from the DuplicationSeed.
	 */
	bool bAssignExternalPackages = true;

	/**
	 * if this option is true, then PostLoad won't be called on the new duplicated objects. 
	 * It will be the responsability of the caller in that case to eventually call post load on those objects. a `CreatedObjects` map should be provided.
	 */
	bool bSkipPostLoad = false;
	
	/**
	 * Optional class to specify for the destination object.
	 * @warning: MUST BE SERIALIZATION COMPATIBLE WITH SOURCE OBJECT, AND DOES NOT WORK WELL FOR OBJECT WHICH HAVE COMPLEX COMPONENT HIERARCHIES!!!
	 */
	UClass*			DestClass;

	/**
	 * Objects to use for prefilling the dup-source => dup-target map used by StaticDuplicateObject.  Can be used to allow individual duplication of several objects that share
	 * a common Outer in cases where you don't want to duplicate the shared Outer but need references between the objects to be replaced anyway.
	 *
	 * Objects in this map will NOT be duplicated
	 * Key should be the source object; value should be the object which will be used as its duplicate.
	 */
	TMap<UObject*,UObject*>	DuplicationSeed;

	/**
	 * If non-null, this will be filled with the list of objects created during the call to StaticDuplicateObject.
	 *
	 * Key will be the source object; value will be the duplicated object
	 */
	TMap<UObject*,UObject*>* CreatedObjects;

	/**
	 * Constructor
	 */
	COREUOBJECT_API FObjectDuplicationParameters( UObject* InSourceObject, UObject* InDestOuter );
};

// @note: should we start putting the code in the UE namespace?
namespace UE
{
	/**
	 * Return if the passed in package is currently saving,
	 * or if any packages are saving if no parameter is passed.
	 * @param InOuter The object which we want to check if its package is saving
	 * @returns true if the package is saving, or if any packages are saving if no parameter is passed in.
	 */
	COREUOBJECT_API bool IsSavingPackage(UObject* InOuter = nullptr);

} // namespace UE


/** Parses a bit mask of property flags into an array of string literals that match the flags */
COREUOBJECT_API TArray<const TCHAR*> ParsePropertyFlags(EPropertyFlags Flags);

/** Returns the transient top-level package, which is useful for temporarily storing objects that should never be saved */
COREUOBJECT_API UPackage* GetTransientPackage();

/** Returns an object in the transient package which respects the rules of Within */
COREUOBJECT_API UObject* GetTransientOuterForRename(UClass* ForClass);

/**
 * Gets INI file name from object's reference if it contains one. 
 *
 * @returns If object reference doesn't contain any INI reference the function returns nullptr. Otherwise a ptr to INI's file name.
 */
COREUOBJECT_API const FString* GetIniFilenameFromObjectsReference(const FString& ObjectsReferenceString);

/**
 * Resolves ini object path to string object path. This used to happen automatically in ResolveName but now must be called manually
 *
 * @param ObjectReference Ini reference, of the form engine-ini:/Script/Engine.Engine.DefaultMaterialName
 * @param IniFilename Ini filename. If null it will call GetIniFilenameFromObjectsReference
 * @param bThrow If true, will print an error if it can't find the file
 *
 * @returns Resolved object path.
 */
COREUOBJECT_API FString ResolveIniObjectsReference(const FString& ObjectReference, const FString* IniFilename = nullptr, bool bThrow = false);

/**
 * Internal function that takes a fully qualified or relative object path string and converts it into a path relative to a package.
 * Normally, you should call one of the FindObject or LoadObject functions instead.
 *
 * @param	Outer					The package to search within. If null, ObjectsReferenceString be a globally scoped path and this will be filled in with the actual package if found/created
 * @param	ObjectsReferenceString	The object path string to resolve. If it is successfully resolved, this will be replaced with a path relative to Outer
 * @param	Create					If true, it will try to load or create the required package if it is not in memory
 * @param	Throw					If true, it will potentially raise an error if the object cannot be found
 * @param	LoadFlags				Flags to use if Create is true and it needs to load a package, from the ELoadFlags enum
 * @param	InstancingContext		The linker instancing context used to resolve package name during instacning (i.e. when a package file is loaded into a package with a different name)
 * @return	True if the name was successfully resolved
 */
COREUOBJECT_API bool ResolveName(UObject*& Outer, FString& ObjectsReferenceString, bool Create, bool Throw, uint32 LoadFlags = LOAD_None, const FLinkerInstancingContext* InstancingContext = nullptr);

/** Internal function used to possibly output an error message, taking into account the outer and LoadFlags. Returns true if a log message was emitted. */
COREUOBJECT_API bool SafeLoadError( UObject* Outer, uint32 LoadFlags, const TCHAR* ErrorMessage);

/** Internal function used to update the suffix to be given to the next newly-created unnamed object. */
COREUOBJECT_API int32 UpdateSuffixForNextNewObject(UObject* Parent, const UClass* Class, TFunctionRef<void(int32&)> IndexMutator);


/**
 * Fast version of StaticFindObject that relies on the passed in FName being the object name without any group/package qualifiers.
 * This will only find top level packages or subobjects nested directly within a passed in outer.
 *
 * @param	Class			The to be found object's class
 * @param	InOuter			Outer object to look inside, if null this will only look for top level packages
 * @param	InName			Object name to look for relative to InOuter
 * @param	bExactClass		Whether to require an exact match with the passed in class
 * @param	bAnyPackage		Whether to look in any package
 * @param	ExclusiveFlags	Ignores objects that contain any of the specified exclusive flags
 * @param	ExclusiveInternalFlags  Ignores objects that contain any of the specified internal exclusive flags
 *
 * @return	Returns a pointer to the found object or null if none could be found
 */
COREUOBJECT_API UObject* StaticFindObjectFast(UClass* Class, UObject* InOuter, FName InName, bool bExactClass = false, bool bAnyPackage = false, EObjectFlags ExclusiveFlags = RF_NoFlags, EInternalObjectFlags ExclusiveInternalFlags = EInternalObjectFlags::None);

/**
 * Fast and safe version of StaticFindObject that relies on the passed in FName being the object name without any group/package qualifiers.
 * It will not assert on GIsSavingPackage or IsGarbageCollecting(). If called from within package saving code or GC, will return nullptr
 * This will only find top level packages or subobjects nested directly within a passed in outer.
 *
 * @param	Class			The to be found object's class
 * @param	InOuter			Outer object to look inside, if null this will only look for top level packages
 * @param	InName			Object name to look for relative to InOuter
 * @param	bExactClass		Whether to require an exact match with the passed in class
 * @param	bAnyPackage		Whether to look in any package
 * @param	ExclusiveFlags	Ignores objects that contain any of the specified exclusive flags
 * @param	ExclusiveInternalFlags  Ignores objects that contain any of the specified internal exclusive flags
 *
 * @return	Returns a pointer to the found object or null if none could be found
 */
COREUOBJECT_API UObject* StaticFindObjectFastSafe(UClass* Class, UObject* InOuter, FName InName, bool bExactClass = false, bool bAnyPackage = false, EObjectFlags ExclusiveFlags = RF_NoFlags, EInternalObjectFlags ExclusiveInternalFlags = EInternalObjectFlags::None);


/**
 * Tries to find an object in memory. This will handle fully qualified paths of the form /path/packagename.object:subobject and resolve references for you.
 *
 * @param	Class			The to be found object's class
 * @param	InOuter			Outer object to look inside. If this is ANY_PACKAGE it will search all in memory packages, if this is null then InName should start with a package name
 * @param	InName			The object path to search for an object, relative to InOuter
 * @param	ExactClass		Whether to require an exact match with the passed in class
 *
 * @return	Returns a pointer to the found object or nullptr if none could be found
 */
COREUOBJECT_API UObject* StaticFindObject( UClass* Class, UObject* InOuter, const TCHAR* Name, bool ExactClass=false );

/** Version of StaticFindObject() that will assert if the object is not found */
COREUOBJECT_API UObject* StaticFindObjectChecked( UClass* Class, UObject* InOuter, const TCHAR* Name, bool ExactClass=false );

/** Internal version of StaticFindObject that will not assert on GIsSavingPackage or IsGarbageCollecting() */
COREUOBJECT_API UObject* StaticFindObjectSafe( UClass* Class, UObject* InOuter, const TCHAR* Name, bool ExactClass=false );


/**
 * Parse a reference to an object from a text representation
 *
 * @param Stream		String containing text to parse
 * @param Match				Tag to search for object representation within string
 * @param Class			The class of the object to be loaded.
 * @param DestRes			Returned object pointer
 * @param InParent		Outer to search
 * @param bInvalidObject	[opt] Optional output.  If true, Tag was matched but the specified object wasn't found.
 *
 * @return True if the object parsed successfully, even if object was not found
 */
COREUOBJECT_API bool ParseObject( const TCHAR* Stream, const TCHAR* Match, UClass* Class, UObject*& DestRes, UObject* InParent, bool* bInvalidObject=nullptr );

/**
 * Find or load an object by string name with optional outer and filename specifications.
 * These are optional because the InName can contain all of the necessary information.
 *
 * @param ObjectClass	The class (or a superclass) of the object to be loaded.
 * @param InOuter		An optional object to narrow where to find/load the object from
 * @param Name			String name of the object. If it's not fully qualified, InOuter and/or Filename will be needed
 * @param Filename		An optional file to load from (or find in the file's package object)
 * @param LoadFlags		Flags controlling how to handle loading from disk, from the ELoadFlags enum
 * @param Sandbox		A list of packages to restrict the search for the object
 * @param bAllowObjectReconciliation	Whether to allow the object to be found via FindObject in the case of seek free loading
 * @param InstancingContext				InstancingContext used to remap imports when loading a packager under a new name
 *
 * @return The object that was loaded or found. nullptr for a failure.
 */
COREUOBJECT_API UObject* StaticLoadObject( UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, UPackageMap* Sandbox = nullptr, bool bAllowObjectReconciliation = true, const FLinkerInstancingContext* InstancingContext = nullptr);

/** Version of StaticLoadObject() that will load classes */
COREUOBJECT_API UClass* StaticLoadClass(UClass* BaseClass, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, UPackageMap* Sandbox = nullptr);

/**
 * Create a new instance of an object.  The returned object will be fully initialized.  If InFlags contains RF_NeedsLoad (indicating that the object still needs to load its object data from disk), components
 * are not instanced (this will instead occur in PostLoad()).  The different between StaticConstructObject and StaticAllocateObject is that StaticConstructObject will also call the class constructor on the object
 * and instance any components.
 *
 * @param	Params		The parameters to use when construction the object. @see FStaticConstructObjectParameters
 *
 * @return	A pointer to a fully initialized object of the specified class.
 */
COREUOBJECT_API UObject* StaticConstructObject_Internal(const FStaticConstructObjectParameters& Params);

/**
 * Create a new instance of an object.  The returned object will be fully initialized.  If InFlags contains RF_NeedsLoad (indicating that the object still needs to load its object data from disk), components
 * are not instanced (this will instead occur in PostLoad()).  The different between StaticConstructObject and StaticAllocateObject is that StaticConstructObject will also call the class constructor on the object
 * and instance any components.
 *
 * @param	Class		The class of the object to create
 * @param	InOuter		The object to create this object within (the Outer property for the new object will be set to the value specified here).
 * @param	Name		The name to give the new object. If no value (NAME_None) is specified, the object will be given a unique name in the form of ClassName_#.
 * @param	SetFlags	The ObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
 * @param	InternalSetFlags	The InternalObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
 * @param	Template	If specified, the property values from this object will be copied to the new object, and the new object's ObjectArchetype value will be set to this object.
 *						If nullptr, the class default object is used instead.
 * @param	bInCopyTransientsFromClassDefaults	If true, copy transient from the class defaults instead of the pass in archetype ptr (often these are the same)
 * @param	InstanceGraph	Contains the mappings of instanced objects and components to their templates
 * @param	bAssumeTemplateIsArchetype	If true, Template is guaranteed to be an archetype
 * @param	ExternalPackage	Assign an external Package to the created object if non-null
 *
 * @return	A pointer to a fully initialized object of the specified class.
 */
UE_DEPRECATED(4.26, "Use version that takes parameter struct")
COREUOBJECT_API UObject* StaticConstructObject_Internal(const UClass* Class, UObject* InOuter = (UObject*)GetTransientPackage(), FName Name = NAME_None, EObjectFlags SetFlags = RF_NoFlags, EInternalObjectFlags InternalSetFlags = EInternalObjectFlags::None, UObject* Template = nullptr, bool bCopyTransientsFromClassDefaults = false, struct FObjectInstancingGraph* InstanceGraph = nullptr, bool bAssumeTemplateIsArchetype = false, UPackage* ExternalPackage = nullptr);

/**
 * Creates a copy of SourceObject using the Outer and Name specified, as well as copies of all objects contained by SourceObject.  
 * Any objects referenced by SourceOuter or RootObject and contained by SourceOuter are also copied, maintaining their name relative to SourceOuter.
 * Any references to objects that are duplicated are automatically replaced with the copy of the object.
 *
 * @param	SourceObject	The object to duplicate
 * @param	DestOuter		The object to use as the Outer for the copy of SourceObject
 * @param	DestName		The name to use for the copy of SourceObject, if none it will be autogenerated
 * @param	FlagMask		A bitmask of EObjectFlags that should be propagated to the object copies.  The resulting object copies will only have the object flags
 *							specified copied from their source object.
 * @param	DestClass		Optional class to specify for the destination object. MUST BE SERIALIZATION COMPATIBLE WITH SOURCE OBJECT!!!
 * @param	InternalFlagsMask  Bitmask of EInternalObjectFlags that should be propagated to the object copies.
 *
 * @return	The duplicate of SourceObject.
 *
 * @deprecated This version is deprecated in favor of StaticDuplicateObjectEx
 */
COREUOBJECT_API UObject* StaticDuplicateObject(UObject const* SourceObject, UObject* DestOuter, const FName DestName = NAME_None, EObjectFlags FlagMask = RF_AllFlags, UClass* DestClass = nullptr, EDuplicateMode::Type DuplicateMode = EDuplicateMode::Normal, EInternalObjectFlags InternalFlagsMask = EInternalObjectFlags::AllFlags);

/**
 * Returns FObjectDuplicationParameters initialized based of StaticDuplicateObject parameters
 */
COREUOBJECT_API FObjectDuplicationParameters InitStaticDuplicateObjectParams(UObject const* SourceObject, UObject* DestOuter, const FName DestName = NAME_None, EObjectFlags FlagMask = RF_AllFlags, UClass* DestClass = nullptr, EDuplicateMode::Type DuplicateMode = EDuplicateMode::Normal, EInternalObjectFlags InternalFlagsMask = EInternalObjectFlags::AllFlags);

/**
 * Creates a copy of SourceObject using the Outer and Name specified, as well as copies of all objects contained by SourceObject.
 * Any objects referenced by SourceOuter or RootObject and contained by SourceOuter are also copied, maintaining their name relative to SourceOuter.
 * Any references to objects that are duplicated are automatically replaced with the copy of the object.
 *
 * @param	Parameters  Specific options to use when duplicating this object
 *
 * @return	The duplicate of SourceObject.
 */
COREUOBJECT_API UObject* StaticDuplicateObjectEx( FObjectDuplicationParameters& Parameters );

/** 
 * Parses a global context system console or debug command and executes it.
 *
 * @param	InWorld		The world to use as a context, enables certain commands
 * @param	Cmd			Command string to execute
 * @param	Ar			Output device to write results of commands to
 * 
 * @return	True if the command was successfully parsed
 */
COREUOBJECT_API bool StaticExec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar=*GLog );

/**
 * Static UObject tick function, used to verify certain key assumptions and to tick the async loading code.
 *
 * @param DeltaTime	Time in seconds since last call
 * @param bUseFullTimeLimit	If true, use the entire time limit even if blocked on I/O
 * @param AsyncLoadingTime Time in seconds to use for async loading limit
 */
COREUOBJECT_API void StaticTick( float DeltaTime, bool bUseFullTimeLimit = true, float AsyncLoadingTime = 0.005f );

/**
 * Loads a package and all contained objects that match context flags.
 *
 * @param	InOuter				Package to load new package into (usually nullptr or ULevel->GetOuter())
 * @param	InLongPackageName	Long package name to load, if null the name is taken from InOuter
 * @param	LoadFlags			Flags controlling loading behavior, from the ELoadFlags enum
 * @param	InReaderOverride	Optional archive to use for reading package data
 * @param	InLoadContext		Additional context when called during serialization
 * @param   InstancingContext   Additional context to map object names to their instanced counterpart when loading an instanced package
 *
 * @return	Loaded package if successful, nullptr otherwise
 */
COREUOBJECT_API UPackage* LoadPackage( UPackage* InOuter, const TCHAR* InLongPackageName, uint32 LoadFlags, FArchive* InReaderOverride = nullptr, const FLinkerInstancingContext* InstancingContext = nullptr);

/**
 * Loads a package and all contained objects that match context flags.
 *
 * @param	InOuter				Package to load new package into (usually nullptr or ULevel->GetOuter())
 * @param	InPackagePath		PackagePath to load, must be non-empty
 * @param	LoadFlags			Flags controlling loading behavior, from the ELoadFlags enum
 * @param	InReaderOverride	Optional archive to use for reading package data
 * @param	InLoadContext		Additional context when called during serialization
 * @param   InstancingContext   Additional context to map object names to their instanced counterpart when loading an instanced package
 * @param   DiffPackagePath		An additional PackagePath to load and compare to the package at InPackagePath, used when diffing packages
 *
 * @return	Loaded package if successful, nullptr otherwise
 */
COREUOBJECT_API UPackage* LoadPackage(UPackage* InOuter, const FPackagePath& InPackagePath, uint32 LoadFlags, FArchive* InReaderOverride = nullptr, const FLinkerInstancingContext* InstancingContext = nullptr, const FPackagePath* DiffPackagePath = nullptr);

/** Async package loading result */
namespace EAsyncLoadingResult
{
	enum Type
	{
		/** Package failed to load */
		Failed,
		/** Package loaded successfully */
		Succeeded,
		/** Async loading was canceled */
		Canceled
	};
}

/** The type that represents an async loading priority */
typedef int32 TAsyncLoadPriority;

/**
 * Delegate called on completion of async package loading
 * @param	PackageName			Package name we were trying to load
 * @param	LoadedPackage		Loaded package if successful, nullptr otherwise	
 * @param	Result		Result of async loading.
 */
DECLARE_DELEGATE_ThreeParams(FLoadPackageAsyncDelegate, const FName& /*PackageName*/, UPackage* /*LoadedPackage*/, EAsyncLoadingResult::Type /*Result*/)

/**
 * Asynchronously load a package and all contained objects that match context flags. Non-blocking.
 * Use this version to specify the PackagePath rather than having the other versions internally convert the InName to a PackagePath by searching the current package mount points.
 * Use this version if you need to specify a packagename that is different from the packagename on disk; this is useful when loading multiple copies of the same package.
 *
 * @param	InPackagePath			PackagePath to load. Must be a mounted path. The package is created if it does not already exist.
 * @param	InPackageNameToCreate	If not none, this is the name of the package to load the bytes on disk into (and create if not yet existing). If none, the name is taken from PackagePath.
 * @param	InCompletionDelegate	Delegate to be invoked when the packages has finished streaming
 * @param	InPackageFlags			Package flags used to construct loaded package in memory
 * @param	InPIEInstanceID			Play in Editor instance ID
 * @param	InPackagePriority		Loading priority
 * @param   InstancingContext		Additional context to map object names to their instanced counterpart when loading an instanced package
 * @return Unique ID associated with this load request (the same package can be associated with multiple IDs).
 */
COREUOBJECT_API int32 LoadPackageAsync(const FPackagePath& InPackagePath, FName InPackageNameToCreate = NAME_None, FLoadPackageAsyncDelegate InCompletionDelegate = FLoadPackageAsyncDelegate(), EPackageFlags InPackageFlags = PKG_None, int32 InPIEInstanceID = INDEX_NONE, TAsyncLoadPriority InPackagePriority = 0, const FLinkerInstancingContext* InstancingContext = nullptr);

/**
 * Asynchronously load a package and all contained objects that match context flags. Non-blocking.
 * Use this version for convenience when you just need to load a package without notification and with default behavior from a packagename/filename.
 *
 * @param	InName					PackageName or LocalFilePath of package to load. Must be a mounted name/path. The package is created if it does not already exist.
 * @param	InGuid					GUID of the package to load, or nullptr for "don't care"
 * @return Unique ID associated with this load request (the same package can be associated with multiple IDs).
 */
COREUOBJECT_API int32 LoadPackageAsync(const FString& InName, const FGuid* InGuid = nullptr);

/**
 * Asynchronously load a package and all contained objects that match context flags. Non-blocking.
 * Use this version when you need to load a package with default behavior from a packagename/filename, and need to be notified when it is loaded.
 *
 * @param	InName					PackageName or LocalFilePath of package to load. Must be a mounted name/path. The package is created if it does not already exist.
 * @param	InCompletionDelegate	Delegate to be invoked when the packages has finished streaming
 * @param	InPackagePriority		Loading priority
 * @param	InPackageFlags			Package flags used to construct loaded package in memory
 * @param	InPIEInstanceID			Play in Editor instance ID
 * @return Unique ID associated with this load request (the same package can be associated with multiple IDs).
 */
COREUOBJECT_API int32 LoadPackageAsync(const FString& InName, FLoadPackageAsyncDelegate InCompletionDelegate, TAsyncLoadPriority InPackagePriority = 0, EPackageFlags InPackageFlags = PKG_None, int32 InPIEInstanceID = INDEX_NONE);

UE_DEPRECATED(5.0, "Use version that takes a FPackagePath without a FGuid")
COREUOBJECT_API int32 LoadPackageAsync(const FString& InName, const FGuid* InGuid, const TCHAR* InPackageToLoadFrom, FLoadPackageAsyncDelegate InCompletionDelegate = FLoadPackageAsyncDelegate(), EPackageFlags InPackageFlags = PKG_None, int32 InPIEInstanceID = INDEX_NONE, TAsyncLoadPriority InPackagePriority = 0, const FLinkerInstancingContext* InstancingContext = nullptr);

/**
* Cancels all async package loading requests.
*/
COREUOBJECT_API void CancelAsyncLoading();

/**
* Returns true if the event driven loader is enabled in cooked builds
*/
UE_DEPRECATED(5.0, "Any call to IsEventDrivenLoaderEnabledInCookedBuilds can be removed. "
		"Cooked packages are always split into different files/segments and headers will always contain preload dependencies.")
COREUOBJECT_API bool IsEventDrivenLoaderEnabledInCookedBuilds();

/**
* Returns true if the event driven loader is enabled in the current build
*/
COREUOBJECT_API bool IsEventDrivenLoaderEnabled();

/**
 * Returns the async load percentage for a package in flight with the passed in name or -1 if there isn't one.
 * @warning THIS IS SLOW. MAY BLOCK ASYNC LOADING.
 *
 * @param	PackageName			Name of package to query load percentage for
 * @return	Async load percentage if package is currently being loaded, -1 otherwise
 */
COREUOBJECT_API float GetAsyncLoadPercentage( const FName& PackageName );

/**
* Whether we are running on the Garbage Collector Thread
*/
COREUOBJECT_API bool IsInGarbageCollectorThread();

/** 
 * Deletes all unreferenced objects, keeping objects that have any of the passed in KeepFlags set. Will wait for other threads to unlock GC.
 *
 * @param	KeepFlags			objects with those flags will be kept regardless of being referenced or not
 * @param	bPerformFullPurge	if true, perform a full purge after the mark pass
 */
COREUOBJECT_API void CollectGarbage(EObjectFlags KeepFlags, bool bPerformFullPurge = true);

/**
* Performs garbage collection only if no other thread holds a lock on GC
*
* @param	KeepFlags			objects with those flags will be kept regardless of being referenced or not
* @param	bPerformFullPurge	if true, perform a full purge after the mark pass
*/
COREUOBJECT_API bool TryCollectGarbage(EObjectFlags KeepFlags, bool bPerformFullPurge = true);

/**
* Calls ConditionalBeginDestroy on unreachable objects
*
* @param	bUseTimeLimit	whether the time limit parameter should be used
* @param	TimeLimit		soft time limit for this function call
*
* @return true if the time limit passed and there's still objects pending to be unhashed
*/
COREUOBJECT_API bool UnhashUnreachableObjects(bool bUseTimeLimit, double TimeLimit = 0.0);

/**
* Checks if there's objects pending to be unhashed when running incremental purge
*
* @return true if the time limit passed and there's still objects pending to be unhashed
*/
COREUOBJECT_API bool IsIncrementalUnhashPending();

/**
 * Returns whether an incremental purge is still pending/ in progress.
 *
 * @return	true if incremental purge needs to be kicked off or is currently in progress, false othwerise.
 */
COREUOBJECT_API bool IsIncrementalPurgePending();

/**
 * Gathers unreachable objects for IncrementalPurgeGarbage.
 *
 * @param bForceSingleThreaded true to force the process to just one thread
 */
COREUOBJECT_API void GatherUnreachableObjects(bool bForceSingleThreaded);

/**
 * Incrementally purge garbage by deleting all unreferenced objects after routing Destroy.
 *
 * Calling code needs to be EXTREMELY careful when and how to call this function as 
 * RF_Unreachable cannot change on any objects unless any pending purge has completed!
 *
 * @param	bUseTimeLimit	whether the time limit parameter should be used
 * @param	TimeLimit		soft time limit for this function call
 */
COREUOBJECT_API void IncrementalPurgeGarbage( bool bUseTimeLimit, double TimeLimit = 0.002 );

/**
 * Create a unique name by combining a base name and an arbitrary number string.
 * The object name returned is guaranteed not to exist.
 *
 * @param	Parent		the outer for the object that needs to be named
 * @param	Class		the class for the object
 * @param	BaseName	optional base name to use when generating the unique object name; if not specified, the class's name is used
 *
 * @return	name is the form BaseName_##, where ## is the number of objects of this
 *			type that have been created since the last time the class was garbage collected.
 */
COREUOBJECT_API FName MakeUniqueObjectName( UObject* Outer, const UClass* Class, FName BaseName=NAME_None );

/**
 * Given a display label string, generates an FName slug that is a valid FName for that label.
 * If the object's current name is already satisfactory, then that name will be returned.
 * For example, "[MyObject]: Object Label" becomes "MyObjectObjectLabel" FName slug.
 * 
 * Note: The generated name isn't guaranteed to be unique.
 *
 * @param DisplayLabel The label string to convert to an FName
 * @param CurrentObjectName The object's current name, or NAME_None if it has no name yet
 *
 * @return	The generated object name
 */
COREUOBJECT_API FName MakeObjectNameFromDisplayLabel(const FString& DisplayLabel, const FName CurrentObjectName);

/**
 * Returns whether an object is referenced, not counting references from itself
 *
 * @param	Obj			Object to check
 * @param	KeepFlags	Objects with these flags will be considered as being referenced
 * @param	InternalKeepFlags	Objects with these internal flags will be considered as being referenced
 * @param	bCheckSubObjects	Treat subobjects as if they are the same as passed in object
 * @param	FoundReferences		If non-nullptr fill in with list of objects that hold references
 * @return true if object is referenced, false otherwise
 */
COREUOBJECT_API bool IsReferenced( UObject*& Res, EObjectFlags KeepFlags, EInternalObjectFlags InternalKeepFlags, bool bCheckSubObjects = false, FReferencerInformationList* FoundReferences = nullptr );

/**
 * Blocks till all pending package/ linker requests are fulfilled.
 *
 * @param PackageID if the package associated with this request ID gets loaded, FlushAsyncLoading returns 
 *        immediately without waiting for the remaining packages to finish loading.
 */
COREUOBJECT_API void FlushAsyncLoading(int32 PackageID = INDEX_NONE);

/**
 * Return number of active async load package requests
 */
COREUOBJECT_API int32 GetNumAsyncPackages();

/**
 * Returns whether we are currently loading a package (sync or async)
 *
 * @return true if we are loading a package, false otherwise
 */
COREUOBJECT_API bool IsLoading();

/**
 * Allows or disallows async loading (for example async loading is not allowed after the final flush on exit)
 *
 * @param bAllowAsyncLoading true if async loading should be allowed, false otherwise
 */
COREUOBJECT_API void SetAsyncLoadingAllowed(bool bAllowAsyncLoading);

/**
 * State of the async package after the last tick.
 */
namespace EAsyncPackageState
{
	enum Type
	{
		/** Package tick has timed out. */
		TimeOut = 0,
		/** Package has pending import packages that need to be streamed in. */
		PendingImports,
		/** Package has finished loading. */
		Complete,
	};
}

/**
 * Serializes a bit of data each frame with a soft time limit. The function is designed to be able
 * to fully load a package in a single pass given sufficient time.
 *
 * @param	bUseTimeLimit	Whether to use a time limit
 * @param	bUseFullTimeLimit	If true, use the entire time limit even if blocked on I/O
 * @param	TimeLimit		Soft limit of time this function is allowed to consume
 * @return The minimum state of any of the queued packages.
 */
COREUOBJECT_API EAsyncPackageState::Type ProcessAsyncLoading( bool bUseTimeLimit, bool bUseFullTimeLimit, double TimeLimit);

/**
 * Blocks and runs ProcessAsyncLoading until the time limit is hit, the completion predicate returns true, or all async loading is done
 * 
 * @param	CompletionPredicate	If this returns true, stop loading. This is called periodically as long as loading continues
 * @param	TimeLimit			Hard time limit. 0 means infinite length
 * @return The minimum state of any of the queued packages.
 */
COREUOBJECT_API EAsyncPackageState::Type ProcessAsyncLoadingUntilComplete(TFunctionRef<bool()> CompletionPredicate, double TimeLimit);

/** UObjects are being loaded between these calls */
COREUOBJECT_API void BeginLoad(FUObjectSerializeContext* LoadContext, const TCHAR* DebugContext = nullptr);
COREUOBJECT_API void EndLoad(FUObjectSerializeContext* LoadContext);

/**
 * Find an existing package by name
 * @param InOuter		The Outer object to search inside
 * @param PackageName	The name of the package to find
 *
 * @return The package if it exists
 */
COREUOBJECT_API UPackage* FindPackage(UObject* InOuter, const TCHAR* PackageName);

/**
 * Find an existing package by name or create it if it doesn't exist
 * @param InOuter		The Outer object to search inside (unused)
 * @return The existing package or a newly created one
 *
 */
UE_DEPRECATED(4.26, "Use CreatePackage overload that does not take the first Outer parameter. Specifying non-null outers for UPackages is no longer supported.")
COREUOBJECT_API UPackage* CreatePackage( UObject* InOuter, const TCHAR* PackageName );

/**
 * Find an existing package by name or create it if it doesn't exist
 * @return The existing package or a newly created one
 *
 */
COREUOBJECT_API UPackage* CreatePackage(const TCHAR* PackageName);

/** Internal function used to set a specific property value from debug/console code */
void GlobalSetProperty( const TCHAR* Value, UClass* Class, FProperty* Property, bool bNotifyObjectOfChange );

/**
 * Save a copy of this object into the transaction buffer if we are currently recording into
 * one (undo/redo). If bMarkDirty is true, will also mark the package as needing to be saved.
 *
 * @param	bMarkDirty	If true, marks the package dirty if we are currently recording into a
 *						transaction buffer
 * @param	Object		object to save.
 *
 * @return	true if a copy of the object was saved and the package potentially marked dirty; false
 *			if we are not recording into a transaction buffer, the package is a PIE/script package,
 *			or the object is not transactional (implies the package was not marked dirty)
 */
COREUOBJECT_API bool SaveToTransactionBuffer(UObject* Object, bool bMarkDirty);

/**
 * Causes the transaction system to emit a snapshot event for the given object if the following conditions are met:
 *  a) The object is currently transacting.
 *  b) The object has changed since it started transacting.
 *
 * @param	Object		object to snapshot.
 * @param	Properties	optional list of properties that have potentially changed on the object (to avoid snapshotting the entire object).
 */
COREUOBJECT_API void SnapshotTransactionBuffer(UObject* Object);
COREUOBJECT_API void SnapshotTransactionBuffer(UObject* Object, TArrayView<const FProperty*> Properties);


/**
 * Utility struct that allows abstract classes to be allocated for non-CDOs while in scope.
 * Abstract objects are generally unsafe and should only be allocated in very unusual circumstances.
 */
struct FScopedAllowAbstractClassAllocation : public FNoncopyable
{
	COREUOBJECT_API FScopedAllowAbstractClassAllocation();
	COREUOBJECT_API ~FScopedAllowAbstractClassAllocation();
	static bool IsDisallowedAbstractClass(const UClass* InClass, EObjectFlags InFlags);

private:
	static int32 AllowAbstractCount;
};

/**
 * Check for StaticAllocateObject error; only for use with the editor, make or other commandlets.
 * 
 * @param	Class		the class of the object to create
 * @param	InOuter		the object to create this object within (the Outer property for the new object will be set to the value specified here).
 * @param	Name		the name to give the new object. If no value (NAME_None) is specified, the object will be given a unique name in the form of ClassName_#.
 * @param	SetFlags	the ObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
 * @return	true if nullptr should be returned; there was a problem reported 
 */
bool StaticAllocateObjectErrorTests( const UClass* Class, UObject* InOuter, FName Name, EObjectFlags SetFlags);

/**
 * Create a new instance of an object or replace an existing object.  If both an Outer and Name are specified, and there is an object already in memory with the same Class, Outer, and Name, the
 * existing object will be destructed, and the new object will be created in its place.
 * 
 * @param	Class		the class of the object to create
 * @param	InOuter		the object to create this object within (the Outer property for the new object will be set to the value specified here).
 * @param	Name		the name to give the new object. If no value (NAME_None) is specified, the object will be given a unique name in the form of ClassName_#.
 * @param	SetFlags	the ObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
 * @param InternalSetFlags	the InternalObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
 * @param bCanReuseSubobjects	if set to true, SAO will not attempt to destroy a subobject if it already exists in memory.
 * @param bOutReusedSubobject	flag indicating if the object is a subobject that has already been created (in which case further initialization is not necessary).
 * @param ExternalPackage	External Package assigned to the allocated object, if any	
 * @return	a pointer to a fully initialized object of the specified class.
 */
COREUOBJECT_API UObject* StaticAllocateObject(const UClass* Class, UObject* InOuter, FName Name, EObjectFlags SetFlags, EInternalObjectFlags InternalSetFlags = EInternalObjectFlags::None, bool bCanReuseSubobjects = false, bool* bOutReusedSubobject = nullptr, UPackage* ExternalPackage = nullptr);

/** FObjectInitializer options */
enum class EObjectInitializerOptions
{
	None							= 0,
	CopyTransientsFromClassDefaults = 1 << 0, // copy transient from the class defaults instead of the pass in archetype ptr
	InitializeProperties			= 1 << 1, // initialize property values with the archetype values
};
ENUM_CLASS_FLAGS(EObjectInitializerOptions)

class FGCObject;

/**
 * Internal class to finalize UObject creation (initialize properties) after the real C++ constructor is called.
 **/
class COREUOBJECT_API FObjectInitializer
{
public:
	/**
	 * Default Constructor, used when you are using the C++ "new" syntax. UObject::UObject will set the object pointer
	 **/
	FObjectInitializer();

	/**
	 * Constructor
	 * @param	InObj object to initialize, from static allocate object, after construction
	 * @param	InObjectArchetype object to initialize properties from
	 * @param	InOptions initialization options, see EObjectInitializerOptions
	 * @param	InInstanceGraph passed instance graph
	 */
	FObjectInitializer(UObject* InObj, UObject* InObjectArchetype, EObjectInitializerOptions InOptions, struct FObjectInstancingGraph* InInstanceGraph = nullptr);

	UE_DEPRECATED(5.0, "Use version that takes EObjectInitializerOptions")
	FObjectInitializer(UObject* InObj, UObject* InObjectArchetype, bool bInCopyTransientsFromClassDefaults, bool bInShouldInitializeProps, struct FObjectInstancingGraph* InInstanceGraph = nullptr)
		: FObjectInitializer(InObj, InObjectArchetype, 
			(bInCopyTransientsFromClassDefaults ? EObjectInitializerOptions::CopyTransientsFromClassDefaults : EObjectInitializerOptions::None) |
			(bInShouldInitializeProps ? EObjectInitializerOptions::InitializeProperties : EObjectInitializerOptions::None),
			InInstanceGraph)
	{
	}

	/** Special constructor for static construct object internal that passes along the params block directly */
	FObjectInitializer(UObject* InObj, const FStaticConstructObjectParameters& StaticConstructParams);

private:
	/** Helper for the common behaviors in the constructors */
	void Construct_Internal();

public:
	~FObjectInitializer();

	/** 
	 * Return the archetype that this object will copy properties from later
	**/
	FORCEINLINE UObject* GetArchetype() const
	{
		return ObjectArchetype;
	}

	/**
	* Return the object that is being constructed
	**/
	FORCEINLINE UObject* GetObj() const
	{
		return Obj;
	}

	/**
	* Return the class of the object that is being constructed
	**/
	UClass* GetClass() const;

	/**
	 * Create a component or subobject
	 * @param	TReturnType					class of return type, all overrides must be of this type
	 * @param	Outer						outer to construct the subobject in
	 * @param	SubobjectName				name of the new component
	 * @param bTransient		true if the component is being assigned to a transient property
	 */
	template<class TReturnType>
	TReturnType* CreateDefaultSubobject(UObject* Outer, FName SubobjectName, bool bTransient = false) const
	{
		UClass* ReturnType = TReturnType::StaticClass();
		return static_cast<TReturnType*>(CreateDefaultSubobject(Outer, SubobjectName, ReturnType, ReturnType, /*bIsRequired =*/ true, bTransient));
	}

	/**
	 * Create optional component or subobject. Optional subobjects may not get created
	 * when a derived class specified DoNotCreateDefaultSubobject with the subobject's name.
	 * @param	TReturnType					class of return type, all overrides must be of this type
	 * @param	Outer						outer to construct the subobject in
	 * @param	SubobjectName				name of the new component
	 * @param bTransient		true if the component is being assigned to a transient property
	 */
	template<class TReturnType>
	TReturnType* CreateOptionalDefaultSubobject(UObject* Outer, FName SubobjectName, bool bTransient = false) const
	{
		UClass* ReturnType = TReturnType::StaticClass();
		return static_cast<TReturnType*>(CreateDefaultSubobject(Outer, SubobjectName, ReturnType, ReturnType, /*bIsRequired =*/ false, bTransient));
	}

	/** 
	* Create a component or subobject 
	* @param TReturnType class of return type, all overrides must be of this type 
	* @param TClassToConstructByDefault class to construct by default
	* @param Outer outer to construct the subobject in 
	* @param SubobjectName name of the new component 
	* @param bTransient		true if the component is being assigned to a transient property
	*/ 
	template<class TReturnType, class TClassToConstructByDefault> 
	TReturnType* CreateDefaultSubobject(UObject* Outer, FName SubobjectName, bool bTransient = false) const 
	{ 
		return static_cast<TReturnType*>(CreateDefaultSubobject(Outer, SubobjectName, TReturnType::StaticClass(), TClassToConstructByDefault::StaticClass(), /*bIsRequired =*/ true, bTransient));
	}

	/**
	 * Create a component or subobject only to be used with the editor.
	 * @param	TReturnType					class of return type, all overrides must be of this type
	 * @param	Outer						outer to construct the subobject in
	 * @param	SubobjectName				name of the new component
	 * @param	bTransient					true if the component is being assigned to a transient property
	 */
	template<class TReturnType>
	TReturnType* CreateEditorOnlyDefaultSubobject(UObject* Outer, FName SubobjectName, bool bTransient = false) const
	{
		const UClass* ReturnType = TReturnType::StaticClass();
		return static_cast<TReturnType*>(CreateEditorOnlyDefaultSubobject(Outer, SubobjectName, ReturnType, bTransient));
	}

	/**
	* Create a component or subobject only to be used with the editor.
	* @param	Outer						outer to construct the subobject in
	* @param	SubobjectName				name of the new component
	* @param	ReturnType					type of the new component
	* @param	bTransient					true if the component is being assigned to a transient property
	*/
	UObject* CreateEditorOnlyDefaultSubobject(UObject* Outer, FName SubobjectName, const UClass* ReturnType, bool bTransient = false) const;

	/**
	 * Create a component or subobject
	 * @param	Outer                       outer to construct the subobject in
	 * @param	SubobjectName               name of the new component
	 * @param	ReturnType                  class of return type, all overrides must be of this type
	 * @param	ClassToConstructByDefault   if the derived class has not overridden, create a component of this type
	 * @param	bIsRequired                 true if the component is required and will always be created even if DoNotCreateDefaultSubobject was specified.
	 * @param	bIsTransient                true if the component is being assigned to a transient property
	 */
	UObject* CreateDefaultSubobject(UObject* Outer, FName SubobjectFName, const UClass* ReturnType, const UClass* ClassToCreateByDefault, bool bIsRequired = true, bool bIsTransient = false) const;

	/**
	 * Sets the class to use for a subobject defined in a base class, the class must be a subclass of the class used by the base class.
	 * @param	SubobjectName	name of the new component or subobject
	 * @param	Class			The class to use for the specified subobject or component.
	 */
	const FObjectInitializer& SetDefaultSubobjectClass(FName SubobjectName, const UClass* Class) const
	{
		AssertIfSubobjectSetupIsNotAllowed(SubobjectName);
		SubobjectOverrides.Add(SubobjectName, Class);
		return *this;
	}

	/**
	 * Sets the class to use for a subobject defined in a base class, the class must be a subclass of the class used by the base class.
	 * @param	SubobjectName	name of the new component or subobject
	 */
	template<class T>
	const FObjectInitializer& SetDefaultSubobjectClass(FName SubobjectName) const
	{
		return SetDefaultSubobjectClass(SubobjectName, T::StaticClass());
	}

	/**
	 * Indicates that a base class should not create a component
	 * @param	SubobjectName	name of the new component or subobject to not create
	 */
	const FObjectInitializer& DoNotCreateDefaultSubobject(FName SubobjectName) const
	{
		AssertIfSubobjectSetupIsNotAllowed(SubobjectName);
		SubobjectOverrides.Add(SubobjectName, nullptr);
		return *this;
	}

	/**
	 * Sets the class to use for a subobject defined in a nested subobject, the class must be a subclass of the class used when calling CreateDefaultSubobject.
	 * @param	SubobjectName	path to the new component or subobject
	 * @param	Class			The class to use for the specified subobject or component.
	 */
	const FObjectInitializer& SetNestedDefaultSubobjectClass(FStringView SubobjectName, const UClass* Class) const
	{
		AssertIfSubobjectSetupIsNotAllowed(SubobjectName);
		SubobjectOverrides.Add(SubobjectName, Class);
		return *this;
	}

	/**
	 * Sets the class to use for a subobject defined in a nested subobject, the class must be a subclass of the class used when calling CreateDefaultSubobject.
	 * @param	SubobjectName	path to the new component or subobject
	 * @param	Class			The class to use for the specified subobject or component.
	 */
	const FObjectInitializer& SetNestedDefaultSubobjectClass(TArrayView<const FName> SubobjectNames, const UClass* Class) const
	{
		AssertIfSubobjectSetupIsNotAllowed(SubobjectNames);
		SubobjectOverrides.Add(SubobjectNames, Class);
		return *this;
	}

	/**
	 * Sets the class to use for a subobject defined in a nested subobject, the class must be a subclass of the class used when calling CreateDefaultSubobject.
	 * @param	SubobjectName	path to the new component or subobject
	 */
	template<class T>
	const FObjectInitializer& SetNestedDefaultSubobjectClass(FStringView SubobjectName) const
	{
		return SetNestedDefaultSubobjectClass(SubobjectName, T::StaticClass());
	}

	/**
	 * Sets the class to use for a subobject defined in a nested subobject, the class must be a subclass of the class used when calling CreateDefaultSubobject.
	 * @param	SubobjectName	path to the new component or subobject
	 */
	template<class T>
	const FObjectInitializer& SetNestedDefaultSubobjectClass(TArrayView<const FName> SubobjectNames) const
	{
		return SetNestedDefaultSubobjectClass(SubobjectNames, T::StaticClass());
	}

	/**
	 * Indicates that a subobject should not create a component if created using CreateOptionalDefaultSubobject
	 * @param	SubobjectName	name of the new component or subobject to not create
	 */
	const FObjectInitializer& DoNotCreateNestedDefaultSubobject(FStringView SubobjectName) const
	{
		AssertIfSubobjectSetupIsNotAllowed(SubobjectName);
		SubobjectOverrides.Add(SubobjectName, nullptr);
		return *this;
	}

	/**
	 * Indicates that a subobject should not create a component if created using CreateOptionalDefaultSubobject
	 * @param	SubobjectName	name of the new component or subobject to not create
	 */
	const FObjectInitializer& DoNotCreateNestedDefaultSubobject(TArrayView<const FName> SubobjectNames) const
	{
		AssertIfSubobjectSetupIsNotAllowed(SubobjectNames);
		SubobjectOverrides.Add(SubobjectNames, nullptr);
		return *this;
	}

	/**
	 * Asserts with the specified message if code is executed inside UObject constructor
	 **/
	static void AssertIfInConstructor(UObject* Outer, const TCHAR* ErrorMessage);

	FORCEINLINE void FinalizeSubobjectClassInitialization()
	{
		bSubobjectClassInitializationAllowed = false;
	}

	/** Gets ObjectInitializer for the currently constructed object. Can only be used inside of a constructor of UObject-derived class. */
	static FObjectInitializer& Get();

private:

	friend class UObject; 
	friend class FScriptIntegrationObjectHelper;

	template<class T>
	friend void InternalConstructor(const class FObjectInitializer& X);

	/**
	 * Binary initialize object properties to zero or defaults.
	 *
	 * @param	Obj					object to initialize data for
	 * @param	DefaultsClass		the class to use for initializing the data
	 * @param	DefaultData			the buffer containing the source data for the initialization
	 * @param	bCopyTransientsFromClassDefaults if true, copy the transients from the DefaultsClass defaults, otherwise copy the transients from DefaultData
	 */
	static void InitProperties(UObject* Obj, UClass* DefaultsClass, UObject* DefaultData, bool bCopyTransientsFromClassDefaults);

	bool IsInstancingAllowed() const;

	/**
	 * Calls InitProperties for any default subobjects created through this ObjectInitializer.
	 * @param bAllowInstancing	Indicates whether the object's components may be copied from their templates.
	 * @return true if there are any subobjects which require instancing.
	*/
	bool InitSubobjectProperties(bool bAllowInstancing) const;

	/**
	 * Create copies of the object's components from their templates.
	 * @param Class						Class of the object we are initializing
	 * @param bNeedInstancing			Indicates whether the object's components need to be instanced
	 * @param bNeedSubobjectInstancing	Indicates whether subobjects of the object's components need to be instanced
	 */
	void InstanceSubobjects(UClass* Class, bool bNeedInstancing, bool bNeedSubobjectInstancing) const;

	/** 
	 * Initializes a non-native property, according to the initialization rules. If the property is non-native
	 * and does not have a zero contructor, it is inialized with the default value.
	 * @param	Property			Property to be initialized
	 * @param	Data				Default data
	 * @return	Returns true if that property was a non-native one, otherwise false
	 */
	static bool InitNonNativeProperty(FProperty* Property, UObject* Data);
	
	/**
	 * Finalizes a constructed UObject by initializing properties, 
	 * instancing/initializing sub-objects, etc.
	 */
	void PostConstructInit();

private:

	/**  Little helper struct to manage overrides from derived classes **/
	struct FOverrides
	{
		/**  Add an override, make sure it is legal **/
		void Add(FName InComponentName, const UClass* InComponentClass, const TArrayView<const FName>* FullPath = nullptr);

		/**  Add a potentially nested override, make sure it is legal **/
		void Add(FStringView InComponentPath, const UClass* InComponentClass);

		/**  Add a potentially nested override, make sure it is legal **/
		void Add(TArrayView<const FName> InComponentPath, const UClass* InComponentClass, const TArrayView<const FName>* FullPath = nullptr);

		struct FOverrideDetails
		{
			const UClass* Class = nullptr;
			FOverrides* SubOverrides = nullptr;
		};

		/**  Retrieve an override, or TClassToConstructByDefault::StaticClass or nullptr if this was removed by a derived class **/
		FOverrideDetails Get(FName InComponentName, const UClass* ReturnType, const UClass* ClassToConstructByDefault, bool bOptional) const;

	private:
		static bool IsLegalOverride(const UClass* DerivedComponentClass, const UClass* BaseComponentClass);

		/**  Search for an override **/
		int32 Find(FName InComponentName) const
		{
			for (int32 Index = 0 ; Index < Overrides.Num(); Index++)
			{
				if (Overrides[Index].ComponentName == InComponentName)
				{
					return Index;
				}
			}
			return INDEX_NONE;
		}
		/**  Element of the override array **/
		struct FOverride
		{
			FName ComponentName;
			const UClass* ComponentClass = nullptr;
			TUniquePtr<FOverrides> SubOverrides;
			bool bDoNotCreate = false;
			FOverride(FName InComponentName)
				: ComponentName(InComponentName)
			{
			}

			FOverride& operator=(const FOverride& Other)
			{
				ComponentName = Other.ComponentName;
				ComponentClass = Other.ComponentClass;
				SubOverrides = (Other.SubOverrides ? MakeUnique<FOverrides>(*Other.SubOverrides) : nullptr);
				bDoNotCreate = Other.bDoNotCreate;
				return *this;
			}

			FOverride(const FOverride& Other)
			{
				*this = Other;
			}

			FOverride(FOverride&&) = default;
			FOverride& operator=(FOverride&&) = default;
		};
		/**  The override array **/
		TArray<FOverride, TInlineAllocator<8> > Overrides;
	};
	/**  Little helper struct to manage overrides from derived classes **/
	struct FSubobjectsToInit
	{
		/**  Add a subobject **/
		void Add(UObject* Subobject, UObject* Template)
		{
			for (int32 Index = 0; Index < SubobjectInits.Num(); Index++)
			{
				check(SubobjectInits[Index].Subobject != Subobject);
			}
			new (SubobjectInits) FSubobjectInit(Subobject, Template);
		}
		/**  Element of the SubobjectInits array **/
		struct FSubobjectInit
		{
			UObject* Subobject;
			UObject* Template;
			FSubobjectInit(UObject* InSubobject, UObject* InTemplate)
				: Subobject(InSubobject)
				, Template(InTemplate)
			{
			}
		};
		/**  The SubobjectInits array **/
		TArray<FSubobjectInit, TInlineAllocator<8> > SubobjectInits;
	};

	/** Asserts if SetDefaultSubobjectClass or DoNotCreateOptionalDefaultSuobject are called inside of the constructor body */
	void AssertIfSubobjectSetupIsNotAllowed(const FName SubobjectName) const;

	/** Asserts if SetDefaultSubobjectClass or DoNotCreateOptionalDefaultSuobject are called inside of the constructor body */
	void AssertIfSubobjectSetupIsNotAllowed(FStringView SubobjectName) const;

	/** Asserts if SetDefaultSubobjectClass or DoNotCreateOptionalDefaultSuobject are called inside of the constructor body */
	void AssertIfSubobjectSetupIsNotAllowed(TArrayView<const FName> SubobjectNames) const;

	/**  object to initialize, from static allocate object, after construction **/
	UObject* Obj;
	/**  object to copy properties from **/
	UObject* ObjectArchetype;
	/**  if true, copy the transients from the DefaultsClass defaults, otherwise copy the transients from DefaultData **/
	bool bCopyTransientsFromClassDefaults;
	/**  If true, initialize the properties **/
	bool bShouldInitializePropsFromArchetype;
	/**  Only true until ObjectInitializer has not reached the base UObject class */
	bool bSubobjectClassInitializationAllowed = true;
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	/**  */
	bool bIsDeferredInitializer = false;
#endif // USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	/**  Instance graph **/
	struct FObjectInstancingGraph* InstanceGraph;
	/**  List of component classes to override from derived classes **/
	mutable FOverrides SubobjectOverrides;
	/**  List of component classes to initialize after the C++ constructors **/
	mutable FSubobjectsToInit ComponentInits;
#if !UE_BUILD_SHIPPING
	/** List of all subobject names constructed for this object */
	mutable TArray<FName, TInlineAllocator<8>> ConstructedSubobjects;
#endif
	/**  Previously constructed object in the callstack */
	UObject* LastConstructedObject = nullptr;

	/** Callback for custom property initialization before PostInitProperties gets called */
	TFunction<void()> PropertyInitCallback;

	friend struct FStaticConstructObjectParameters;

#if WITH_EDITORONLY_DATA
	/** Detects when a new GC object was created */
	void OnGCObjectCreated(FGCObject* InObject);

	/** Delegate handle for OnGCObjectCreated callback */
	FDelegateHandle OnGCObjectCreatedHandle;
	/** List of FGCObjects created during UObject construction */
	TArray<FGCObject*> CreatedGCObjects;
#endif // WITH_EDITORONLY_DATA
};

/**
 * This struct is used for passing parameter values to the StaticConstructObject_Internal() method.  Only the constructor parameters are required to
 * be valid - all other members are optional.
 */
struct FStaticConstructObjectParameters
{
	/** The class of the object to create */
	const UClass* Class;

	/** The object to create this object within (the Outer property for the new object will be set to the value specified here). */
	UObject* Outer;

	/** The name to give the new object.If no value(NAME_None) is specified, the object will be given a unique name in the form of ClassName_#. */
	FName Name;

	/** The ObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object. */
	EObjectFlags SetFlags = RF_NoFlags;

	/** The InternalObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object. */
	EInternalObjectFlags InternalSetFlags = EInternalObjectFlags::None;

	/** If true, copy transient from the class defaults instead of the pass in archetype ptr(often these are the same) */
	bool bCopyTransientsFromClassDefaults = false;

	/** If true, Template is guaranteed to be an archetype */
	bool bAssumeTemplateIsArchetype = false;

	/**
	 * If specified, the property values from this object will be copied to the new object, and the new object's ObjectArchetype value will be set to this object.
	 * If nullptr, the class default object is used instead.
	 */
	UObject* Template = nullptr;

	/** Contains the mappings of instanced objects and components to their templates */
	FObjectInstancingGraph* InstanceGraph = nullptr;

	/** Assign an external Package to the created object if non-null */
	UPackage* ExternalPackage = nullptr;

	/** Callback for custom code to initialize properties before PostInitProperties runs */
	TFunction<void()> PropertyInitCallback;

private:
	FObjectInitializer::FOverrides* SubobjectOverrides = nullptr;

public:

	COREUOBJECT_API FStaticConstructObjectParameters(const UClass* InClass);

	friend FObjectInitializer;
};

/**
* Helper class for script integrations to access some UObject innards. Needed for script-generated UObject classes
*/
class FScriptIntegrationObjectHelper
{
public:
	/**
	* Binary initialize object properties to zero or defaults.
	*
	* @param	ObjectInitializer	FObjectInitializer helper
	* @param	Obj					object to initialize data for
	* @param	DefaultsClass		the class to use for initializing the data
	* @param	DefaultData			the buffer containing the source data for the initialization
	*/
	inline static void InitProperties(const FObjectInitializer& ObjectInitializer, UObject* Obj, UClass* DefaultsClass, UObject* DefaultData)
	{
		FObjectInitializer::InitProperties(Obj, DefaultsClass, DefaultData, ObjectInitializer.bCopyTransientsFromClassDefaults);
	}

	/**
	* Calls InitProperties for any default subobjects created through this ObjectInitializer.
	* @param bAllowInstancing	Indicates whether the object's components may be copied from their templates.
	* @return true if there are any subobjects which require instancing.
	*/
	inline static bool InitSubobjectProperties(const FObjectInitializer& ObjectInitializer)
	{
		return ObjectInitializer.InitSubobjectProperties(ObjectInitializer.IsInstancingAllowed());
	}

	/**
	* Create copies of the object's components from their templates.
	* @param ObjectInitializer			FObjectInitializer helper
	* @param Class						Class of the object we are initializing
	* @param bNeedInstancing			Indicates whether the object's components need to be instanced
	* @param bNeedSubobjectInstancing	Indicates whether subobjects of the object's components need to be instanced
	*/
	inline static void InstanceSubobjects(const FObjectInitializer& ObjectInitializer, UClass* Class, bool bNeedInstancing, bool bNeedSubobjectInstancing)
	{
		ObjectInitializer.InstanceSubobjects(Class, bNeedInstancing, bNeedSubobjectInstancing);
	}

	/**
	 * Finalizes a constructed UObject by initializing properties, instancing &
	 * initializing sub-objects, etc.
	 * 
	 * @param  ObjectInitializer    The initializer to run PostConstructInit() on.
	 */
	inline static void PostConstructInitObject(FObjectInitializer& ObjectInitializer)
	{
		ObjectInitializer.PostConstructInit();
	}
};

#if DO_CHECK
/** Called by NewObject to make sure Child is actually a child of Parent */
COREUOBJECT_API void CheckIsClassChildOf_Internal(const UClass* Parent, const UClass* Child);
#endif

/**
 * Convenience template for constructing a gameplay object
 *
 * @param	Outer		the outer for the new object.  If not specified, object will be created in the transient package.
 * @param	Class		the class of object to construct
 * @param	Name		the name for the new object.  If not specified, the object will be given a transient name via MakeUniqueObjectName
 * @param	Flags		the object flags to apply to the new object
 * @param	Template	the object to use for initializing the new object.  If not specified, the class's default object will be used
 * @param	bCopyTransientsFromClassDefaults	if true, copy transient from the class defaults instead of the pass in archetype ptr (often these are the same)
 * @param	InInstanceGraph						contains the mappings of instanced objects and components to their templates
 * @param	ExternalPackage						Assign an external Package to the created object if non-null
 *
 * @return	a pointer of type T to a new object of the specified class
 */
template< class T >
FUNCTION_NON_NULL_RETURN_START
	T* NewObject(UObject* Outer, const UClass* Class, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool bCopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* ExternalPackage = nullptr)
FUNCTION_NON_NULL_RETURN_END
{
	if (Name == NAME_None)
	{
		FObjectInitializer::AssertIfInConstructor(Outer, TEXT("NewObject with empty name can't be used to create default subobjects (inside of UObject derived class constructor) as it produces inconsistent object names. Use ObjectInitializer.CreateDefaultSubobject<> instead."));
	}

#if DO_CHECK
	// Class was specified explicitly, so needs to be validated
	CheckIsClassChildOf_Internal(T::StaticClass(), Class);
#endif

	FStaticConstructObjectParameters Params(Class);
	Params.Outer = Outer;
	Params.Name = Name;
	Params.SetFlags = Flags;
	Params.Template = Template;
	Params.bCopyTransientsFromClassDefaults = bCopyTransientsFromClassDefaults;
	Params.InstanceGraph = InInstanceGraph;
	Params.ExternalPackage = ExternalPackage;
	return static_cast<T*>(StaticConstructObject_Internal(Params));
}

template< class T >
FUNCTION_NON_NULL_RETURN_START
	T* NewObject(UObject* Outer = (UObject*)GetTransientPackage())
FUNCTION_NON_NULL_RETURN_END
{
	// Name is always None for this case
	FObjectInitializer::AssertIfInConstructor(Outer, TEXT("NewObject with empty name can't be used to create default subobjects (inside of UObject derived class constructor) as it produces inconsistent object names. Use ObjectInitializer.CreateDefaultSubobject<> instead."));

	FStaticConstructObjectParameters Params(T::StaticClass());
	Params.Outer = Outer;
	return static_cast<T*>(StaticConstructObject_Internal(Params));
}

template< class T >
FUNCTION_NON_NULL_RETURN_START
	T* NewObject(UObject* Outer, FName Name, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool bCopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr)
FUNCTION_NON_NULL_RETURN_END
{
	if (Name == NAME_None)
	{
		FObjectInitializer::AssertIfInConstructor(Outer, TEXT("NewObject with empty name can't be used to create default subobjects (inside of UObject derived class constructor) as it produces inconsistent object names. Use ObjectInitializer.CreateDefaultSubobject<> instead."));
	}

	FStaticConstructObjectParameters Params(T::StaticClass());
	Params.Outer = Outer;
	Params.Name = Name;
	Params.SetFlags = Flags;
	Params.Template = Template;
	Params.bCopyTransientsFromClassDefaults = bCopyTransientsFromClassDefaults;
	Params.InstanceGraph = InInstanceGraph;
	return static_cast<T*>(StaticConstructObject_Internal(Params));
}

/**
 * Convenience template for duplicating an object
 *
 * @param SourceObject the object being copied
 * @param Outer the outer to use for the object
 * @param Name the optional name of the object
 *
 * @return the copied object or null if it failed for some reason
 */
template< class T >
T* DuplicateObject(T const* SourceObject,UObject* Outer, const FName Name = NAME_None)
{
	if (SourceObject != nullptr)
	{
		if (Outer == nullptr || Outer == INVALID_OBJECT)
		{
			Outer = (UObject*)GetTransientPackage();
		}
		return (T*)StaticDuplicateObject(SourceObject,Outer,Name);
	}
	return nullptr;
}
template <typename T>
T* DuplicateObject(const TObjectPtr<T>& SourceObject,UObject* Outer, const FName Name = NAME_None)
{
	if (SourceObject != nullptr)
	{
		if (Outer == nullptr || Outer == INVALID_OBJECT)
		{
			Outer = (UObject*)GetTransientPackage();
		}
		return (T*)StaticDuplicateObject(SourceObject,Outer,Name);
	}
	return nullptr;
}

/**
 * Determines whether the specified object should load values using PerObjectConfig rules
 */
COREUOBJECT_API bool UsesPerObjectConfig( UObject* SourceObject );

/**
 * Returns the file to load ini values from for the specified object, taking into account PerObjectConfig-ness
 */
COREUOBJECT_API FString GetConfigFilename( UObject* SourceObject );

/*----------------------------------------------------------------------------
	Core templates.
----------------------------------------------------------------------------*/

/** Parse a reference to an object from the input stream. */
template< class T > 
inline bool ParseObject( const TCHAR* Stream, const TCHAR* Match, T*& Obj, UObject* Outer, bool* bInvalidObject=nullptr )
{
	return ParseObject( Stream, Match, T::StaticClass(), (UObject*&)Obj, Outer, bInvalidObject );
}

/** 
 * Find an optional object, relies on the name being unqualified 
 * @see StaticFindObjectFast()
 */
template< class T > 
inline T* FindObjectFast( UObject* Outer, FName Name, bool ExactClass=false, bool AnyPackage=false, EObjectFlags ExclusiveFlags=RF_NoFlags )
{
	return (T*)StaticFindObjectFast( T::StaticClass(), Outer, Name, ExactClass, AnyPackage, ExclusiveFlags );
}

/**
 * Find an optional object.
 * @see StaticFindObject()
 */
template< class T > 
inline T* FindObject( UObject* Outer, const TCHAR* Name, bool ExactClass=false )
{
	return (T*)StaticFindObject( T::StaticClass(), Outer, Name, ExactClass );
}

/**
 * Find an optional object, no failure allowed
 * @see StaticFindObjectChecked()
 */
template< class T > 
inline T* FindObjectChecked( UObject* Outer, const TCHAR* Name, bool ExactClass=false )
{
	return (T*)StaticFindObjectChecked( T::StaticClass(), Outer, Name, ExactClass );
}

/**
 * Find an object without asserting on GIsSavingPackage or IsGarbageCollecting()
 * @see StaticFindObjectSafe()
 */
template< class T > 
inline T* FindObjectSafe( UObject* Outer, const TCHAR* Name, bool ExactClass=false )
{
	return (T*)StaticFindObjectSafe( T::StaticClass(), Outer, Name, ExactClass );
}

/** 
 * Load an object. 
 * @see StaticLoadObject()
 */
template< class T > 
inline T* LoadObject( UObject* Outer, const TCHAR* Name, const TCHAR* Filename=nullptr, uint32 LoadFlags=LOAD_None, UPackageMap* Sandbox=nullptr )
{
	return (T*)StaticLoadObject( T::StaticClass(), Outer, Name, Filename, LoadFlags, Sandbox );
}

/**
 * Load a class object
 * @see StaticLoadClass
 */
template< class T > 
inline UClass* LoadClass( UObject* Outer, const TCHAR* Name, const TCHAR* Filename=nullptr, uint32 LoadFlags=LOAD_None, UPackageMap* Sandbox=nullptr )
{
	return StaticLoadClass( T::StaticClass(), Outer, Name, Filename, LoadFlags, Sandbox );
}

/** 
 * Get default object of a class.
 * @see UClass::GetDefaultObject()
 */
template< class T > 
inline const T* GetDefault()
{
	return (const T*)T::StaticClass()->GetDefaultObject();
}

/**
 * Get default object of a class.
 * @see Class.h
 */
template< class T > 
inline const T* GetDefault(UClass *Class);

/** Version of GetDefault() that allows modification */
template< class T >
inline T* GetMutableDefault()
{
	return (T*)T::StaticClass()->GetDefaultObject();
}

/** Version of GetDefault() that allows modification */
template< class T > 
inline T* GetMutableDefault(UClass *Class);

/** Returns true if a class has been loaded (e.g. it has a CDO) */
template< class T >
inline bool IsClassLoaded()
{
	return T::StaticClass()->GetDefaultObject(false) != nullptr;
}

/**
 * Looks for delegate signature with given name.
 */
COREUOBJECT_API UFunction* FindDelegateSignature(FName DelegateSignatureName);

/**
 * Determines whether the specified array contains objects of the specified class.
 *
 * @param	ObjectArray		the array to search - must be an array of pointers to instances of a UObject-derived class
 * @param	ClassToCheck	the object class to search for
 * @param	bExactClass		true to consider only those objects that have the class specified, or false to consider objects
 *							of classes derived from the specified SearhClass as well
 * @param	out_Objects		if specified, any objects that match the SearchClass will be added to this array
 */
template <class T>
bool ContainsObjectOfClass( const TArray<T*>& ObjectArray, UClass* ClassToCheck, bool bExactClass=false, TArray<T*>* out_Objects=nullptr )
{
	bool bResult = false;
	for ( int32 ArrayIndex = 0; ArrayIndex < ObjectArray.Num(); ArrayIndex++ )
	{
		if ( ObjectArray[ArrayIndex] != nullptr )
		{
			bool bMatchesSearchCriteria = bExactClass
				? ObjectArray[ArrayIndex]->GetClass() == ClassToCheck
				: ObjectArray[ArrayIndex]->IsA(ClassToCheck);

			if ( bMatchesSearchCriteria )
			{
				bResult = true;
				if ( out_Objects != nullptr )
				{
					out_Objects->Add(ObjectArray[ArrayIndex]);
				}
				else
				{
					// if we don't need a list of objects that match the search criteria, we can stop as soon as we find at least one object of that class
					break;
				}
			}
		}
	}

	return bResult;
}

/**
 * Utility struct for restoring object flags for all objects.
 */
class FScopedObjectFlagMarker
{
	struct FStoredObjectFlags
	{
		FStoredObjectFlags()
		: Flags(RF_NoFlags)
		, InternalFlags(EInternalObjectFlags::None)
		{}
		FStoredObjectFlags(EObjectFlags InFlags, EInternalObjectFlags InInternalFlags)
			: Flags(InFlags)
			, InternalFlags(InInternalFlags)
		{}
		EObjectFlags Flags;
		EInternalObjectFlags InternalFlags;
	};

	/**
	 * Map that tracks the ObjectFlags set on all objects; we use a map rather than iterating over all objects twice because FObjectIterator
	 * won't return objects that have RF_Unreachable set, and we may want to actually unset that flag.
	 */
	TMap<UObject*, FStoredObjectFlags> StoredObjectFlags;
	
	/**
	 * Stores the object flags for all objects in the tracking array.
	 */
	void SaveObjectFlags();

	/**
	 * Restores the object flags for all objects from the tracking array.
	 */
	void RestoreObjectFlags();

public:
	/** Constructor */
	FScopedObjectFlagMarker()
	{
		SaveObjectFlags();
	}

	/** Destructor */
	~FScopedObjectFlagMarker()
	{
		RestoreObjectFlags();
	}
};


/**
  * Iterator for arrays of UObject pointers
  * @param TObjectClass		type of pointers contained in array
*/
template<class TObjectClass>
class TObjectArrayIterator
{
	/* sample code
	TArray<APawn *> TestPawns;
	...
	// test iterator, all items
	for ( TObjectArrayIterator<APawn> It(TestPawns); It; ++It )
	{
		UE_LOG(LogUObjectGlobals, Log, TEXT("Item %s"),*It->GetFullName());
	}
	*/
public:
	/**
		* Constructor, iterates all non-null, non pending kill objects, optionally of a particular class or base class
		* @param	InArray			the array to iterate on
		* @param	InClass			if non-null, will only iterate on items IsA this class
		* @param	InbExactClass	if true, will only iterate on exact matches
		*/
	FORCEINLINE TObjectArrayIterator( TArray<TObjectClass*>& InArray, UClass* InClassToCheck = nullptr, bool InbExactClass = false) :	
		Array(InArray),
		Index(-1),
		ClassToCheck(InClassToCheck),
		bExactClass(InbExactClass)
	{
		Advance();
	}
	/**
		* Iterator advance
		*/
	FORCEINLINE void operator++()
	{
		Advance();
	}
	/** conversion to "bool" returning true if the iterator is valid. */
	FORCEINLINE explicit operator bool() const
	{ 
		return Index < Array.Num(); 
	}
	/** inverse of the "bool" operator */
	FORCEINLINE bool operator !() const 
	{
		return !(bool)*this;
	}

	/**
		* Dereferences the iterator 
		* @return	the UObject at the iterator
	*/
	FORCEINLINE TObjectClass& operator*() const
	{
		checkSlow(GetObject());
		return *GetObject();
	}
	/**
		* Dereferences the iterator 
		* @return	the UObject at the iterator
	*/
	FORCEINLINE TObjectClass* operator->() const
	{
		checkSlow(GetObject());
		return GetObject();
	}

	/** 
	  * Removes the current element from the array, slower, but preserves the order. 
	  * Iterator is decremented for you so a loop will check all items.
	*/
	FORCEINLINE void RemoveCurrent()
	{
		Array.RemoveAt(Index--);
	}
	/** 
	  * Removes the current element from the array, faster, but does not preserves the array order. 
	  * Iterator is decremented for you so a loop will check all items.
	*/
	FORCEINLINE void RemoveCurrentSwap()
	{
		Array.RemoveSwap(Index--);
	}

protected:
	/**
		* Dereferences the iterator with an ordinary name for clarity in derived classes
		* @return	the UObject at the iterator
	*/
	FORCEINLINE TObjectClass* GetObject() const 
	{ 
		return Array(Index);
	}
	/**
		* Iterator advance with ordinary name for clarity in subclasses
		* @return	true if the iterator points to a valid object, false if iteration is complete
	*/
	FORCEINLINE bool Advance()
	{
		while(++Index < Array.Num())
		{
			TObjectClass* At = GetObject();
			if (
				IsValid(At) && 
				(!ClassToCheck ||
					(bExactClass
						? At->GetClass() == ClassToCheck
						: At->IsA(ClassToCheck)))
				)
			{
				return true;
			}
		}
		return false;
	}
private:
	/** The array that we are iterating on */
	TArray<TObjectClass*>&	Array;
	/** Index of the current element in the object array */
	int32						Index;
	/** Class using as a criteria */
	UClass*					ClassToCheck;
	/** Flag to require exact class matches */
	bool					bExactClass;
};

/** Base class for reference serialization archives */
class COREUOBJECT_API FReferenceCollectorArchive : public FArchiveUObject
{
	/** Object which is performing the serialization. */
	const UObject* SerializingObject;
	/** Object that owns the serialized data. */
	const UObject* SerializedDataContainer;
	/** Pointer to serialized data (read-only). */
	const void* SerializedDataPtr;
	/** Stored pointer to reference collector. */
	class FReferenceCollector& Collector;

protected:

	class FReferenceCollector& GetCollector()
	{
		return Collector;
	}

public:

	FReferenceCollectorArchive(const UObject* InSerializingObject, FReferenceCollector& InCollector)
		: SerializingObject(InSerializingObject)
		, SerializedDataContainer(nullptr)
		, SerializedDataPtr(nullptr)
		, Collector(InCollector)
	{
	}
	void SetSerializingObject(const UObject* InSerializingObject)
	{
		SerializingObject = InSerializingObject;
	}
	const UObject* GetSerializingObject() const
	{
		return SerializingObject;
	}
	void SetSerializedDataContainer(const UObject* InDataContainer)
	{
		SerializedDataContainer = InDataContainer;
	}
	const UObject* GetSerializedDataContainer() const
	{
		return SerializedDataContainer;
	}
	void SetSerializedDataPtr(const void* InSerializedDataPtr)
	{
		SerializedDataPtr = InSerializedDataPtr;
	}
	const void* GetSerializedDataPtr() const
	{
		return SerializedDataPtr;
	}
};

/** Helper class for setting and resetting attributes on the FReferenceCollectorArchive */
class COREUOBJECT_API FVerySlowReferenceCollectorArchiveScope
{	
	FReferenceCollectorArchive& Archive;
	const UObject* OldSerializingObject;
	FProperty* OldSerializedProperty;
	const UObject* OldSerializedDataContainer;
	const void* OldSerializedDataPtr;

public:
	FVerySlowReferenceCollectorArchiveScope(FReferenceCollectorArchive& InArchive, const UObject* InSerializingObject, FProperty* InSerializedProperty = nullptr, const UObject* InSerializedDataContainer = nullptr, const void* InSerializedDataPtr = nullptr)
		: Archive(InArchive)
		, OldSerializingObject(InArchive.GetSerializingObject())
		, OldSerializedProperty(InArchive.GetSerializedProperty())
		, OldSerializedDataContainer(InArchive.GetSerializedDataContainer())
		, OldSerializedDataPtr(InArchive.GetSerializedDataPtr())
	{
		Archive.SetSerializingObject(InSerializingObject);
		Archive.SetSerializedProperty(InSerializedProperty);
		Archive.SetSerializedDataContainer(InSerializedDataContainer);
		Archive.SetSerializedDataPtr(InSerializedDataPtr);
	}
	~FVerySlowReferenceCollectorArchiveScope()
	{
		Archive.SetSerializingObject(OldSerializingObject);
		Archive.SetSerializedProperty(OldSerializedProperty);
		Archive.SetSerializedDataContainer(OldSerializedDataContainer);
		Archive.SetSerializedDataPtr(OldSerializedDataPtr);
	}
	FReferenceCollectorArchive& GetArchive()
	{
		return Archive;
	}
};

/**
 * FReferenceCollector.
 * Helper class used by the garbage collector to collect object references.
 */
class COREUOBJECT_API FReferenceCollector
{
public:

	FReferenceCollector();
	virtual ~FReferenceCollector();

	/**
	 * Adds object reference.
	 *
	 * @param Object Referenced object.
	 * @param ReferencingObject Referencing object (if available).
	 * @param ReferencingProperty Referencing property (if available).
	 */
	template<class UObjectType>
	void AddReferencedObject(UObjectType*& Object, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		// @todo: should be uncommented when proper usage is fixed everywhere
		// static_assert(sizeof(UObjectType) > 0, "AddReferencedObject: Element must be a pointer to a fully-defined type");
		// static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObject: Element must be a pointer to a type derived from UObject");
		HandleObjectReference(*(UObject**)&Object, ReferencingObject, ReferencingProperty);
	}

	/**
	 * Adds const object reference, this reference can still be nulled out if forcefully collected.
	 *
	 * @param Object Referenced object.
	 * @param ReferencingObject Referencing object (if available).
	 * @param ReferencingProperty Referencing property (if available).
	 */
	template<class UObjectType>
	void AddReferencedObject(const UObjectType*& Object, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		// @todo: should be uncommented when proper usage is fixed everywhere
		// static_assert(sizeof(UObjectType) > 0, "AddReferencedObject: Element must be a pointer to a fully-defined type");
		// static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObject: Element must be a pointer to a type derived from UObject");
		HandleObjectReference(*(UObject**)const_cast<UObjectType**>(&Object), ReferencingObject, ReferencingProperty);
	}

	/**
	* Adds references to an array of objects.
	*
	* @param ObjectArray Referenced objects array.
	* @param ReferencingObject Referencing object (if available).
	* @param ReferencingProperty Referencing property (if available).
	*/
	template<class UObjectType>
	void AddReferencedObjects(TArray<UObjectType*>& ObjectArray, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(UObjectType) > 0, "AddReferencedObjects: Elements must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObjects: Elements must be pointers to a type derived from UObject");
		HandleObjectReferences(reinterpret_cast<UObject**>(ObjectArray.GetData()), ObjectArray.Num(), ReferencingObject, ReferencingProperty);
	}

	/**
	* Adds references to an array of const objects, these objects can still be nulled out if forcefully collected.
	*
	* @param ObjectArray Referenced objects array.
	* @param ReferencingObject Referencing object (if available).
	* @param ReferencingProperty Referencing property (if available).
	*/
	template<class UObjectType>
	void AddReferencedObjects(TArray<const UObjectType*>& ObjectArray, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(UObjectType) > 0, "AddReferencedObjects: Elements must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObjects: Elements must be pointers to a type derived from UObject");
		HandleObjectReferences(reinterpret_cast<UObject**>(const_cast<UObjectType**>(ObjectArray.GetData())), ObjectArray.Num(), ReferencingObject, ReferencingProperty);
	}

	/**
	* Adds references to a set of objects.
	*
	* @param ObjectSet Referenced objects set.
	* @param ReferencingObject Referencing object (if available).
	* @param ReferencingProperty Referencing property (if available).
	*/
	template<class UObjectType>
	void AddReferencedObjects(TSet<UObjectType*>& ObjectSet, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(UObjectType) > 0, "AddReferencedObjects: Elements must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObjects: Elements must be pointers to a type derived from UObject");
		for (auto& Object : ObjectSet)
		{
			HandleObjectReference(*(UObject**)&Object, ReferencingObject, ReferencingProperty);
		}
	}

	/**
	 * Adds references to a map of objects.
	 *
	 * @param Map Referenced objects map.
	 * @param ReferencingObject Referencing object (if available).
	 * @param ReferencingProperty Referencing property (if available).
	 */
	template <typename KeyType, typename ValueType, typename Allocator, typename KeyFuncs>
	void AddReferencedObjects(TMapBase<KeyType*, ValueType, Allocator, KeyFuncs>& Map, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(KeyType) > 0, "AddReferencedObjects: Keys must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<KeyType, const UObjectBase>::Value, "AddReferencedObjects: Keys must be pointers to a type derived from UObject");
		for (auto& It : Map)
		{
			HandleObjectReference(*(UObject**)&It.Key, ReferencingObject, ReferencingProperty);
		}
	}
	template <typename KeyType, typename ValueType, typename Allocator, typename KeyFuncs>
	void AddReferencedObjects(TMapBase<KeyType, ValueType*, Allocator, KeyFuncs>& Map, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(ValueType) > 0, "AddReferencedObjects: Values must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<ValueType, const UObjectBase>::Value, "AddReferencedObjects: Values must be pointers to a type derived from UObject");
		for (auto& It : Map)
		{
			HandleObjectReference(*(UObject**)&It.Value, ReferencingObject, ReferencingProperty);
		}
	}
	template <typename KeyType, typename ValueType, typename Allocator, typename KeyFuncs>
	void AddReferencedObjects(TMapBase<KeyType*, ValueType*, Allocator, KeyFuncs>& Map, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(KeyType) > 0, "AddReferencedObjects: Keys must be pointers to a fully-defined type");
		static_assert(sizeof(ValueType) > 0, "AddReferencedObjects: Values must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<KeyType, const UObjectBase>::Value, "AddReferencedObjects: Keys must be pointers to a type derived from UObject");
		static_assert(TPointerIsConvertibleFromTo<ValueType, const UObjectBase>::Value, "AddReferencedObjects: Values must be pointers to a type derived from UObject");
		for (auto& It : Map)
		{
			HandleObjectReference(*(UObject**)&It.Key, ReferencingObject, ReferencingProperty);
			HandleObjectReference(*(UObject**)&It.Value, ReferencingObject, ReferencingProperty);
		}
	}



	/**
	 * Adds any applicable references from a UScriptStruct. Only necessary to handle cases of an unreflected/non-UPROPERTY struct that wants to have references emitted.
	 *
	 * @param ScriptStruct Script struct to add referenced objects from
	 * @param StructMemory Memory of struct instance
	 * @param ReferencingObject Referencing object (if available).
	 * @param ReferencingProperty Referencing property (if available).
	 */
	void AddReferencedObjects(const class UScriptStruct*& ScriptStruct, void* StructMemory, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr);

	/**
	 * Adds object reference.
	 *
	 * @param Object Referenced object.
	 * @param ReferencingObject Referencing object (if available).
	 * @param ReferencingProperty Referencing property (if available).
	 */
	template<class UObjectType>
	void AddReferencedObject(TObjectPtr<UObjectType>& Object, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		if (IsObjectHandleResolved(Object.GetHandle()))
		{
			HandleObjectReference(*reinterpret_cast<UObject**>(&Object), ReferencingObject, ReferencingProperty);
		}
	}

	/**
	 * Adds const object reference, this reference can still be nulled out if forcefully collected.
	 *
	 * @param Object Referenced object.
	 * @param ReferencingObject Referencing object (if available).
	 * @param ReferencingProperty Referencing property (if available).
	 */
	template<class UObjectType>
	void AddReferencedObject(TObjectPtr<const UObjectType>& Object, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(UObjectType) > 0, "AddReferencedObject: Element must be a pointer to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObject: Element must be a pointer to a type derived from UObject");
		if (IsObjectHandleResolved(Object.GetHandle()))
		{
			HandleObjectReference(*reinterpret_cast<UObject**>(&Object), ReferencingObject, ReferencingProperty);
		}
	}

	/**
	* Adds references to an array of objects.
	*
	* @param ObjectArray Referenced objects array.
	* @param ReferencingObject Referencing object (if available).
	* @param ReferencingProperty Referencing property (if available).
	*/
	template<class UObjectType>
	void AddReferencedObjects(TArray<TObjectPtr<UObjectType>>& ObjectArray, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(UObjectType) > 0, "AddReferencedObjects: Elements must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObjects: Elements must be pointers to a type derived from UObject");
		// Cannot use a reinterpret_cast due to MSVC (and not Clang) emitting a warning:
		// C4946: reinterpret_cast used between related classes: ...
		HandleObjectReferences((FObjectPtr*)(ObjectArray.GetData()), ObjectArray.Num(), ReferencingObject, ReferencingProperty);
	}

	/**
	* Adds references to an array of const objects, these objects can still be nulled out if forcefully collected.
	*
	* @param ObjectArray Referenced objects array.
	* @param ReferencingObject Referencing object (if available).
	* @param ReferencingProperty Referencing property (if available).
	*/
	template<class UObjectType>
	void AddReferencedObjects(TArray<TObjectPtr<const UObjectType>>& ObjectArray, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(UObjectType) > 0, "AddReferencedObjects: Elements must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObjects: Elements must be pointers to a type derived from UObject");
		// Cannot use a reinterpret_cast due to MSVC (and not Clang) emitting a warning:
		// C4946: reinterpret_cast used between related classes: ...
		HandleObjectReferences((FObjectPtr*)(ObjectArray.GetData()), ObjectArray.Num(), ReferencingObject, ReferencingProperty);
	}

	/**
	* Adds references to a set of objects.
	*
	* @param ObjectSet Referenced objects set.
	* @param ReferencingObject Referencing object (if available).
	* @param ReferencingProperty Referencing property (if available).
	*/
	template<class UObjectType>
	void AddReferencedObjects(TSet<TObjectPtr<UObjectType>>& ObjectSet, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(UObjectType) > 0, "AddReferencedObjects: Elements must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<UObjectType, const UObjectBase>::Value, "AddReferencedObjects: Elements must be pointers to a type derived from UObject");
		for (auto& Object : ObjectSet)
		{
			if (IsObjectHandleResolved(Object.GetHandle()))
			{
				HandleObjectReference(*reinterpret_cast<UObject**>(&Object), ReferencingObject, ReferencingProperty);
			}
		}
	}

	/**
	 * Adds references to a map of objects.
	 *
	 * @param ObjectArray Referenced objects map.
	 * @param ReferencingObject Referencing object (if available).
	 * @param ReferencingProperty Referencing property (if available).
	 */
	template <typename KeyType, typename ValueType, typename Allocator, typename KeyFuncs>
	void AddReferencedObjects(TMapBase<TObjectPtr<KeyType>, ValueType, Allocator, KeyFuncs>& Map, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(KeyType) > 0, "AddReferencedObjects: Keys must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<KeyType, const UObjectBase>::Value, "AddReferencedObjects: Keys must be pointers to a type derived from UObject");
		for (auto& It : Map)
		{
			if (IsObjectHandleResolved(It.Key.GetHandle()))
			{
				HandleObjectReference(*reinterpret_cast<UObject**>(&It.Key), ReferencingObject, ReferencingProperty);
			}
		}
	}
	template <typename KeyType, typename ValueType, typename Allocator, typename KeyFuncs>
	void AddReferencedObjects(TMapBase<KeyType, TObjectPtr<ValueType>, Allocator, KeyFuncs>& Map, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(ValueType) > 0, "AddReferencedObjects: Values must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<ValueType, const UObjectBase>::Value, "AddReferencedObjects: Values must be pointers to a type derived from UObject");
		for (auto& It : Map)
		{
			if (IsObjectHandleResolved(It.Value.GetHandle()))
			{
				HandleObjectReference(*reinterpret_cast<UObject**>(&It.Value), ReferencingObject, ReferencingProperty);
			}
		}
	}
	template <typename KeyType, typename ValueType, typename Allocator, typename KeyFuncs>
	void AddReferencedObjects(TMapBase<TObjectPtr<KeyType>, TObjectPtr<ValueType>, Allocator, KeyFuncs>& Map, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr)
	{
		static_assert(sizeof(KeyType) > 0, "AddReferencedObjects: Keys must be pointers to a fully-defined type");
		static_assert(sizeof(ValueType) > 0, "AddReferencedObjects: Values must be pointers to a fully-defined type");
		static_assert(TPointerIsConvertibleFromTo<KeyType, const UObjectBase>::Value, "AddReferencedObjects: Keys must be pointers to a type derived from UObject");
		static_assert(TPointerIsConvertibleFromTo<ValueType, const UObjectBase>::Value, "AddReferencedObjects: Values must be pointers to a type derived from UObject");
		for (auto& It : Map)
		{
			if (IsObjectHandleResolved(It.Key.GetHandle()))
			{
				HandleObjectReference(*reinterpret_cast<UObject**>(&It.Key), ReferencingObject, ReferencingProperty);
			}
			if (IsObjectHandleResolved(It.Value.GetHandle()))
			{
				HandleObjectReference(*reinterpret_cast<UObject**>(&It.Value), ReferencingObject, ReferencingProperty);
			}
		}
	}

	/**
	 * If true archetype references should not be added to this collector.
	 */
	virtual bool IsIgnoringArchetypeRef() const = 0;
	/**
	 * If true transient objects should not be added to this collector.
	 */
	virtual bool IsIgnoringTransient() const = 0;
	/**
	 * Allows reference elimination by this collector.
	 */
	virtual void AllowEliminatingReferences(bool bAllow) {}
	/**
	 * Sets the property that is currently being serialized
	 */
	virtual void SetSerializedProperty(class FProperty* Inproperty) {}
	/**
	 * Gets the property that is currently being serialized
	 */
	virtual class FProperty* GetSerializedProperty() const { return nullptr; }
	/** 
	 * Marks a specific object reference as a weak reference. This does not affect GC but will be freed at a later point
	 * The default behavior returns false as weak references must be explicitly supported
	 */
	virtual bool MarkWeakObjectReferenceForClearing(UObject** WeakReference) { return false; }
	/**
	 * Sets whether this collector is currently processing native references or not.
	 */
	virtual void SetIsProcessingNativeReferences(bool bIsNative) {}
	/**
	 * If true, this collector is currently processing native references (true by default).
	 */
	virtual bool IsProcessingNativeReferences() const { return true; }

	/**
	* Returns the collector archive associated with this collector.
	* NOTE THAT COLLECTING REFERENCES THROUGH SERIALIZATION IS VERY SLOW.
	*/
	FReferenceCollectorArchive& GetVerySlowReferenceCollectorArchive()
	{
		if (!DefaultReferenceCollectorArchive)
		{
			CreateVerySlowReferenceCollectorArchive();
		}
		return *DefaultReferenceCollectorArchive;
	}

	/**
	* INTERNAL USE ONLY: returns the persistent frame collector archive associated with this collector.
	* NOTE THAT COLLECTING REFERENCES THROUGH SERIALIZATION IS VERY SLOW.
	*/
	FReferenceCollectorArchive& GetInternalPersistentFrameReferenceCollectorArchive()
	{
		if (!PersistentFrameReferenceCollectorArchive)
		{
			CreatePersistentFrameReferenceCollectorArchive();
		}
		return *PersistentFrameReferenceCollectorArchive;
	}

protected:
	/**
	 * Handle object reference. Called by AddReferencedObject.
	 *
	 * @param Object Referenced object.
	 * @param ReferencingObject Referencing object (if available).
	 * @param ReferencingProperty Referencing property (if available).
	 */
	virtual void HandleObjectReference(UObject*& InObject, const UObject* InReferencingObject, const FProperty* InReferencingProperty) = 0;

	/**
	* Handle multiple object references. Called by AddReferencedObjects.
	* DEFAULT IMPLEMENTATION IS SLOW as it calls HandleObjectReference multiple times. In order to optimize it, provide your own implementation.
	*
	* @param Object Referenced object.
	* @param ReferencingObject Referencing object (if available).
	* @param ReferencingProperty Referencing property (if available).
	*/
	virtual void HandleObjectReferences(UObject** InObjects, const int32 ObjectNum, const UObject* InReferencingObject, const FProperty* InReferencingProperty)
	{
		for (int32 ObjectIndex = 0; ObjectIndex < ObjectNum; ++ObjectIndex)
		{
			UObject*& Object = InObjects[ObjectIndex];
			HandleObjectReference(Object, InReferencingObject, InReferencingProperty);
		}
	}

	/**
	* Handle multiple object references. Called by AddReferencedObjects.
	* DEFAULT IMPLEMENTATION IS SLOW as it calls HandleObjectReference multiple times. In order to optimize it, provide your own implementation.
	*
	* @param Object Referenced object.
	* @param ReferencingObject Referencing object (if available).
	* @param ReferencingProperty Referencing property (if available).
	*/
	virtual void HandleObjectReferences(FObjectPtr* InObjects, const int32 ObjectNum, const UObject* InReferencingObject, const FProperty* InReferencingProperty);

private:

	/** Creates the roxy archive that uses serialization to add objects to this collector */
	void CreateVerySlowReferenceCollectorArchive();
	/** Creates persistent frame proxy archive that uses serialization to add objects to this collector */
	void CreatePersistentFrameReferenceCollectorArchive();

	/** Default proxy archive that uses serialization to add objects to this collector */
	FReferenceCollectorArchive* DefaultReferenceCollectorArchive;
	/** Persistent frame proxy archive that uses serialization to add objects to this collector */
	FReferenceCollectorArchive* PersistentFrameReferenceCollectorArchive;
};

/**
 * FReferenceFinder.
 * Helper class used to collect object references.
 */
class COREUOBJECT_API FReferenceFinder : public FReferenceCollector
{
public:

	/**
	 * Constructor
	 *
	 * @param InObjectArray Array to add object references to
	 * @param	InOuter					value for LimitOuter
	 * @param	bInRequireDirectOuter	value for bRequireDirectOuter
	 * @param	bShouldIgnoreArchetype	whether to disable serialization of ObjectArchetype references
	 * @param	bInSerializeRecursively	only applicable when LimitOuter != nullptr && bRequireDirectOuter==true;
	 *									serializes each object encountered looking for subobjects of referenced
	 *									objects that have LimitOuter for their Outer (i.e. nested subobjects/components)
	 * @param	bShouldIgnoreTransient	true to skip serialization of transient properties
	 */
	FReferenceFinder(TArray<UObject*>& InObjectArray, UObject* InOuter = nullptr, bool bInRequireDirectOuter = true, bool bInShouldIgnoreArchetype = false, bool bInSerializeRecursively = false, bool bInShouldIgnoreTransient = false);

	/**
	 * Finds all objects referenced by Object.
	 *
	 * @param Object Object which references are to be found.
	 * @param ReferencingObject object that's referencing the current object.
	 * @param ReferencingProperty property the current object is being referenced through.
	 */
	virtual void FindReferences(UObject* Object, UObject* ReferencingObject = nullptr, FProperty* ReferencingProperty = nullptr);

	// FReferenceCollector interface.
	virtual void HandleObjectReference(UObject*& Object, const UObject* ReferencingObject, const FProperty* InReferencingProperty) override;
	virtual bool IsIgnoringArchetypeRef() const override { return bShouldIgnoreArchetype; }
	virtual bool IsIgnoringTransient() const override { return bShouldIgnoreTransient; }
	virtual void SetSerializedProperty(class FProperty* Inproperty) override
	{
		SerializedProperty = Inproperty;
	}
	virtual class FProperty* GetSerializedProperty() const override
	{
		return SerializedProperty;
	}
protected:

	/** Stored reference to array of objects we add object references to. */
	TArray<UObject*>&		ObjectArray;
	/** List of objects that have been recursively serialized. */
	TSet<const UObject*>	SerializedObjects;
	/** Only objects within this outer will be considered, nullptr value indicates that outers are disregarded. */
	UObject*		LimitOuter;
	/** Property that is referencing the current object */
	class FProperty* SerializedProperty;
	/** Determines whether nested objects contained within LimitOuter are considered. */
	bool			bRequireDirectOuter;
	/** Determines whether archetype references are considered. */
	bool			bShouldIgnoreArchetype;
	/** Determines whether we should recursively look for references of the referenced objects. */
	bool			bSerializeRecursively;
	/** Determines whether transient references are considered. */
	bool			bShouldIgnoreTransient;
};

/** Defined in PackageReload.h */
enum class EPackageReloadPhase : uint8;
class FPackageReloadedEvent;

class FGarbageCollectionTracer;

enum class EHotReloadedClassFlags
{
	None = 0,

	// Set when the hot reloaded class has been detected as changed
	Changed = 0x01
};

ENUM_CLASS_FLAGS(EHotReloadedClassFlags)

enum class EReloadCompleteReason
{
	None,
	HotReloadAutomatic,
	HotReloadManual,
};

/**
 * Global CoreUObject delegates
 */
struct COREUOBJECT_API FCoreUObjectDelegates
{
#if WITH_EDITOR
	/** Callback for object property modifications, called by UObject::PostEditChangeProperty with a single property event */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnObjectPropertyChanged, UObject*, struct FPropertyChangedEvent&);
	static FOnObjectPropertyChanged OnObjectPropertyChanged;

	/** Callback for object property modifications, called by UObject::PreEditChange with a full property chain */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPreObjectPropertyChanged, UObject*, const class FEditPropertyChain&);
	static FOnPreObjectPropertyChanged OnPreObjectPropertyChanged;

	/** Called when an object is registered for change with UObject::Modify. This gets called in both the editor and standalone game editor builds, for every object modified */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnObjectModified, UObject*);
	static FOnObjectModified OnObjectModified;

	/** Set of objects modified this frame, to prevent multiple triggerings of the OnObjectModified delegate */
	static TSet<UObject*> ObjectsModifiedThisFrame;

	/** Broadcast OnObjectModified if the broadcast hasn't occurred for this object in this frame */
	static void BroadcastOnObjectModified(UObject* Object)
	{
		if (OnObjectModified.IsBound() && !ObjectsModifiedThisFrame.Contains(Object))
		{
			ObjectsModifiedThisFrame.Add(Object);
			OnObjectModified.Broadcast(Object);
		}
	}

	/** Callback for an object being transacted */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnObjectTransacted, UObject*, const class FTransactionObjectEvent&);
	static FOnObjectTransacted OnObjectTransacted;

	/** Called when UObjects have been replaced to allow others a chance to fix their references */
	using FReplacementObjectMap = TMap<UObject*, UObject*>; // Alias for use in the macro
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnObjectsReplaced, const FReplacementObjectMap&);
	static FOnObjectsReplaced OnObjectsReplaced;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnObjectSaved, UObject*);
	UE_DEPRECATED(5.0, "Use OnObjectPreSave instead.")
	static FOnObjectSaved OnObjectSaved;

	/** Callback for when an asset is saved. This is called from UObject::PreSave before it is actually written to disk, for every object saved */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnObjectPreSave, UObject*, FObjectPreSaveContext);
	static FOnObjectPreSave OnObjectPreSave;

	/** Callback for when an asset is loaded. This gets called in both the editor and standalone game editor builds, but only for objects that return true for IsAsset() */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssetLoaded, UObject*);
	static FOnAssetLoaded OnAssetLoaded;

	/** Callback when packages end loading in LoadPackage or AsyncLoadPackage. All packages loaded recursively due to imports are included in the single call of the explicitly-loaded package. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnEndLoadPackage, TConstArrayView<UPackage*>);
	static FOnEndLoadPackage OnEndLoadPackage;

	/** Delegate used by SavePackage() to create the package backup */
	DECLARE_DELEGATE_RetVal_OneParam(bool, FAutoPackageBackupDelegate, const UPackage&);
	static FAutoPackageBackupDelegate AutoPackageBackupDelegate;
#endif // WITH_EDITOR

	/** Called by ReloadPackage during package reloading. It will be called several times for different phases of fix-up to allow custom code to handle updating objects as needed */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPackageReloaded, EPackageReloadPhase, FPackageReloadedEvent*);
	static FOnPackageReloaded OnPackageReloaded;

	/** Called when a package reload request is received from a network file server */
	DECLARE_DELEGATE_OneParam(FNetworkFileRequestPackageReload, const TArray<FString>& /*PackageNames*/);
	static FNetworkFileRequestPackageReload NetworkFileRequestPackageReload;

	/** Delegate used by SavePackage() to check whether a package should be saved */
	DECLARE_DELEGATE_RetVal_ThreeParams(bool, FIsPackageOKToSaveDelegate, UPackage*, const FString&, FOutputDevice*);
	static FIsPackageOKToSaveDelegate IsPackageOKToSaveDelegate;

	/** Delegate for reloaded classes that have been added. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FReloadAddedClassesDelegate, const TArray<UClass*>&);
	static FReloadAddedClassesDelegate ReloadAddedClassesDelegate;

	/** Delegate for reload re-instancing complete */
	DECLARE_MULTICAST_DELEGATE(FReloadReinstancingCompleteDelegate);
	static FReloadReinstancingCompleteDelegate ReloadReinstancingCompleteDelegate;

	/** Delegate for reload re-instancing complete */
	DECLARE_MULTICAST_DELEGATE_OneParam(FReloadCompleteDelegate, EReloadCompleteReason);
	static FReloadCompleteDelegate ReloadCompleteDelegate;

	/** Delegate for registering hot-reloaded classes that have been added  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FRegisterHotReloadAddedClassesDelegate, const TArray<UClass*>&);
	UE_DEPRECATED(5.0, "RegisterHotReloadAddedClassesDelegate has been deprecated, use ReloadAddedClassesDelegate.")
	static FRegisterHotReloadAddedClassesDelegate RegisterHotReloadAddedClassesDelegate;

	/** Delegate for registering hot-reloaded classes that changed after hot-reload for reinstancing */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FRegisterClassForHotReloadReinstancingDelegate, UClass*, UClass*, EHotReloadedClassFlags);
	UE_DEPRECATED(5.0, "RegisterClassForHotReloadReinstancingDelegate has been deprecated, use FReload for class re-instancing.")
	static FRegisterClassForHotReloadReinstancingDelegate RegisterClassForHotReloadReinstancingDelegate;

	/** Delegate for reinstancing hot-reloaded classes */
	DECLARE_MULTICAST_DELEGATE(FReinstanceHotReloadedClassesDelegate);
	UE_DEPRECATED(5.0, "ReinstanceHotReloadedClassesDelegate has been deprecated, use FReload for class re-instancing or ReloadReinstancingCompleteDelegate for notification")
	static FReinstanceHotReloadedClassesDelegate ReinstanceHotReloadedClassesDelegate;

	/** Delegate for catching when UClasses/UStructs/UEnums would be available via FindObject<>(), but before their CDOs would be constructed. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FCompiledInUObjectsRegisteredDelegate, FName /*Package*/);
	static FCompiledInUObjectsRegisteredDelegate CompiledInUObjectsRegisteredDelegate;

	/** Sent at the very beginning of LoadMap */
	DECLARE_MULTICAST_DELEGATE_OneParam(FPreLoadMapDelegate, const FString& /* MapName */);
	static FPreLoadMapDelegate PreLoadMap;

	/** Sent at the very beginning of LoadMap */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FPreLoadMapWithContextDelegate, const FWorldContext& /*WorldContext*/, const FString& /* MapName */);
	static FPreLoadMapWithContextDelegate PreLoadMapWithContext;
	
	/** Sent at the end of LoadMap */
	DECLARE_MULTICAST_DELEGATE_OneParam(FPostLoadMapDelegate, UWorld* /* LoadedWorld */);
	static FPostLoadMapDelegate PostLoadMapWithWorld;

	/** Sent when a network replay has started */
	static FSimpleMulticastDelegate PostDemoPlay;

	/** Called before garbage collection */
	static FSimpleMulticastDelegate& GetPreGarbageCollectDelegate();

	/** Delegate type for reachability analysis external roots callback. First parameter is FGarbageCollectionTracer to use for tracing, second is flags with which objects should be kept alive regardless, third is whether to force single threading */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FTraceExternalRootsForReachabilityAnalysisDelegate, FGarbageCollectionTracer&, EObjectFlags, bool);

	/** Called as last phase of reachability analysis. Allow external systems to add UObject roots *after* first reachability pass has been done */
	static FTraceExternalRootsForReachabilityAnalysisDelegate TraceExternalRootsForReachabilityAnalysis;

	/** Called after reachability analysis, before any purging */
	static FSimpleMulticastDelegate PostReachabilityAnalysis;

	/** Called after garbage collection */
	static FSimpleMulticastDelegate& GetPostGarbageCollect();

	/** Called before ConditionalBeginDestroy phase of garbage collection */
	static FSimpleMulticastDelegate PreGarbageCollectConditionalBeginDestroy;

	/** Called after ConditionalBeginDestroy phase of garbage collection */
	static FSimpleMulticastDelegate PostGarbageCollectConditionalBeginDestroy;

	/** Queries whether an object should be loaded on top ( replace ) an already existing one */
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnLoadObjectsOnTop, const FString&);
	static FOnLoadObjectsOnTop ShouldLoadOnTop;

	/** Called when path to world root is changed */
	DECLARE_MULTICAST_DELEGATE_OneParam(FPackageCreatedForLoad, class UPackage*);
	static FPackageCreatedForLoad PackageCreatedForLoad;

	/** Called when trying to figure out if a UObject is a primary asset, if it doesn't implement GetPrimaryAssetId itself */
	DECLARE_DELEGATE_RetVal_OneParam(FPrimaryAssetId, FGetPrimaryAssetIdForObject, const UObject*);
	static FGetPrimaryAssetIdForObject GetPrimaryAssetIdForObject;

	/** Called during cooking to see if a specific package should be cooked for a given target platform */
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FShouldCookPackageForPlatform, const UPackage*, const ITargetPlatform*);
	static FShouldCookPackageForPlatform ShouldCookPackageForPlatform;
};

/** Allows release builds to override not verifying GC assumptions. Useful for profiling as it's hitchy. */
extern COREUOBJECT_API bool GShouldVerifyGCAssumptions;

/** If non-zero, the engine will create Garbage Collector clusters to speed up Garbage Collection */
extern COREUOBJECT_API int32 GCreateGCClusters;

/** If non-zero, the engine will attempt to create clusters from asset files */
extern COREUOBJECT_API int32 GAssetClustreringEnabled;

/** A struct used as stub for deleted ones. */
COREUOBJECT_API UScriptStruct* GetFallbackStruct();
// @todo: BP2CPP_remove
enum class UE_DEPRECATED(5.0, "This type is no longer in use and will be removed.") EConstructDynamicType : uint8
{
	OnlyAllocateClassObject,
	CallZConstructor
};

// @todo: BP2CPP_remove
/** Constructs dynamic type of a given class. */
PRAGMA_DISABLE_DEPRECATION_WARNINGS
UE_DEPRECATED(5.0, "This method is no longer in use and will be removed.")
COREUOBJECT_API UObject* ConstructDynamicType(FName TypePathName, EConstructDynamicType ConstructionSpecifier);
PRAGMA_ENABLE_DEPRECATION_WARNINGS

// @todo: BP2CPP_remove
/** Given a dynamic type path name, returns that type's class name (can be either DynamicClass, ScriptStruct or Enum). */
UE_DEPRECATED(5.0, "This method is no longer in use and will be removed.")
COREUOBJECT_API FName GetDynamicTypeClassName(FName TypePathName);

// @todo: BP2CPP_remove
/** Finds or constructs a package for dynamic type. */
UE_DEPRECATED(5.0, "This method is no longer in use and will be removed.")
COREUOBJECT_API UPackage* FindOrConstructDynamicTypePackage(const TCHAR* PackageName);

// @todo: BP2CPP_remove
/** Get names of "virtual" packages, that contain Dynamic types  */
UE_DEPRECATED(5.0, "This method is no longer in use and will be removed.")
COREUOBJECT_API TMap<FName, FName>& GetConvertedDynamicPackageNameToTypeName();

/** Utility accessor for whether we are running with component class overrides enabled */
COREUOBJECT_API bool GetAllowNativeComponentClassOverrides();

// @todo: BP2CPP_remove
struct COREUOBJECT_API UE_DEPRECATED(5.0, "This type is no longer in use and will be removed.") FDynamicClassStaticData
{
	/** Autogenerated Z_Construct* function pointer */
	UClass* (*ZConstructFn)();
	/** StaticClass() function pointer */
	UClass* (*StaticClassFn)();
	/** Selected AssetRegistrySearchable values */
	TMap<FName, FName> SelectedSearchableValues;
};

// @todo: BP2CPP_remove
/** Returns map of all dynamic/nativized classes */
PRAGMA_DISABLE_DEPRECATION_WARNINGS
UE_DEPRECATED(5.0, "This method is no longer in use and will be removed.")
COREUOBJECT_API TMap<FName, FDynamicClassStaticData>& GetDynamicClassMap();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

/**
 * FAssetMsg
 * This struct contains functions for asset-related messaging
 */
struct FAssetMsg
{
	/** Formats a path for the UE_ASSET_LOG macro */
	static COREUOBJECT_API FString FormatPathForAssetLog(const TCHAR* Path);

	/** If possible, finds a path to the underlying asset for the provided object and formats it for the UE_ASSET_LOG macro */
	static COREUOBJECT_API FString FormatPathForAssetLog(const UObject* Object);

	static COREUOBJECT_API FString GetAssetLogString(const TCHAR* Path, const FString& Message);
	static COREUOBJECT_API FString GetAssetLogString(const UObject* Object, const FString& Message);
};

#define ASSET_LOG_FORMAT_STRING TEXT("[AssetLog] %s: ")

#if NO_LOGGING
	#define UE_ASSET_LOG(...)
#else
	/**
	 * A macro that outputs a formatted message to log with a canonical reference to an asset if a given logging category is active at a given verbosity level
	 * @param CategoryName name of the logging category
	 * @param Verbosity, verbosity level to test against
	 * @param Asset, Object or asset path to format
	 * @param Format, format text
	 */
	#define UE_ASSET_LOG(CategoryName, Verbosity, Asset, Format, ...) \
	{ \
		static_assert(TIsArrayOrRefOfType<decltype(Format), TCHAR>::Value, "Formatting string must be a TCHAR array."); \
		static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategory##CategoryName::CompileTimeVerbosity) \
		{ \
			UE_LOG_EXPAND_IS_FATAL(Verbosity, PREPROCESSOR_NOTHING, if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity))) \
			{ \
				FString FormatPath = FAssetMsg::FormatPathForAssetLog(Asset);\
				FMsg::Logf_Internal(__FILE__, __LINE__, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, ASSET_LOG_FORMAT_STRING Format, *FormatPath, ##__VA_ARGS__); \
				UE_LOG_EXPAND_IS_FATAL(Verbosity, \
					{ \
						_DebugBreakAndPromptForRemote(); \
						FDebug::AssertFailed("", __FILE__, __LINE__, TEXT("%s: ") Format, *FormatPath, ##__VA_ARGS__); \
						CA_ASSUME(false); \
					}, \
					PREPROCESSOR_NOTHING \
				) \
			} \
		} \
	}
#endif // NO_LOGGING

#if WITH_EDITOR
/** 
 * Returns if true if the object is editor-only:
 * - it's a package marked as PKG_EditorOnly or inside one
 * or
 * - IsEditorOnly returns true
 * or
 * - if bCheckMarks is true, if it has the EditorOnly object mark
 * or 
 * - if bCheckRecursive is true, if it's class, outer, or archetypes are editor only
 */
COREUOBJECT_API bool IsEditorOnlyObject(const UObject* InObject, bool bCheckRecursive = true, bool bCheckMarks = true);
#endif //WITH_EDITOR

struct FClassFunctionLinkInfo;
struct FCppClassTypeInfoStatic;
class FFieldClass;

/// @cond DOXYGEN_IGNORE
namespace UECodeGen_Private
{
	enum class EPropertyGenFlags : uint32
	{
		None              = 0x00,

		// First 6 bits are the property type
		Byte              = 0x00,
		Int8              = 0x01,
		Int16             = 0x02,
		Int               = 0x03,
		Int64             = 0x04,
		UInt16            = 0x05,
		UInt32            = 0x06,
		UInt64            = 0x07,
		UnsizedInt        = 0x08,
		UnsizedUInt       = 0x09,
		Float             = 0x0A,
		Double            = 0x0B,
		Bool              = 0x0C,
		SoftClass         = 0x0D,
		WeakObject        = 0x0E,
		LazyObject        = 0x0F,
		SoftObject        = 0x10,
		Class             = 0x11,
		Object            = 0x12,
		Interface         = 0x13,
		Name              = 0x14,
		Str               = 0x15,
		Array             = 0x16,
		Map               = 0x17,
		Set               = 0x18,
		Struct            = 0x19,
		Delegate          = 0x1A,
		InlineMulticastDelegate = 0x1B,
		SparseMulticastDelegate = 0x1C,
		Text              = 0x1D,
		Enum              = 0x1E,
		FieldPath         = 0x1F,
		LargeWorldCoordinatesReal = 0x20,

		// Property-specific flags
		NativeBool        = 0x40,
		ObjectPtr         = 0x40,

	};

	ENUM_CLASS_FLAGS(EPropertyGenFlags)

	// Value which masks out the type of combined EPropertyGenFlags.
	constexpr EPropertyGenFlags PropertyTypeMask = (EPropertyGenFlags)0x3F;

#if WITH_METADATA
	struct FMetaDataPairParam
	{
		const char* NameUTF8;
		const char* ValueUTF8;
	};
#endif

	struct FEnumeratorParam
	{
		const char*               NameUTF8;
		int64                     Value;
	};

	// This is not a base class but is just a common initial sequence of all of the F*PropertyParams types below.
	// We don't want to use actual inheritance because we want to construct aggregated compile-time tables of these things.
	struct FPropertyParamsBase
	{
		const char*    NameUTF8;
		const char*       RepNotifyFuncUTF8;
		EPropertyFlags    PropertyFlags;
		EPropertyGenFlags Flags;
		EObjectFlags   ObjectFlags;
		int32          ArrayDim;
	};

	struct FPropertyParamsBaseWithOffset // : FPropertyParamsBase
	{
		const char*    NameUTF8;
		const char*       RepNotifyFuncUTF8;
		EPropertyFlags    PropertyFlags;
		EPropertyGenFlags Flags;
		EObjectFlags   ObjectFlags;
		int32          ArrayDim;
		int32          Offset;
	};

	struct FGenericPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*       RepNotifyFuncUTF8;
		EPropertyFlags    PropertyFlags;
		EPropertyGenFlags Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FBytePropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UEnum*         (*EnumFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FBoolPropertyParams // : FPropertyParamsBase
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		uint32           ElementSize;
		SIZE_T           SizeOfOuter;
		void           (*SetBitFunc)(void* Obj);
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FObjectPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UClass*        (*ClassFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FClassPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UClass*        (*MetaClassFunc)();
		UClass*        (*ClassFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FSoftClassPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UClass*        (*MetaClassFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FInterfacePropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UClass*        (*InterfaceClassFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FStructPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UScriptStruct* (*ScriptStructFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FDelegatePropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UFunction*     (*SignatureFunctionFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FMulticastDelegatePropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UFunction*     (*SignatureFunctionFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FEnumPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*        RepNotifyFuncUTF8;
		EPropertyFlags     PropertyFlags;
		EPropertyGenFlags  Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		UEnum*         (*EnumFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FFieldPathPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*      NameUTF8;
		const char*        RepNotifyFuncUTF8;
		EPropertyFlags     PropertyFlags;
		EPropertyGenFlags  Flags;
		EObjectFlags     ObjectFlags;
		int32            ArrayDim;
		int32            Offset;
		FFieldClass*     (*PropertyClassFunc)();
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FArrayPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*         NameUTF8;
		const char*         RepNotifyFuncUTF8;
		EPropertyFlags      PropertyFlags;
		EPropertyGenFlags   Flags;
		EObjectFlags        ObjectFlags;
		int32               ArrayDim;
		int32               Offset;
		EArrayPropertyFlags ArrayFlags;
#if WITH_METADATA
		const FMetaDataPairParam* MetaDataArray;
		int32                     NumMetaData;
#endif
	};

	struct FMapPropertyParams // : FPropertyParamsBaseWithOffset
	{
		const char*       NameUTF8;
		const char*       RepNotifyFuncUTF8;
		EPropertyFlags    PropertyFlags;
		EPropertyGenFlags Flags;
		EObjectFlags      ObjectFlags;
		int32             ArrayDim;
		int32             Offset;
		EMapPropertyFlags MapFlags;
#if WITH_METADATA
		const FMetaDataPairParam* MetaDataArray;
		int32                     NumMetaData;
#endif
	};

	// These property types don't add new any construction parameters to their base property
	typedef FGenericPropertyParams FInt8PropertyParams;
	typedef FGenericPropertyParams FInt16PropertyParams;
	typedef FGenericPropertyParams FIntPropertyParams;
	typedef FGenericPropertyParams FInt64PropertyParams;
	typedef FGenericPropertyParams FFInt16PropertyParams;
	typedef FGenericPropertyParams FUInt32PropertyParams;
	typedef FGenericPropertyParams FFInt64PropertyParams;
	typedef FGenericPropertyParams FUnsizedIntPropertyParams;
	typedef FGenericPropertyParams FUnsizedFIntPropertyParams;
	typedef FGenericPropertyParams FFloatPropertyParams;
	typedef FGenericPropertyParams FDoublePropertyParams;
	typedef FGenericPropertyParams FLargeWorldCoordinatesRealPropertyParams;
	typedef FGenericPropertyParams FNamePropertyParams;
	typedef FGenericPropertyParams FStrPropertyParams;
	typedef FGenericPropertyParams FSetPropertyParams;
	typedef FGenericPropertyParams FTextPropertyParams;
	typedef FObjectPropertyParams  FWeakObjectPropertyParams;
	typedef FObjectPropertyParams  FLazyObjectPropertyParams;
	typedef FObjectPropertyParams  FObjectPtrPropertyParams;
	typedef FClassPropertyParams   FClassPtrPropertyParams;
	typedef FObjectPropertyParams  FSoftObjectPropertyParams;

	struct FFunctionParams
	{
		UObject*                          (*OuterFunc)();
		UFunction*                        (*SuperFunc)();
		const char*                         NameUTF8;
		const char*                         OwningClassName;
		const char*                         DelegateName;
		SIZE_T                              StructureSize;
		const FPropertyParamsBase* const*   PropertyArray;
		int32                               NumProperties;
		EObjectFlags                        ObjectFlags;
		EFunctionFlags                      FunctionFlags;
		uint16                              RPCId;
		uint16                              RPCResponseId;
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FEnumParams
	{
		UObject*                  (*OuterFunc)();
		FText                     (*DisplayNameFunc)(int32);
		const char*                 NameUTF8;
		const char*                 CppTypeUTF8;
		const FEnumeratorParam*     EnumeratorParams;
		int32                       NumEnumerators;
		EObjectFlags                ObjectFlags;
		EEnumFlags                  EnumFlags;
		uint8                       CppForm; // this is of type UEnum::ECppForm
#if WITH_METADATA
		const FMetaDataPairParam*   MetaDataArray;
		int32                       NumMetaData;
#endif
	};

	struct FStructParams
	{
		UObject*                          (*OuterFunc)();
		UScriptStruct*                    (*SuperFunc)();
		void*                             (*StructOpsFunc)(); // really returns UScriptStruct::ICppStructOps*
		const char*                         NameUTF8;
		SIZE_T                              SizeOf;
		SIZE_T                              AlignOf;
		const FPropertyParamsBase* const*   PropertyArray;
		int32                               NumProperties;
		EObjectFlags                        ObjectFlags;
		uint32                              StructFlags; // EStructFlags
#if WITH_METADATA
		const FMetaDataPairParam*           MetaDataArray;
		int32                               NumMetaData;
#endif
	};

	struct FPackageParams
	{
		const char*                        NameUTF8;
		UObject*                  (*const *SingletonFuncArray)();
		int32                              NumSingletons;
		uint32                             PackageFlags; // EPackageFlags
		uint32                             BodyCRC;
		uint32                             DeclarationsCRC;
#if WITH_METADATA
		const FMetaDataPairParam*          MetaDataArray;
		int32                              NumMetaData;
#endif
	};

	struct FImplementedInterfaceParams
	{
		UClass* (*ClassFunc)();
		int32     Offset;
		bool      bImplementedByK2;
	};

	struct FClassParams
	{
		UClass*                                   (*ClassNoRegisterFunc)();
		const char*                                 ClassConfigNameUTF8;
		const FCppClassTypeInfoStatic*              CppClassInfo;
		UObject*                           (*const *DependencySingletonFuncArray)();
		const FClassFunctionLinkInfo*               FunctionLinkArray;
		const FPropertyParamsBase* const*           PropertyArray;
		const FImplementedInterfaceParams*          ImplementedInterfaceArray;
		int32                                       NumDependencySingletons;
		int32                                       NumFunctions;
		int32                                       NumProperties;
		int32                                       NumImplementedInterfaces;
		uint32                                      ClassFlags; // EClassFlags
#if WITH_METADATA
		const FMetaDataPairParam*                   MetaDataArray;
		int32                                       NumMetaData;
#endif
	};

	UE_DEPRECATED(5.0, "ConstructUFunction deprecated.  Please use the version of ConstructUFunction which retains the singleton pointer.")
	COREUOBJECT_API void ConstructUFunction(UFunction*& OutFunction, const FFunctionParams& Params);
	COREUOBJECT_API void ConstructUFunction(UFunction** SingletonPtr, const FFunctionParams& Params);
	COREUOBJECT_API void ConstructUEnum(UEnum*& OutEnum, const FEnumParams& Params);
	COREUOBJECT_API void ConstructUScriptStruct(UScriptStruct*& OutStruct, const FStructParams& Params);
	COREUOBJECT_API void ConstructUPackage(UPackage*& OutPackage, const FPackageParams& Params);
	COREUOBJECT_API void ConstructUClass(UClass*& OutClass, const FClassParams& Params);
}
/// @endcond

// METADATA_PARAMS(x, y) expands to x, y, if WITH_METADATA is set, otherwise expands to nothing
#if WITH_METADATA
	#define METADATA_PARAMS(x, y) x, y,
#else
	#define METADATA_PARAMS(x, y)
#endif

// IF_WITH_EDITOR(x, y) expands to x if WITH_EDITOR is set, otherwise expands to y
#if WITH_EDITOR
	#define IF_WITH_EDITOR(x, y) x
#else
	#define IF_WITH_EDITOR(x, y) y
#endif

// IF_WITH_EDITORONLY_DATA(x, y) expands to x if WITH_EDITORONLY_DATA is set, otherwise expands to y
#if WITH_EDITORONLY_DATA
	#define IF_WITH_EDITORONLY_DATA(x, y) x
#else
	#define IF_WITH_EDITORONLY_DATA(x, y) y
#endif

/** Enum used by DataValidation plugin to see if an asset has been validated for correctness */
enum class EDataValidationResult : uint8
{
	/** Asset has failed validation */
	Invalid,
	/** Asset has passed validation */
	Valid,
	/** Asset has not yet been validated */
	NotValidated
};

/**
 * Combines two different data validation results and returns the combined result.
 *
 * @param	Result1			One of the data validation results to be combined
 * @param	Result2			One of the data validation results to be combined
 *
 * @return	Returns the combined data validation result
 */
COREUOBJECT_API EDataValidationResult CombineDataValidationResults(EDataValidationResult Result1, EDataValidationResult Result2);
