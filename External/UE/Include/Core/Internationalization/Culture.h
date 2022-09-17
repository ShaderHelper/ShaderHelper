// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Templates/UniquePtr.h"
#include "Internationalization/CulturePointer.h"

struct FDecimalNumberFormattingRules;
enum class ETextPluralForm : uint8;
enum class ETextPluralType : uint8;

#if UE_ENABLE_ICU
class FICUCultureImplementation;
typedef FICUCultureImplementation FCultureImplementation;
#else
class FLegacyCultureImplementation;
typedef FLegacyCultureImplementation FCultureImplementation;
#endif

class CORE_API FCulture
{
#if UE_ENABLE_ICU
	friend class FText;
	friend class FTextChronoFormatter;
	friend class FICUBreakIteratorManager;
#endif

public:
	~FCulture();
	
	static FCultureRef Create(TUniquePtr<FCultureImplementation>&& InImplementation);

	const FString& GetDisplayName() const;

	const FString& GetEnglishName() const;

	int GetKeyboardLayoutId() const;

	int GetLCID() const;

	TArray<FString> GetPrioritizedParentCultureNames() const;

	static TArray<FString> GetPrioritizedParentCultureNames(const FString& LanguageCode, const FString& ScriptCode, const FString& RegionCode);

	static FString CreateCultureName(const FString& LanguageCode, const FString& ScriptCode, const FString& RegionCode);

	static FString GetCanonicalName(const FString& Name);

	const FString& GetName() const;
	
	const FString& GetNativeName() const;

	const FString& GetUnrealLegacyThreeLetterISOLanguageName() const;

	const FString& GetThreeLetterISOLanguageName() const;

	const FString& GetTwoLetterISOLanguageName() const;

	const FString& GetNativeLanguage() const;

	const FString& GetRegion() const;

	const FString& GetNativeRegion() const;

	const FString& GetScript() const;

	const FString& GetVariant() const;

	bool IsRightToLeft() const;

	const FDecimalNumberFormattingRules& GetDecimalNumberFormattingRules() const;

	const FDecimalNumberFormattingRules& GetPercentFormattingRules() const;

	const FDecimalNumberFormattingRules& GetCurrencyFormattingRules(const FString& InCurrencyCode) const;

	/**
	 * Get the correct plural form to use for the given number
	 * @param PluralType The type of plural form to get (cardinal or ordinal)
	 */
	ETextPluralForm GetPluralForm(float Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(double Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(int8 Val,		const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(int16 Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(int32 Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(int64 Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(uint8 Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(uint16 Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(uint32 Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(uint64 Val,	const ETextPluralType PluralType) const;
	ETextPluralForm GetPluralForm(long Val,		const ETextPluralType PluralType) const;

	/**
	 * Get the plural forms supported by this culture
	 * @param PluralType The type of plural form to get (cardinal or ordinal)
	 */
	const TArray<ETextPluralForm>& GetValidPluralForms(const ETextPluralType PluralType) const;
	
	void RefreshCultureDisplayNames(const TArray<FString>& InPrioritizedDisplayCultureNames, const bool bFullRefresh = true);

private:
	explicit FCulture(TUniquePtr<FCultureImplementation>&& InImplementation);

	TUniquePtr<FCultureImplementation> Implementation;

	FString CachedDisplayName;
	FString CachedEnglishName;
	FString CachedName;
	FString CachedNativeName;
	FString CachedUnrealLegacyThreeLetterISOLanguageName;
	FString CachedThreeLetterISOLanguageName;
	FString CachedTwoLetterISOLanguageName;
	FString CachedNativeLanguage;
	FString CachedRegion;
	FString CachedNativeRegion;
	FString CachedScript;
	FString CachedVariant;
	bool CachedIsRightToLeft;
};
