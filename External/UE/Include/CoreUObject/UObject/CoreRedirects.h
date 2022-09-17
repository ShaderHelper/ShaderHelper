// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CoreRedirects.h: Object/Class/Field redirects read from ini files or registered at startup
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumClassFlags.h"
#include "HAL/CriticalSection.h"

class IPakFile;

#define WITH_COREREDIRECTS_MULTITHREAD_WARNING !UE_BUILD_SHIPPING && !IS_PROGRAM && !WITH_EDITOR

/** 
 * Flags describing the type and properties of this redirect
 */
enum class ECoreRedirectFlags : uint32
{
	None = 0,

	// Core type of the Thing being redirected, multiple can be set.  A Query will only find Redirects that have at least one of the same Type bits set.
	Type_Object =			0x00000001, // UObject
	Type_Class =			0x00000002, // UClass
	Type_Struct =			0x00000004, // UStruct
	Type_Enum =				0x00000008, // UEnum
	Type_Function =			0x00000010, // UFunction
	Type_Property =			0x00000020, // FProperty
	Type_Package =			0x00000040, // UPackage
	Type_AllMask =			0x0000FFFF, // Bit mask of all possible Types

	// Category flags.  A Query will only match Redirects that have the same value for every category bit.
	Category_InstanceOnly = 0x00010000, // Only redirect instances of this type, not the type itself
	Category_Removed =		0x00020000, // This type was explicitly removed, new name isn't valid
	Category_AllMask =		0x00FF0000, // Bit mask of all possible Categories

	// Option flags.  Does not behave as a bit-match between Queries and Redirects.  Each one specifies a custom rule for how FCoreRedirects handles the Redirect.
	Option_MatchSubstring = 0x01000000, // Does a slow substring match
	Option_MissingLoad =	0x02000000, // An automatically-created redirect that was created in response to a missing Thing during load. Redirect will be removed if and when the Thing is loaded.
	Option_AllMask =		0xFF000000, // Bit mask of all possible Options

	// Deprecated Names
	Option_InstanceOnly UE_DEPRECATED(4.25, "Use Category_InstanceOnly instead") = Category_InstanceOnly,
	Option_Removed UE_DEPRECATED(4.25, "Use Category_Removed instead") = Category_Removed,
	Option_ExactMatchMask UE_DEPRECATED(4.25, "Use Category_AllMask instead") = Category_AllMask,
};
ENUM_CLASS_FLAGS(ECoreRedirectFlags);


/**
 * An object path extracted into component names for matching. TODO merge with FSoftObjectPath?
 */
struct COREUOBJECT_API FCoreRedirectObjectName
{
	/** Raw name of object */
	FName ObjectName;

	/** String of outer chain, may be empty */
	FName OuterName;

	/** Package this was in before, may be extracted out of OldName */
	FName PackageName;

	/** Default to invalid names */
	FCoreRedirectObjectName() {}

	/** Copy constructor */
	FCoreRedirectObjectName(const FCoreRedirectObjectName& Other)
		: ObjectName(Other.ObjectName), OuterName(Other.OuterName), PackageName(Other.PackageName)
	{
	}

	/** Construct from FNames that are already expanded */
	FCoreRedirectObjectName(FName InObjectName, FName InOuterName, FName InPackageName)
		: ObjectName(InObjectName), OuterName(InOuterName), PackageName(InPackageName)
	{

	}

	/** Construct from a path string, this handles full paths with packages, or partial paths without */
	FCoreRedirectObjectName(const FString& InString);

	/** Construct from object in memory */
	FCoreRedirectObjectName(const class UObject* Object);

	/** Creates FString version */
	FString ToString() const;

	/** Sets back to invalid state */
	void Reset();

	/** Checks for exact equality */
	bool operator==(const FCoreRedirectObjectName& Other) const
	{
		return ObjectName == Other.ObjectName && OuterName == Other.OuterName && PackageName == Other.PackageName;
	}

	bool operator!=(const FCoreRedirectObjectName& Other) const
	{
		return !(*this == Other);
	}

	/** Returns true if the passed in name matches requirements, will ignore names that are none */
	bool Matches(const FCoreRedirectObjectName& Other, bool bCheckSubstring = false) const;

	/** Returns integer of degree of match. 0 if doesn't match at all, higher integer for better matches */
	int32 MatchScore(const FCoreRedirectObjectName& Other) const;

	/** Returns the name used as the key into the acceleration map */
	FName GetSearchKey(ECoreRedirectFlags Type) const
	{
		if ((Type & ECoreRedirectFlags::Option_MatchSubstring) == ECoreRedirectFlags::Option_MatchSubstring)
		{
			static FName SubstringName = FName(TEXT("*SUBSTRING*"));

			// All substring matches pass initial test as they need to be manually checked
			return SubstringName;
		}

		if ((Type & ECoreRedirectFlags::Type_Package) == ECoreRedirectFlags::Type_Package)
		{
			return PackageName;
		}

		return ObjectName;
	}

	/** Returns true if this refers to an actual object */
	bool IsValid() const
	{
		return ObjectName != NAME_None || PackageName != NAME_None;
	}

	/** Returns true if all names have valid characters */
	bool HasValidCharacters() const;

	/** Expand OldName/NewName as needed */
	static bool ExpandNames(const FString& FullString, FName& OutName, FName& OutOuter, FName &OutPackage);

	/** Turn it back into an FString */
	static FString CombineNames(FName NewName, FName NewOuter, FName NewPackage);
};

/** 
 * A single redirection from an old name to a new name, parsed out of an ini file
 */
struct COREUOBJECT_API FCoreRedirect
{
	/** Flags of this redirect */
	ECoreRedirectFlags RedirectFlags;

	/** Name of object to look for */
	FCoreRedirectObjectName OldName;

	/** Name to replace with */
	FCoreRedirectObjectName NewName;

	/** Change the class of this object when doing a redirect */
	FCoreRedirectObjectName OverrideClassName;

	/** Map of value changes, from old value to new value */
	TMap<FString, FString> ValueChanges;

	/** Construct from name strings, which may get parsed out */
	FCoreRedirect(const FCoreRedirect& Other)
		: RedirectFlags(Other.RedirectFlags), OldName(Other.OldName), NewName(Other.NewName), OverrideClassName(Other.OverrideClassName), ValueChanges(Other.ValueChanges)
	{
	}

	/** Construct from name strings, which may get parsed out */
	FCoreRedirect(ECoreRedirectFlags InRedirectFlags, FString InOldName, FString InNewName)
		: RedirectFlags(InRedirectFlags), OldName(InOldName), NewName(InNewName)
	{
		NormalizeNewName();
	}
	
	/** Construct parsed out object names */
	FCoreRedirect(ECoreRedirectFlags InRedirectFlags, const FCoreRedirectObjectName& InOldName, const FCoreRedirectObjectName& InNewName)
		: RedirectFlags(InRedirectFlags), OldName(InOldName), NewName(InNewName)
	{
		NormalizeNewName();
	}

	/** Normalizes NewName with data from OldName */
	void NormalizeNewName();

	/** Parses a char buffer into the ValueChanges map */
	const TCHAR* ParseValueChanges(const TCHAR* Buffer);

	/** Returns true if the passed in name and flags match requirements */
	bool Matches(ECoreRedirectFlags InFlags, const FCoreRedirectObjectName& InName) const;

	/** Returns true if this has value redirects */
	bool HasValueChanges() const;

	/** Returns true if this is a substring match */
	bool IsSubstringMatch() const;

	/** Convert to new names based on mapping */
	FCoreRedirectObjectName RedirectName(const FCoreRedirectObjectName& OldObjectName) const;

	/** See if search criteria is identical */
	bool IdenticalMatchRules(const FCoreRedirect& Other) const
	{
		return RedirectFlags == Other.RedirectFlags && OldName == Other.OldName;
	}

	/** Returns the name used as the key into the acceleration map */
	FName GetSearchKey() const
	{
		return OldName.GetSearchKey(RedirectFlags);
	}
};

/**
 * A container for all of the registered core-level redirects 
 */
struct COREUOBJECT_API FCoreRedirects
{
	/** Run initialization steps that are needed before any data can be stored in FCoreRedirects. Reads can occur before this, but no redirects will exist and redirect queries will all return empty. */
	static void Initialize();

	/** Returns a redirected version of the object name. If there are no valid redirects, it will return the original name */
	static FCoreRedirectObjectName GetRedirectedName(ECoreRedirectFlags Type, const FCoreRedirectObjectName& OldObjectName);

	/** Returns map of String->String value redirects for the object name, or nullptr if none found */
	static const TMap<FString, FString>* GetValueRedirects(ECoreRedirectFlags Type, const FCoreRedirectObjectName& OldObjectName);

	/** Performs both a name redirect and gets a value redirect struct if it exists. Returns true if either redirect found */
	static bool RedirectNameAndValues(ECoreRedirectFlags Type, const FCoreRedirectObjectName& OldObjectName, FCoreRedirectObjectName& NewObjectName, const FCoreRedirect** FoundValueRedirect);

	/** Returns true if this name has been registered as explicitly missing */
	static bool IsKnownMissing(ECoreRedirectFlags Type, const FCoreRedirectObjectName& ObjectName);

	/**
	  * Adds the given combination of (Type, ObjectName, Channel) as a missing name; IsKnownMissing queries will now find it
	  *
	  * @param Type Combination of the ECoreRedirectFlags::Type_* flags specifying the type of the object now known to be missing
	  * @param ObjectName The name of the object now known to be missing
	  * @param Channel may be Option_MissingLoad or Option_None; used to distinguish between detected-at-runtime and specified-by-ini
	  */
	static bool AddKnownMissing(ECoreRedirectFlags Type, const FCoreRedirectObjectName& ObjectName, ECoreRedirectFlags Channel = ECoreRedirectFlags::Option_MissingLoad);

	/**
	  * Removes the given combination of (Type, ObjectName, Channel) as a missing name
	  *
	  * @param Type Combination of the ECoreRedirectFlags::Type_* flags specifying the type of the object that has just been loaded.
	  * @param ObjectName The name of the object that has just been loaded.
	  * @param Channel may be Option_MissingLoad or Option_None; used to distinguish between detected-at-runtime and specified-by-ini
	  */
	static bool RemoveKnownMissing(ECoreRedirectFlags Type, const FCoreRedirectObjectName& ObjectName, ECoreRedirectFlags Channel = ECoreRedirectFlags::Option_MissingLoad);

	static void ClearKnownMissing(ECoreRedirectFlags Type, ECoreRedirectFlags Channel = ECoreRedirectFlags::Option_MissingLoad);

	/** Returns list of names it may have been before */
	static bool FindPreviousNames(ECoreRedirectFlags Type, const FCoreRedirectObjectName& NewObjectName, TArray<FCoreRedirectObjectName>& PreviousNames);

	/** Returns list of all core redirects that match requirements */
	static bool GetMatchingRedirects(ECoreRedirectFlags Type, const FCoreRedirectObjectName& OldObjectName, TArray<const FCoreRedirect*>& FoundRedirects);

	/** Parse all redirects out of a given ini file */
	static bool ReadRedirectsFromIni(const FString& IniName);

	/** Adds an array of redirects to global list */
	static bool AddRedirectList(TArrayView<const FCoreRedirect> Redirects, const FString& SourceString);

	/** Removes an array of redirects from global list */
	static bool RemoveRedirectList(TArrayView<const FCoreRedirect> Redirects, const FString& SourceString);

	/** Returns true if this has ever been initialized */
	static bool IsInitialized() { return bInitialized; }

	/** Gets map from config key -> Flags */
	static const TMap<FName, ECoreRedirectFlags>& GetConfigKeyMap() { return ConfigKeyMap; }

	/** Goes from the containing package and name of the type to the type flag */
	static ECoreRedirectFlags GetFlagsForTypeName(FName PackageName, FName TypeName);

	/** Goes from UClass Type to the type flag */
	static ECoreRedirectFlags GetFlagsForTypeClass(UClass *TypeClass);

	/** Runs set of redirector tests, returns false on failure */
	static bool RunTests();

private:
	/** Static only class, never constructed */
	FCoreRedirects();

	/** Add a single redirect to a type map */
	static bool AddSingleRedirect(const FCoreRedirect& NewRedirect, const FString& SourceString);

	/** Remove a single redirect from a type map */
	static bool RemoveSingleRedirect(const FCoreRedirect& OldRedirect, const FString& SourceString);

	/** Add native redirects, called before ini is parsed for the first time */
	static void RegisterNativeRedirects();

#if WITH_COREREDIRECTS_MULTITHREAD_WARNING
	/** Mark that CoreRedirects is about to start being used from multiple threads, and writes to new types of redirects are no longer allowed.
	  * ReadRedirectsFromIni and all other AddRedirectList calls must be called before this
	  */
	static void EnterMultithreadedPhase();
#endif

	/** There is one of these for each registered set of redirect flags */
	struct FRedirectNameMap
	{
		/** Map from name of thing being mapped to full list. List must be filtered further */
		TMap<FName, TArray<FCoreRedirect> > RedirectMap;
	};

	/** Whether this has been initialized at least once */
	static bool bInitialized;

#if WITH_COREREDIRECTS_MULTITHREAD_WARNING
	/** Whether CoreRedirects is now being used multithreaded and therefore does not support writes to RedirectTypeMap keyvalue pairs */
	static bool bIsInMultithreadedPhase;
#endif

	/** Map from config name to flag */
	static TMap<FName, ECoreRedirectFlags> ConfigKeyMap;

	/** Map from name of thing being mapped to full list. List must be filtered further */
	static TMap<ECoreRedirectFlags, FRedirectNameMap> RedirectTypeMap;

	/**
	 * Lock to protect multithreaded access to *KnownMissing functions, which can be called from the async loading threads. 
	 * TODO: The KnownMissing functions use RedirectTypeMap, which is unguarded; there is race condition vulnerability if asyncloading thread is active before all categories are added to RedirectTypeMap.
	 */
	static FRWLock KnownMissingLock;
};
