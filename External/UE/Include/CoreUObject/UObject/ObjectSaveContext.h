// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/UnrealString.h"
#include "HAL/Platform.h"

class FPackagePath;
class ITargetPlatform;
class UPackage;

/** Data used to provide information about the save parameters during PreSave/PostSave. */
struct FObjectSaveContextData
{
	FObjectSaveContextData() = default;
	/** Standard constructor; calculates derived fields from the given externally-specified fields. */
	COREUOBJECT_API FObjectSaveContextData(UPackage* Package, const ITargetPlatform* InTargetPlatform, const TCHAR* InTargetFilename, uint32 InSaveFlags);
	COREUOBJECT_API FObjectSaveContextData(UPackage* Package, const ITargetPlatform* InTargetPlatform, const FPackagePath& TargetPath, uint32 InSaveFlags);

	/** Set the fields set by the standard constructor. */
	COREUOBJECT_API void Set(UPackage* Package, const ITargetPlatform* InTargetPlatform, const TCHAR* InTargetFilename, uint32 InSaveFlags);
	COREUOBJECT_API void Set(UPackage* Package, const ITargetPlatform* InTargetPlatform, const FPackagePath& TargetPath, uint32 InSaveFlags);


	// Global Parameters that are read-only by the interfaces

	/**
	 * The target Filename being saved into (not the temporary file for saving).
	 * The path is in the standard UnrealEngine form - it is as a relative path from the process binary directory.
	 * Set to the empty string if the saved bytes are not being saved to a file.
	 */
	FString TargetFilename;

	/** The target platform of the save, if cooking. Null if not cooking. */
	const ITargetPlatform* TargetPlatform = nullptr;

	/** The save flags (ESaveFlags) of the save. */
	uint32 SaveFlags = 0;

	/** Package->GetPackageFlags before the save, or 0 if no package. */
	uint32 OriginalPackageFlags = 0;

	/**
	 * Set to true when the package is being saved due to a procedural save.
	 * Any save without the possibility of user-generated edits to the package is a procedural save (Cooking, EditorDomain).
	 * This allows us to execute transforms that only need to be executed in response to new user data.
	 */
	bool bProceduralSave = false;

	/**
	 * Set to true when the LoadedPath of the package being saved is being updated.
	 * This allows us to update the in-memory package when it is saved in editor to match its new save file.
	 */
	bool bUpdatingLoadedPath = false;

	/**
	 * Always true normally. When a system is executing multiple PreSaves/PostSaves concurrently before a single save,
	 * all but the first PreSaves have this set to false. If there are PostSaves they are executed in reverse order,
	 * and all but the last PostSave have this set to false.
	 */
	bool bOuterConcurrentSave = true;

	/** Set to false if the save failed, before calling any PostSaves. */
	bool bSaveSucceeded = true;


	// Per-object Output variables; writable from PreSave functions, readable from PostSave functions

	/** A bool that can be set from PreSave to indicate PostSave needs to take some extra cleanup steps. */
	bool bCleanupRequired = false;


	// Variables set/read per call to PreSave/PostSave functions
	/** PreSave contract enforcement; records whether PreSave is overridden. */
	int32 NumRefPasses = 0;

	/** Call-site enforcement; records whether the base PreSave was called. */
	bool bBaseClassCalled = false;

};

/** Interface used by PreSave to access the save parameters. */
class FObjectPreSaveContext
{
public:
	explicit FObjectPreSaveContext(FObjectSaveContextData& InData)
		: Data(InData)
	{
		++Data.NumRefPasses; // Record the number of copies; used to check whether PreSave is overridden
	}

	FObjectPreSaveContext(const FObjectPreSaveContext& Other)
		: Data(Other.Data)
	{
		++Data.NumRefPasses; // Record the number of copies; used to check whether PreSave is overridden
	}

	/**
	 * The target Filename being saved into (not the temporary file for saving).
	 * The path is in the standard UnrealEngine form - it is as a relative path from the process binary directory.
	 * Empty string if the saved bytes are not being saved to a file. Never null.
	 */
	const TCHAR* GetTargetFilename() const { return *Data.TargetFilename; }

	/** Report whether this is a save into a target-specific cooked format. */
	bool IsCooking() const { return Data.TargetPlatform != nullptr; }

	/** Return the targetplatform of the save, if cooking. Null if not cooking. */
	const ITargetPlatform* GetTargetPlatform() const { return Data.TargetPlatform; }

	/**
	 * Return whether the package is being saved due to a procedural save.
	 * Any save without the possibility of user-generated edits to the package is a procedural save (Cooking, EditorDomain).
	 * This allows us to execute transforms that only need to be executed in response to new user data.
	 */
	bool IsProceduralSave() const { return Data.bProceduralSave; }

	/**
	 * Return whether LoadedPath of the package being saved is being updated.
	 * This allows us to update the in-memory package when it is saved in editor to match its new save file.
	 */
	bool IsUpdatingLoadedPath() const { return Data.bUpdatingLoadedPath; }

	/** Return the save flags (ESaveFlags) of the save. */
	uint32 GetSaveFlags() const { return Data.SaveFlags; }

	/**
	 * Always true normally. When a system is executing multiple PreSaves concurrently before a single save,
	 * will return false for all but the first PreSave.
	 */
	bool IsFirstConcurrentSave() const { return Data.bOuterConcurrentSave; }

protected:
	FObjectSaveContextData& Data;
	friend class UObject;
};

/** Interface used by PostSave to access the save parameters. */
class FObjectPostSaveContext
{
public:
	explicit FObjectPostSaveContext(FObjectSaveContextData& InData)
		: Data(InData)
	{
		++Data.NumRefPasses; // Record the number of copies; used to check whether PreSave is overridden
	}

	FObjectPostSaveContext(const FObjectPostSaveContext& Other)
		: Data(Other.Data)
	{
		++Data.NumRefPasses; // Record the number of copies; used to check whether PreSave is overridden
	}

	/**
	 * The target Filename being saved into (not the temporary file for saving).
	 * The path is in the standard UnrealEngine form - it is as a relative path from the process binary directory.
	 * Empty string if the saved bytes are not being saved to a file. Never null.
	 */
	const TCHAR* GetTargetFilename() const { return *Data.TargetFilename; }

	/** Report whether this is a save into a target-specific cooked format. */
	bool IsCooking() const { return Data.TargetPlatform != nullptr; }

	/** Return the targetplatform of the save, if cooking. Null if not cooking. */
	const ITargetPlatform* GetTargetPlatform() const { return Data.TargetPlatform; }

	/**
	 * Return whether the package is being saved due to a procedural save.
	 * Any save without the possibility of user-generated edits to the package is a procedural save (Cooking, EditorDomain).
	 * This allows us to execute transforms that only need to be executed in response to new user data.
	 */
	bool IsProceduralSave() const { return Data.bProceduralSave; }

	/**
	 * Return whether LoadedPath of the package being saved is being updated.
	 * This allows us to update the in-memory package when it is saved in editor to match its new save file.
	 */
	bool IsUpdatingLoadedPath() const { return Data.bUpdatingLoadedPath; }

	/** Return the save flags (ESaveFlags) of the save. */
	uint32 GetSaveFlags() const { return Data.SaveFlags; }

	/** Package->GetPackageFlags before the save, or 0 if no package. */
	uint32 GetOriginalPackageFlags() const { return Data.OriginalPackageFlags; }

	/** Return whether the Save was successful. Note that some PostSave operations are only called when this is true. */
	bool SaveSucceeded() const { return Data.bSaveSucceeded; }

	/**
	 * Always true normally. When a system is executing multiple PreSaves and PostSaves concurrently before a single save,
	 * PostSaves are executed in reverse order of the PreSaves, and this function returns false for all but the last one.
	 */
	bool IsLastConcurrentSave() const { return Data.bOuterConcurrentSave; }

protected:
	FObjectSaveContextData& Data;
	friend class UObject;
};

/** Interface used by PreSaveRoot to access the save parameters. */
class FObjectPreSaveRootContext : public FObjectPreSaveContext
{
public:
	explicit FObjectPreSaveRootContext(FObjectSaveContextData& InData)
		: FObjectPreSaveContext(InData)
	{
	}

	FObjectPreSaveRootContext(const FObjectPreSaveRootContext& Other)
		: FObjectPreSaveContext(Other.Data)
	{
	}

	/** Set whether PostSaveRoot needs to take extra cleanup steps (false by default). */
	void SetCleanupRequired(bool bCleanupRequired) { Data.bCleanupRequired = bCleanupRequired; }

};

/** Interface used by PostSaveRoot to access the save parameters. */
class FObjectPostSaveRootContext : public FObjectPostSaveContext
{
public:
	explicit FObjectPostSaveRootContext(FObjectSaveContextData& InData)
		: FObjectPostSaveContext(InData)
	{
	}

	FObjectPostSaveRootContext(const FObjectPostSaveRootContext& Other)
		: FObjectPostSaveContext(Other.Data)
	{
	}

	/** Return whether PreSaveRoot indicated PostSaveRoot needs to take extra cleanup steps. */
	bool IsCleanupRequired() const { return Data.bCleanupRequired; }
};

