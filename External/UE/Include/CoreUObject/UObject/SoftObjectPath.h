// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/StringView.h"
#include "HAL/ThreadSafeCounter.h"
#include "HAL/ThreadSingleton.h"
#include "UObject/Class.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UObjectHash.h"

/**
 * A struct that contains a string reference to an object, either a top level asset or a subobject.
 * This can be used to make soft references to assets that are loaded on demand.
 * This is stored internally as an FName pointing to the top level asset (/package/path.assetname) and an option a string subobject path.
 * If the MetaClass metadata is applied to a FProperty with this the UI will restrict to that type of asset.
 */
struct COREUOBJECT_API FSoftObjectPath
{
	FSoftObjectPath() {}

	/** Construct from another soft object path */
	FSoftObjectPath(const FSoftObjectPath& Other)
		: AssetPathName(Other.AssetPathName)
		, SubPathString(Other.SubPathString)
	{
	}

	/** Construct from a moveable soft object path */
	FSoftObjectPath(FSoftObjectPath&& Other)
		: AssetPathName(Other.AssetPathName)
		, SubPathString(MoveTemp(Other.SubPathString))
	{
	}

	/** Construct from a path string. Non-explicit for backwards compatibility. */
	FSoftObjectPath(const FString& Path)						{ SetPath(FStringView(Path)); }
	explicit FSoftObjectPath(FWideStringView Path)				{ SetPath(Path); }
	explicit FSoftObjectPath(FAnsiStringView Path)				{ SetPath(Path); }
	explicit FSoftObjectPath(FName Path)						{ SetPath(Path); }
	explicit FSoftObjectPath(const WIDECHAR* Path)				{ SetPath(FWideStringView(Path)); }
	explicit FSoftObjectPath(const ANSICHAR* Path)				{ SetPath(FAnsiStringView(Path)); }
	explicit FSoftObjectPath(TYPE_OF_NULLPTR)					{}
	
	/** Construct from an asset FName and subobject pair */
	FSoftObjectPath(FName InAssetPathName, FString InSubPathString)
		: AssetPathName(InAssetPathName)
		, SubPathString(MoveTemp(InSubPathString))
	{}
	
	/** Construct from an existing object in memory */
	FSoftObjectPath(const UObject* InObject);

	FSoftObjectPath(FObjectPtr InObject)
	{
		SetPath(InObject.GetPath());
	}

	template <typename T>
	FSoftObjectPath(const TObjectPtr<T>& InObject)
	{
		SetPath(InObject.GetPath());
	}

	~FSoftObjectPath() {}
	
	FSoftObjectPath& operator=(const FSoftObjectPath& Path)	= default;
	FSoftObjectPath& operator=(FSoftObjectPath&& Path) = default;
	FSoftObjectPath& operator=(const FString& Path)						{ SetPath(FStringView(Path)); return *this; }
	FSoftObjectPath& operator=(FWideStringView Path)					{ SetPath(Path); return *this; }
	FSoftObjectPath& operator=(FAnsiStringView Path)					{ SetPath(Path); return *this; }
	FSoftObjectPath& operator=(FName Path)								{ SetPath(Path); return *this; }
	FSoftObjectPath& operator=(const WIDECHAR* Path)					{ SetPath(FWideStringView(Path)); return *this; }
	FSoftObjectPath& operator=(const ANSICHAR* Path)					{ SetPath(FAnsiStringView(Path)); return *this; }
	FSoftObjectPath& operator=(TYPE_OF_NULLPTR)							{ Reset(); return *this; }

	/** Returns string representation of reference, in form /package/path.assetname[:subpath] */
	FString ToString() const;

	/** Append string representation of reference, in form /package/path.assetname[:subpath] */
	void ToString(FStringBuilderBase& Builder) const;

	/** Returns the entire asset path as an FName, including both package and asset but not sub object */
	FORCEINLINE FName GetAssetPathName() const
	{
		return AssetPathName;
	}

	FORCEINLINE void SetAssetPathName(FName InAssetPathName)
	{
		AssetPathName = InAssetPathName;
	}

	/** Returns string version of asset path, including both package and asset but not sub object */
	FORCEINLINE FString GetAssetPathString() const
	{
		if (AssetPathName.IsNone())
		{
			return FString();
		}

		return AssetPathName.ToString();
	}

	/** Returns the sub path, which is often empty */
	FORCEINLINE const FString& GetSubPathString() const
	{
		return SubPathString;
	}

	FORCEINLINE void SetSubPathString(const FString& InSubPathString)
	{
		SubPathString = InSubPathString;
	}

	/** Returns /package/path, leaving off the asset name and sub object */
	FString GetLongPackageName() const
	{
		FString PackageName;
		GetAssetPathString().Split(TEXT("."), &PackageName, nullptr, ESearchCase::CaseSensitive, ESearchDir::FromStart);
		return PackageName;
	}

	/** Returns /package/path, leaving off the asset name and sub object */
	FName GetLongPackageFName() const;

	/** Returns assetname string, leaving off the /package/path part and sub object */
	FString GetAssetName() const
	{
		FString AssetName;
		GetAssetPathString().Split(TEXT("."), nullptr, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromStart);
		return AssetName;
	}

	/** Sets asset path of this reference based on a string path */
	void SetPath(FWideStringView Path);
	void SetPath(FAnsiStringView Path);
	void SetPath(FUtf8StringView Path);
	void SetPath(FName Path);
	void SetPath(const WIDECHAR* Path)			{ SetPath(FWideStringView(Path)); }
	void SetPath(const ANSICHAR* Path)			{ SetPath(FAnsiStringView(Path)); }
	void SetPath(const FString& Path)			{ SetPath(FStringView(Path)); }

	/**
	 * Attempts to load the asset, this will call LoadObject which can be very slow
	 * @param InLoadContext Optional load context when called from nested load callstack
	 * @return Loaded UObject, or nullptr if the reference is null or the asset fails to load
	 */
	UObject* TryLoad(FUObjectSerializeContext* InLoadContext = nullptr) const;

	/**
	 * Attempts to find a currently loaded object that matches this path
	 *
	 * @return Found UObject, or nullptr if not currently in memory
	 */
	UObject* ResolveObject() const;

	/** Resets reference to point to null */
	void Reset()
	{		
		AssetPathName = FName();
		SubPathString.Reset();
	}
	
	/** Check if this could possibly refer to a real object, or was initialized to null */
	FORCEINLINE bool IsValid() const
	{
		return !AssetPathName.IsNone();
	}

	/** Checks to see if this is initialized to null */
	FORCEINLINE bool IsNull() const
	{
		return AssetPathName.IsNone();
	}

	/** Check if this represents an asset, meaning it is not null but does not have a sub path */
	FORCEINLINE bool IsAsset() const
	{
		return !AssetPathName.IsNone() && SubPathString.IsEmpty();
	}

	/** Check if this represents a sub object, meaning it has a sub path */
	FORCEINLINE bool IsSubobject() const
	{
		return !AssetPathName.IsNone() && !SubPathString.IsEmpty();
	}

	/** Struct overrides */
	bool Serialize(FArchive& Ar);
	bool Serialize(FStructuredArchive::FSlot Slot);
	bool operator==(FSoftObjectPath const& Other) const;
	bool operator!=(FSoftObjectPath const& Other) const
	{
		return !(*this == Other);
	}

	bool ExportTextItem(FString& ValueStr, FSoftObjectPath const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem( const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText, FArchive* InSerializingArchive = nullptr );
	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FStructuredArchive::FSlot Slot);

	/** Serializes the internal path and also handles save/PIE fixups. Call this from the archiver overrides */
	void SerializePath(FArchive& Ar);

	/** Fixes up path for saving, call if saving with a method that skips SerializePath. This can modify the path, it will return true if it was modified */
	bool PreSavePath(bool* bReportSoftObjectPathRedirects = nullptr);

	/** 
	 * Handles when a path has been loaded, call if loading with a method that skips SerializePath. This does not modify path but might call callbacks
	 * @param InArchive The archive that loaded this path
	 */
	void PostLoadPath(FArchive* InArchive) const;

	/** Fixes up this SoftObjectPath to add the PIE prefix depending on what is currently active, returns true if it was modified. The overload that takes an explicit PIE instance is preferred, if it's available. */
	bool FixupForPIE(TFunctionRef<void(int32, FSoftObjectPath&)> InPreFixupForPIECustomFunction = [](int32, FSoftObjectPath&) {});

	/** Fixes up this SoftObjectPath to add the PIE prefix for the given PIEInstance index, returns true if it was modified */
	bool FixupForPIE(int32 PIEInstance, TFunctionRef<void(int32, FSoftObjectPath&)> InPreFixupForPIECustomFunction = [](int32, FSoftObjectPath&) {});

	/** Fixes soft object path for CoreRedirects to handle renamed native objects, returns true if it was modified */
	bool FixupCoreRedirects();

	FORCEINLINE friend uint32 GetTypeHash(FSoftObjectPath const& This)
	{
		uint32 Hash = 0;

		Hash = HashCombine(Hash, GetTypeHash(This.AssetPathName));
		Hash = HashCombine(Hash, GetTypeHash(This.SubPathString));
		return Hash;
	}

	/** Code needed by FSoftObjectPtr internals */
	static int32 GetCurrentTag()
	{
		return CurrentTag.GetValue();
	}
	static int32 InvalidateTag()
	{
		return CurrentTag.Increment();
	}
	static FSoftObjectPath GetOrCreateIDForObject(const UObject* Object);
	
	/** Adds list of packages names that have been created specifically for PIE, this is used for editor fixup */
	static void AddPIEPackageName(FName NewPIEPackageName);
	
	/** Disables special PIE path handling, call when PIE finishes to clear list */
	static void ClearPIEPackageNames();

private:
	/** Asset path, patch to a top level object in a package. This is /package/path.assetname */
	FName AssetPathName;

	/** Optional FString for subobject within an asset. This is the sub path after the : */
	FString SubPathString;

	/** Global counter that determines when we need to re-search based on path because more objects have been loaded **/
	static FThreadSafeCounter CurrentTag;

	/** Package names currently being duplicated, needed by FixupForPIE */
	static TSet<FName> PIEPackageNames;

	UObject* ResolveObjectInternal() const;
	UObject* ResolveObjectInternal(const TCHAR* PathString) const;

	friend struct Z_Construct_UScriptStruct_FSoftObjectPath_Statics;
};

/** Fast non-alphabetical order that is only stable during this process' lifetime. */
struct FSoftObjectPathFastLess
{
	bool operator()(const FSoftObjectPath& Lhs, const FSoftObjectPath& Rhs) const
	{
		int32 Comp = Lhs.GetAssetPathName().CompareIndexes(Rhs.GetAssetPathName());
		if (Comp < 0)
		{
			return true;
		}
		return Comp == 0 && Lhs.GetSubPathString() < Rhs.GetSubPathString();
	}
};

/** Slow alphabetical order that is stable / deterministic over process runs. */
struct FSoftObjectPathLexicalLess
{
	bool operator()(const FSoftObjectPath& Lhs, const FSoftObjectPath& Rhs) const
	{
		int32 Comp = Lhs.GetAssetPathName().Compare(Rhs.GetAssetPathName());
		if (Comp < 0)
		{
			return true;
		}
		return Comp == 0 && Lhs.GetSubPathString() < Rhs.GetSubPathString();
	}
};

inline FStringBuilderBase& operator<<(FStringBuilderBase& Builder, const FSoftObjectPath& Path)
{
	Path.ToString(Builder);
	return Builder;
}

/**
 * A struct that contains a string reference to a class, can be used to make soft references to classes
 */
struct COREUOBJECT_API FSoftClassPath : public FSoftObjectPath
{
	FSoftClassPath()
	{ }

	FSoftClassPath(FSoftClassPath const& Other)
		: FSoftObjectPath(Other)
	{ }

	/**
	 * Construct from a path string
	 */
	FSoftClassPath(const FString& PathString)
		: FSoftObjectPath(PathString)
	{ }

	/**
	 * Construct from an existing class, will do some string processing
	 */
	FSoftClassPath(const UClass* InClass)
		: FSoftObjectPath(InClass)
	{ }

	/**
	* Attempts to load the class.
	* @return Loaded UObject, or null if the class fails to load, or if the reference is not valid.
	*/
	template<typename T>
	UClass* TryLoadClass() const
	{
		if ( IsValid() )
		{
			return LoadClass<T>(nullptr, *ToString(), nullptr, LOAD_None, nullptr);
		}

		return nullptr;
	}

	/**
	 * Attempts to find a currently loaded object that matches this object ID
	 * @return Found UClass, or NULL if not currently loaded
	 */
	UClass* ResolveClass() const;

	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);

	static FSoftClassPath GetOrCreateIDForClass(const UClass *InClass);

private:
	/** Forbid creation for UObject. This class is for UClass only. Use FSoftObjectPath instead. */
	FSoftClassPath(const UObject* InObject) { }

	/** Forbidden. This class is for UClass only. Use FSoftObjectPath instead. */
	static FSoftObjectPath GetOrCreateIDForObject(const UObject *Object);
};

UE_DEPRECATED(5.0, "FStringAssetReference was renamed to FSoftObjectPath as it is now not always a string and can also refer to a subobject")
typedef FSoftObjectPath FStringAssetReference;

UE_DEPRECATED(5.0, "FStringClassReference was renamed to FSoftClassPath")
typedef FSoftClassPath FStringClassReference;

/** Options for how to set soft object path collection */
enum class ESoftObjectPathCollectType : uint8
{
	/** The SoftObjectPath being loaded is not in a package, so we do not need to record it in inclusion or exclusion lists*/
	NonPackage,
	/** References is not tracked in any situation, transient reference */
	NeverCollect,
	/** Editor only reference, this is tracked for redirector fixup but not for cooking */
	EditorOnlyCollect,
	/** Game reference, this is gathered for both redirector fixup and cooking */
	AlwaysCollect,
};

/** Rules for actually serializing the internals of soft object paths */
enum class ESoftObjectPathSerializeType : uint8
{
	/** Never serialize the raw names */
	NeverSerialize,
	/** Only serialize if the archive has no size */
	SkipSerializeIfArchiveHasSize,
	/** Always serialize the soft object path internals */
	AlwaysSerialize,
};

class COREUOBJECT_API FSoftObjectPathThreadContext : public TThreadSingleton<FSoftObjectPathThreadContext>
{
	friend TThreadSingleton<FSoftObjectPathThreadContext>;
	friend struct FSoftObjectPathSerializationScope;

	FSoftObjectPathThreadContext() {}

	struct FSerializationOptions
	{
		FName PackageName;
		FName PropertyName;
		ESoftObjectPathCollectType CollectType;
		ESoftObjectPathSerializeType SerializeType;

		FSerializationOptions() : CollectType(ESoftObjectPathCollectType::AlwaysCollect) {}
		FSerializationOptions(FName InPackageName, FName InPropertyName, ESoftObjectPathCollectType InCollectType, ESoftObjectPathSerializeType InSerializeType) : PackageName(InPackageName), PropertyName(InPropertyName), CollectType(InCollectType), SerializeType(InSerializeType) {}
	};

	TArray<FSerializationOptions> OptionStack;
public:
	/** 
	 * Returns the current serialization options that were added using SerializationScope or LinkerLoad
	 *
	 * @param OutPackageName Package that this string asset belongs to
	 * @param OutPropertyName Property that this path belongs to
	 * @param OutCollectType Type of collecting that should be done
	 * @param Archive The FArchive that is serializing this path if known. If null it will check FUObjectThreadContext
	 */
	bool GetSerializationOptions(FName& OutPackageName, FName& OutPropertyName, ESoftObjectPathCollectType& OutCollectType, ESoftObjectPathSerializeType& OutSerializeType, FArchive* Archive = nullptr) const;
};

/** Helper class to set and restore serialization options for soft object paths */
struct FSoftObjectPathSerializationScope
{
	/** 
	 * Create a new serialization scope, which affects the way that soft object paths are saved
	 *
	 * @param SerializingPackageName Package that this string asset belongs to
	 * @param SerializingPropertyName Property that this path belongs to
	 * @param CollectType Set type of collecting that should be done, can be used to disable tracking entirely
	 */
	FSoftObjectPathSerializationScope(FName SerializingPackageName, FName SerializingPropertyName, ESoftObjectPathCollectType CollectType, ESoftObjectPathSerializeType SerializeType)
	{
		FSoftObjectPathThreadContext::Get().OptionStack.Emplace(SerializingPackageName, SerializingPropertyName, CollectType, SerializeType);
	}

	~FSoftObjectPathSerializationScope()
	{
		FSoftObjectPathThreadContext::Get().OptionStack.Pop();
	}
};

/** Structure for file paths that are displayed in the editor with a picker UI. */
struct FFilePath
{
	/**
	 * The path to the file.
	 */
	FString FilePath;
};

/** Structure for directory paths that are displayed in the editor with a picker UI. */
struct FDirectoryPath
{
	/**
	 * The path to the directory.
	 */
	FString Path;
};

// Fixup archive
struct FSoftObjectPathFixupArchive : public FArchiveUObject
{
	FSoftObjectPathFixupArchive(TFunction<void(FSoftObjectPath&)> InFixupFunction)
		: FixupFunction(InFixupFunction)
	{
		this->SetIsSaving(true);
		this->ArShouldSkipBulkData = true;
	}

	FSoftObjectPathFixupArchive(const FString& InOldAssetPathString, const FString& InNewAssetPathString)
		: FSoftObjectPathFixupArchive([OldAssetPathString = InOldAssetPathString, NewAssetPathString = InNewAssetPathString](FSoftObjectPath& Value)
		{
			if (!Value.IsNull() && Value.GetAssetPathString().Equals(OldAssetPathString, ESearchCase::IgnoreCase))
			{
				Value.SetAssetPathName(FName(*NewAssetPathString));
			}
		})
	{
	}

	FArchive& operator<<(FSoftObjectPath& Value) override
	{
		FixupFunction(Value);
		return *this;
	}

	void Fixup(UObject* Root)
	{
		Root->Serialize(*this);
		ForEachObjectWithOuter(Root, [this](UObject* InObject) { InObject->Serialize(*this);  });
	}

	TFunction<void(FSoftObjectPath&)> FixupFunction;
};
