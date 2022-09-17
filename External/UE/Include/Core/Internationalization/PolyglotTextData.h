// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Internationalization/LocalizedTextSourceTypes.h"
#include "Containers/UnrealString.h"
#include "Containers/Map.h"
#include "Internationalization/Text.h"

/**
 * Polyglot data that may be registered to the text localization manager at runtime.
 * @note This struct is mirrored in NoExportTypes.h for UHT.
 */
struct CORE_API FPolyglotTextData
{
	/** Needed to avoid member access issues in UHT generated code */
	friend struct Z_Construct_UScriptStruct_FPolyglotTextData_Statics;

public:
	/**
	 * Default constructor
	 */
	FPolyglotTextData() = default;

	/**
	 * Construct this polyglot data with an identity, optionally immediately providing the source text and native culture.
	 */
	FPolyglotTextData(const ELocalizedTextSourceCategory& InCategory, const FString& InNamespace, const FString& InKey, const FString& InNativeString, const FString& InNativeCulture = FString());

	/**
	 * Is this polyglot data valid and can be registered with the text localization manager?
	 */
	bool IsValid(FText* OutFailureReason = nullptr) const;

	/**
	 * Set the category of this polyglot data.
	 * @note This affects when and how the data is loaded into the text localization manager.
	 */
	void SetCategory(const ELocalizedTextSourceCategory InCategory);

	/**
	 * Get the category of this polyglot data.
	 */
	ELocalizedTextSourceCategory GetCategory() const;

	/**
	 * Set the native culture of this polyglot data.
	 * @note This may be empty, and if empty, will be inferred from the native culture of the text category.
	 */
	void SetNativeCulture(const FString& InNativeCulture);

	/**
	 * Get the native culture of this polyglot data.
	 */
	const FString& GetNativeCulture() const;

	/**
	 * Resolve the native culture of this polyglot data, either using the native culture if specified, or inferring it from the category.
	 */
	FString ResolveNativeCulture() const;

	/**
	 * Get the localized cultures of this polyglot data.
	 */
	TArray<FString> GetLocalizedCultures() const;

	/**
	 * Set the identity of the text created from this polyglot data.
	 * @note The key must not be empty.
	 */
	void SetIdentity(const FString& InNamespace, const FString& InKey);

	/**
	 * Get the identity of the text created from this polyglot data.
	 */
	void GetIdentity(FString& OutNamespace, FString& OutKey) const;

	/**
	 * Get the namespace of the text created from this polyglot data.
	 */
	const FString& GetNamespace() const;

	/**
	 * Get the key of the text created from this polyglot data.
	 */
	const FString& GetKey() const;

	/**
	 * Set the native string of this polyglot data.
	 * @note The native string must not be empty.
	 */
	void SetNativeString(const FString& InNativeString);

	/**
	 * Get the native string of this polyglot data.
	 */
	const FString& GetNativeString() const;
	
	/**
	 * Add a localized string to this polyglot data.
	 * @note The native culture may also have a translation added via this function.
	 */
	void AddLocalizedString(const FString& InCulture, const FString& InLocalizedString);

	/**
	 * Remove a localized string from this polyglot data.
	 */
	void RemoveLocalizedString(const FString& InCulture);

	/**
	 * Get a localized string from this polyglot data.
	 */
	bool GetLocalizedString(const FString& InCulture, FString& OutLocalizedString) const;

	/**
	 * Clear the localized strings from this polyglot data.
	 */
	void ClearLocalizedStrings();

	/**
	 * Set whether this polyglot data is a minimal patch.
	 * @see bIsMinimalPatch.
	 */
	void IsMinimalPatch(const bool InIsMinimalPatch);

	/**
	 * Get whether this polyglot data is a minimal patch.
	 * @see bIsMinimalPatch.
	 */
	bool IsMinimalPatch() const;

	/**
	 * Get the text instance created from this polyglot data.
	 */
	FText GetText() const;

private:
	/**
	 * Cache the text instance created from this polyglot data.
	 */
	void CacheText(FText* OutFailureReason = nullptr);

	/**
	 * Clear the cache of the text instance created from this polyglot data.
	 */
	void ClearCache();

	/**
	 * The category of this polyglot data.
	 * @note This affects when and how the data is loaded into the text localization manager.
	 */
	ELocalizedTextSourceCategory Category;

	/**
	 * The native culture of this polyglot data.
	 * @note This may be empty, and if empty, will be inferred from the native culture of the text category.
	 */
	FString NativeCulture;

	/**
	 * The namespace of the text created from this polyglot data.
	 * @note This may be empty.
	 */
	FString Namespace;

	/**
	 * The key of the text created from this polyglot data.
	 * @note This must not be empty.
	 */
	FString Key;

	/**
	 * The native string for this polyglot data.
	 * @note This must not be empty (it should be the same as the originally authored text you are trying to replace).
	 */
	FString NativeString;

	/**
	 * Mapping between a culture code and its localized string.
	 * @note The native culture may also have a translation in this map.
	 */
	TMap<FString, FString> LocalizedStrings;

	/**
	 * True if this polyglot data is a minimal patch, and that missing translations should be 
	 * ignored (falling back to any LocRes data) rather than falling back to the native string.
	 */
	bool bIsMinimalPatch = false;

	/**
	 * Transient cached text instance from registering this polyglot data with the text localization manager.
	 */
	FText CachedText;
};
