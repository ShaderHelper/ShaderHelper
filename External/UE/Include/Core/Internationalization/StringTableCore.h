// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Internationalization/StringTableCoreFwd.h"
#include "Containers/Map.h"
#include "UObject/NameTypes.h"
#include "Containers/UnrealString.h"
#include "Internationalization/Text.h"
#include "Internationalization/LocKeyFuncs.h"
#include "Internationalization/Internationalization.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStringTable, Log, All);

/** Entry within a string table */
class CORE_API FStringTableEntry
{
public:
	/** Create a new string table entry using the given data */
	static FStringTableEntryRef NewStringTableEntry(FStringTableConstRef InOwnerTable, FString InSourceString, FTextId InDisplayStringId)
	{
		return MakeShared<FStringTableEntry, ESPMode::ThreadSafe>(MoveTemp(InOwnerTable), MoveTemp(InSourceString), MoveTemp(InDisplayStringId));
	}

	/** Default constructor */
	FStringTableEntry();

	/** Create a new string table entry using the given data */
	FStringTableEntry(FStringTableConstRef InOwnerTable, FString InSourceString, FTextId InDisplayStringId);

	/** @return true if this entry is currently owned by a string table, false if it's been disowned (and should be re-cached) */
	bool IsOwned() const;

	/** Disown this string table entry. This is used to notify external code that has cached this entry that it needs to re-cache it from the string table */
	void Disown();

	/** Is this string table entry owned by the given string table? */
	bool IsOwnedBy(const FStringTable& InStringTable) const;

	/** Get the source string of this string table entry */
	const FString& GetSourceString() const;

	/** Get the display string of this string table entry */
	FTextConstDisplayStringPtr GetDisplayString() const;

	/** Get the display string ID of this string table entry */
	FTextId GetDisplayStringId() const;

	/** Get the placeholder source string to use for string table entries that are missing */
	static const FString& GetPlaceholderSourceString();

private:
	/** The string table that owns us (if any) */
	FStringTableConstWeakPtr OwnerTable;

	/** The source string of this entry */
	FString SourceString;

	/** The display string ID of this entry */
	FTextId DisplayStringId;
};

/** String table implementation. Holds Key->SourceString pairs of text. */
class CORE_API FStringTable : public TSharedFromThis<FStringTable, ESPMode::ThreadSafe>
{
public:
	/** Create a new string table */
	static FStringTableRef NewStringTable()
	{
		return MakeShared<FStringTable, ESPMode::ThreadSafe>();
	}

	/** Default constructor */
	FStringTable();

	/** Destructor */
	~FStringTable();

	/** @return The asset that owns this string table instance (if any) */
	UStringTable* GetOwnerAsset() const;

	/** Set the asset that owns this string table instance (if any) */
	void SetOwnerAsset(UStringTable* InOwnerAsset);

	/** Has this string table been fully loaded yet? (used during asset loading) */
	bool IsLoaded() const;

	/** Set whether this string table has been fully loaded yet */
	void IsLoaded(const bool bInIsLoaded);

	/** @return The namespace used by all entries in this string table */
	FString GetNamespace() const;

	/** Set the namespace used by all entries in this string table */
	void SetNamespace(const FString& InNamespace);

	/** Get the source string used by the given entry (if any) */
	bool GetSourceString(const FTextKey& InKey, FString& OutSourceString) const;

	/** Set the source string used by the given entry (will replace any existing data for that entry) */
	void SetSourceString(const FTextKey& InKey, const FString& InSourceString);

	/** Remove the given entry (including its meta-data) */
	void RemoveSourceString(const FTextKey& InKey);

	/** Enumerate all source strings in the table. Return true from the enumerator to continue, or false to stop */
	void EnumerateSourceStrings(const TFunctionRef<bool(const FString&, const FString&)>& InEnumerator) const;
	void EnumerateKeysAndSourceStrings(const TFunctionRef<bool(const FTextKey&, const FString&)>& InEnumerator) const;

	/** Clear all entries from the table (including their meta-data) */
	void ClearSourceStrings(const int32 InSlack = 0);

	/** Find the entry with the given key (if any) */
	FStringTableEntryConstPtr FindEntry(const FTextKey& InKey) const;

	/** Given an entry, check to see if it exists in this table, and if so, get its key */
	bool FindKey(const FStringTableEntryConstRef& InEntry, FString& OutKey) const;
	bool FindKey(const FStringTableEntryConstRef& InEntry, FTextKey& OutKey) const;

	/** Get the meta-data with the given ID associated with the given entry, or an empty string if not found */
	FString GetMetaData(const FTextKey& InKey, const FName InMetaDataId) const;

	/** Set the meta-data with the given ID associated with the given entry */
	void SetMetaData(const FTextKey& InKey, const FName InMetaDataId, const FString& InMetaDataValue);

	/** Remove the meta-data with the given ID associated with the given entry */
	void RemoveMetaData(const FTextKey& InKey, const FName InMetaDataId);

	/** Enumerate all meta-data associated with the given entry. Return true from the enumerator to continue, or false to stop */
	void EnumerateMetaData(const FTextKey& InKey, const TFunctionRef<bool(FName, const FString&)>& InEnumerator) const;

	/** Remove all meta-data associated with the given entry */
	void ClearMetaData(const FTextKey& InKey);

	/** Clear all meta-data from the table */
	void ClearMetaData(const int32 InSlack = 0);

	/** Serialize this string table to/from an archive */
	void Serialize(FArchive& Ar);

	/** Export the key, string, and meta-data information in this string table to a CSV file (does not export the namespace) */
	bool ExportStrings(const FString& InFilename) const;

	/** Import key, string, and meta-data information from a CSV file to this string table (does not import the namespace) */
	bool ImportStrings(const FString& InFilename);

private:
	/** Pointer back to the asset that owns this table */
	UStringTable* OwnerAsset;

	/** True if this table has been fully loaded (used for assets) */
	bool bIsLoaded;

	/** The namespace to use for all the strings in this table */
	FTextKey TableNamespace;

	/** Mapping between the text key and entry data for the strings within this table */
	TMap<FTextKey, FStringTableEntryPtr> KeysToEntries;

	/** Critical section preventing concurrent modification of KeysToEntries */
	mutable FCriticalSection KeyMappingCS;

	/** Mapping between the text key and its meta-data map */
	typedef TMap<FName, FString> FMetaDataMap;
	TMap<FTextKey, FMetaDataMap> KeysToMetaData;

	/** Critical section preventing concurrent modification of KeysToMetaData */
	mutable FCriticalSection KeysToMetaDataCS;
};

/** Interface to allow Core code to access String Table assets from the Engine */
class CORE_API IStringTableEngineBridge
{
public:
	/**
	 * Callback used when loading string table assets.
	 * @param The name of the table we were asked to load.
	 * @param The name of the table we actually loaded (may be different if redirected; will be empty if the load failed).
	 */
	typedef TFunction<void(FName, FName)> FLoadStringTableAssetCallback;

	/** 
	 * Check to see whether it is currently safe to attempt to find or load a string table asset.
	 * @return True if it is safe to attempt to find or load a string table asset, false otherwise.
	 */
	static bool CanFindOrLoadStringTableAsset()
	{
		return FInternationalization::IsAvailable()
			&& (!InstancePtr || InstancePtr->CanFindOrLoadStringTableAssetImpl());
	}

	/**
	 * Load a string table asset by its name, potentially doing so asynchronously. 
	 * @note If the string table is already loaded, or loading is perform synchronously, then the callback will be called before this function returns.
	 * @return The async loading ID of the asset, or INDEX_NONE if no async loading was performed.
	 */
	static int32 LoadStringTableAsset(const FName InTableId, FLoadStringTableAssetCallback InLoadedCallback = FLoadStringTableAssetCallback())
	{
		check(CanFindOrLoadStringTableAsset());

		if (InstancePtr)
		{
			return InstancePtr->LoadStringTableAssetImpl(InTableId, InLoadedCallback);
		}

		// No bridge instance - just say it's already loaded
		if (InLoadedCallback)
		{
			InLoadedCallback(InTableId, InTableId);
		}
		return INDEX_NONE;
	}

	/**
	 * Fully load a string table asset by its name, synchronously.
	 * @note This should be used sparingly in places where it is definitely safe to perform a blocking load.
	 */
	static void FullyLoadStringTableAsset(FName& InOutTableId)
	{
		check(CanFindOrLoadStringTableAsset());

		if (InstancePtr)
		{
			return InstancePtr->FullyLoadStringTableAssetImpl(InOutTableId);
		}
	}

	/** Redirect string table asset by its name */
	static void RedirectStringTableAsset(FName& InOutTableId)
	{
		check(CanFindOrLoadStringTableAsset());

		if (InstancePtr)
		{
			InstancePtr->RedirectStringTableAssetImpl(InOutTableId);
		}
	}

	/** Collect a string table asset reference */
	static void CollectStringTableAssetReferences(FName& InOutTableId, FStructuredArchive::FSlot Slot)
	{
		if (InstancePtr)
		{
			InstancePtr->CollectStringTableAssetReferencesImpl(InOutTableId, Slot);
		}
	}

	/** Is this string table from an asset? */
	static bool IsStringTableFromAsset(const FName InTableId)
	{
		return InstancePtr && InstancePtr->IsStringTableFromAssetImpl(InTableId);
	}

	/** Is this string table asset being replaced due to a hot-reload? */
	static bool IsStringTableAssetBeingReplaced(const UStringTable* InStringTableAsset)
	{
		return InstancePtr && InStringTableAsset && InstancePtr->IsStringTableAssetBeingReplacedImpl(InStringTableAsset);
	}

protected:
	virtual ~IStringTableEngineBridge() = default;

	virtual bool CanFindOrLoadStringTableAssetImpl() = 0;
	virtual int32 LoadStringTableAssetImpl(const FName InTableId, FLoadStringTableAssetCallback InLoadedCallback) = 0;
	virtual void FullyLoadStringTableAssetImpl(FName& InOutTableId) = 0;
	virtual void RedirectStringTableAssetImpl(FName& InOutTableId) = 0;
	virtual void CollectStringTableAssetReferencesImpl(FName& InOutTableId, FStructuredArchive::FSlot Slot) = 0;
	virtual bool IsStringTableFromAssetImpl(const FName InTableId) = 0;
	virtual bool IsStringTableAssetBeingReplacedImpl(const UStringTable* InStringTableAsset) = 0;

	/** Singleton instance, populated by the derived type */
	static IStringTableEngineBridge* InstancePtr;
};

/** String table redirect utils */
struct CORE_API FStringTableRedirects
{
	/** Initialize the string table redirects */
	static void InitStringTableRedirects();

	/** Redirect a table ID */
	static void RedirectTableId(FName& InOutTableId);

	/** Redirect a key */
	static void RedirectKey(const FName InTableId, FTextKey& InOutKey);

	/** Redirect a table ID and key */
	static void RedirectTableIdAndKey(FName& InOutTableId, FTextKey& InOutKey);
};
