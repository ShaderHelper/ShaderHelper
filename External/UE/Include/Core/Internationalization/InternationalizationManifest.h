// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Containers/Array.h"
#include "Misc/Crc.h"
#include "Containers/UnrealString.h"
#include "Containers/Set.h"
#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "Internationalization/LocKeyFuncs.h"

class FLocMetadataObject;

struct CORE_API FManifestContext
{
public:
	FManifestContext()
		: Key()
		, SourceLocation()
		, PlatformName()
		, bIsOptional(false)
	{
	}

	explicit FManifestContext(const FLocKey& InKey)
		: Key(InKey)
		, SourceLocation()
		, PlatformName()
		, bIsOptional(false)
	{
	}

	/** Copy ctor */
	FManifestContext(const FManifestContext& Other);

	FManifestContext& operator=(const FManifestContext& Other);
	bool operator==(const FManifestContext& Other) const;
	inline bool operator!=(const FManifestContext& Other) const { return !(*this == Other); }
	bool operator<(const FManifestContext& Other) const;

public:
	FLocKey Key;
	FString SourceLocation;
	FName PlatformName;
	bool bIsOptional;

	TSharedPtr<FLocMetadataObject> InfoMetadataObj;
	TSharedPtr<FLocMetadataObject> KeyMetadataObj;
};


struct CORE_API FLocItem
{
public:
	FLocItem()
		: Text()
		, MetadataObj(nullptr)
	{
	}

	explicit FLocItem(FString InSourceText)
		: Text(MoveTemp(InSourceText))
		, MetadataObj(nullptr)
	{
	}

	FLocItem(FString InSourceText, TSharedPtr<FLocMetadataObject> InMetadataObj)
		: Text(MoveTemp(InSourceText))
		, MetadataObj(MoveTemp(InMetadataObj))
	{
	}

	/** Copy ctor */
	FLocItem(const FLocItem& Other);

	FLocItem& operator=(const FLocItem& Other);
	bool operator==(const FLocItem& Other) const;
	inline bool operator!=(const FLocItem& Other) const { return !(*this == Other); }
	bool operator<(const FLocItem& Other) const;

	/** Similar functionality to == operator but ensures everything matches(ex. ignores COMPARISON_MODIFIER_PREFIX on metadata). */
	bool IsExactMatch(const FLocItem& Other) const;

public:
	FString Text;
	TSharedPtr<FLocMetadataObject> MetadataObj;
};


class CORE_API FManifestEntry
{
public:
	FManifestEntry(const FLocKey& InNamespace, const FLocItem& InSource)
		: Namespace(InNamespace)
		, Source(InSource)
		, Contexts()
	{
	}

	const FManifestContext* FindContext(const FLocKey& ContextKey, const TSharedPtr<FLocMetadataObject>& KeyMetadata = nullptr) const;
	const FManifestContext* FindContextByKey(const FLocKey& ContextKey) const;
	void MergeContextPlatformInfo(const FManifestContext& InContext);

	const FLocKey Namespace;
	const FLocItem Source;
	TArray<FManifestContext> Contexts;
};


typedef TMultiMap< FLocKey, TSharedRef< FManifestEntry > > FManifestEntryByLocKeyContainer;
typedef TMultiMap< FString, TSharedRef< FManifestEntry >, FDefaultSetAllocator, FLocKeyMultiMapFuncs< TSharedRef< FManifestEntry > > > FManifestEntryByStringContainer;


class CORE_API FInternationalizationManifest 
{
public:
	enum class EFormatVersion : uint8
	{
		Initial = 0,
		EscapeFixes,

		LatestPlusOne,
		Latest = LatestPlusOne - 1
	};

	//Default constructor
	FInternationalizationManifest()
		: FormatVersion(EFormatVersion::Latest)
	{
	}

	/**
	* Adds a manifest entry.
	*
	* @return Returns true if add was successful or a matching entry already exists, false is only returned in the case where a duplicate context was found with different text.
	*/
	bool AddSource(const FLocKey& Namespace, const FLocItem& Source, const FManifestContext& Context);

	void UpdateEntry(const TSharedRef<FManifestEntry>& OldEntry, TSharedRef<FManifestEntry>& NewEntry);

	TSharedPtr<FManifestEntry> FindEntryBySource(const FLocKey& Namespace, const FLocItem& Source) const;

	TSharedPtr<FManifestEntry> FindEntryByContext(const FLocKey& Namespace, const FManifestContext& Context) const;

	TSharedPtr<FManifestEntry> FindEntryByKey(const FLocKey& Namespace, const FLocKey& Key, const FString* SourceText = nullptr) const;

	FManifestEntryByLocKeyContainer::TConstIterator GetEntriesByKeyIterator() const
	{
		return EntriesByKey.CreateConstIterator();
	}

	int32 GetNumEntriesByKey() const
	{
		return EntriesByKey.Num();
	}

	FManifestEntryByStringContainer::TConstIterator GetEntriesBySourceTextIterator() const
	{
		return EntriesBySourceText.CreateConstIterator();
	}

	int32 GetNumEntriesBySourceText() const
	{
		return EntriesBySourceText.Num();
	}

	void SetFormatVersion(const EFormatVersion Version)
	{
		FormatVersion = Version;
	}

	EFormatVersion GetFormatVersion() const
	{
		return FormatVersion;
	}

private:
	EFormatVersion FormatVersion;
	FManifestEntryByStringContainer EntriesBySourceText;
	FManifestEntryByLocKeyContainer EntriesByKey;
};
