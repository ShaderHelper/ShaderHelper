// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Misc/StringBuilder.h"

/**
 * A primary asset type, represented as an FName internally and implicitly convertible back and forth
 * This exists so the blueprint API can understand it's not a normal FName
 */
struct FPrimaryAssetType
{
	/** Convert from FName */
	FPrimaryAssetType() {}
	FPrimaryAssetType(FName InName) : Name(InName) {}
	FPrimaryAssetType(EName InName) : Name(FName(InName)) {}
	FPrimaryAssetType(const WIDECHAR* InName) : Name(FName(InName)) {}
	FPrimaryAssetType(const ANSICHAR* InName) : Name(FName(InName)) {}

	/** Convert to FName */
	operator FName&() { return Name; }
	operator const FName&() const { return Name; }

	/** Returns internal Name explicitly, not normally needed */
	FName GetName() const
	{
		return Name;
	}

	bool operator==(const FPrimaryAssetType& Other) const
	{
		return Name == Other.Name;
	}

	bool operator!=(const FPrimaryAssetType& Other) const
	{
		return Name != Other.Name;
	}

	FPrimaryAssetType& operator=(const FPrimaryAssetType& Other)
	{
		Name = Other.Name;
		return *this;
	}

	/** Returns true if this is a valid Type */
	bool IsValid() const
	{
		return Name != NAME_None;
	}

	/** Returns string version of this Type */
	FString ToString() const
	{
		return Name.ToString();
	}

	/** Appends to the given builder the string version of this Type */
	void AppendString(FStringBuilderBase& Builder) const
	{
		Name.AppendString(Builder);
	}

	/** UStruct Overrides */
	bool ExportTextItem(FString& ValueStr, FPrimaryAssetType const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FStructuredArchive::FSlot Slot);

	friend inline uint32 GetTypeHash(const FPrimaryAssetType& Key)
	{
		return GetTypeHash(Key.Name);
	}

private:
	friend struct Z_Construct_UScriptStruct_FPrimaryAssetType_Statics;

	/** The FName representing this type */
	FName Name;
};

/**
 * This identifies an object as a "primary" asset that can be searched for by the AssetManager and used in various tools
 */
struct FPrimaryAssetId
{
	/** An FName describing the logical type of this object, usually the name of a base UClass. For example, any Blueprint derived from APawn will have a Primary Asset Type of "Pawn".
	"PrimaryAssetType:PrimaryAssetName" should form a unique name across your project. */
	FPrimaryAssetType PrimaryAssetType;
	/** An FName describing this asset. This is usually the short name of the object, but could be a full asset path for things like maps, or objects with GetPrimaryId() overridden.
	"PrimaryAssetType:PrimaryAssetName" should form a unique name across your project. */
	FName PrimaryAssetName;

	/** Static names to represent the AssetRegistry tags for the above data */
	static COREUOBJECT_API const FName PrimaryAssetTypeTag;
	static COREUOBJECT_API const FName PrimaryAssetNameTag;

	FPrimaryAssetId() {}

	FPrimaryAssetId(FPrimaryAssetType InAssetType, FName InAssetName)
		: PrimaryAssetType(InAssetType), PrimaryAssetName(InAssetName)
	{}

	static COREUOBJECT_API FPrimaryAssetId ParseTypeAndName(const TCHAR* TypeAndName, uint32 Len);
	static COREUOBJECT_API FPrimaryAssetId ParseTypeAndName(FName TypeAndName);
	static FPrimaryAssetId ParseTypeAndName(const FString& TypeAndName)
	{
		return ParseTypeAndName(*TypeAndName, static_cast<uint32>(TypeAndName.Len()));
	}

	explicit FPrimaryAssetId(const FString& TypeAndName)
		: FPrimaryAssetId(ParseTypeAndName(TypeAndName))
	{}

	/** Returns true if this is a valid identifier */
	bool IsValid() const
	{
		return PrimaryAssetType != NAME_None && PrimaryAssetName != NAME_None;
	}

	/** Returns string version of this identifier in Type:Name format */
	FString ToString() const
	{
		TStringBuilder<256> Builder;
		AppendString(Builder);
		return FString(Builder.Len(), Builder.GetData());
	}

	/** Appends to the given builder the string version of this identifier in Type:Name format */
	void AppendString(FStringBuilderBase& Builder) const
	{
		if (IsValid())
		{
			PrimaryAssetType.AppendString(Builder);
			Builder << TEXT(":");
			PrimaryAssetName.AppendString(Builder);
		}
	}

	/** Converts from Type:Name format */
	static FPrimaryAssetId FromString(const FString& String)
	{
		return FPrimaryAssetId(String);
	}

	bool operator==(const FPrimaryAssetId& Other) const
	{
		return PrimaryAssetType == Other.PrimaryAssetType && PrimaryAssetName == Other.PrimaryAssetName;
	}

	bool operator!=(const FPrimaryAssetId& Other) const
	{
		return PrimaryAssetType != Other.PrimaryAssetType || PrimaryAssetName != Other.PrimaryAssetName;
	}

	FPrimaryAssetId& operator=(const FPrimaryAssetId& Other)
	{
		PrimaryAssetType = Other.PrimaryAssetType;
		PrimaryAssetName = Other.PrimaryAssetName;
		return *this;
	}

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FPrimaryAssetId& Other)
	{
        Ar << Other.PrimaryAssetType;
		Ar << Other.PrimaryAssetName;
		return Ar;
	}

	/** UStruct Overrides */
	bool ExportTextItem(FString& ValueStr, FPrimaryAssetId const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FStructuredArchive::FSlot Slot);

	friend inline uint32 GetTypeHash(const FPrimaryAssetId& Key)
	{
		uint32 Hash = 0;

		Hash = HashCombine(Hash, GetTypeHash(Key.PrimaryAssetType));
		Hash = HashCombine(Hash, GetTypeHash(Key.PrimaryAssetName));
		return Hash;
	}

	friend struct Z_Construct_UScriptStruct_FPrimaryAssetId_Statics;
};

COREUOBJECT_API FStringBuilderBase& operator<<(FStringBuilderBase& Builder, const FPrimaryAssetId& Id);