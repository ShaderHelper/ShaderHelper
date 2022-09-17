// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "HAL/PlatformAtomics.h"
#include "Misc/AssertionMacros.h"
#include "Misc/EnumClassFlags.h"
#include "Templates/UnrealTypeTraits.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Containers/Map.h"
#include "Containers/EnumAsByte.h"
#include "Templates/SharedPointer.h"
#include "Internationalization/TextKey.h"
#include "Internationalization/LocKeyFuncs.h"
#include "Internationalization/CulturePointer.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/StringTableCoreFwd.h"
#include "Internationalization/ITextData.h"
#include "Misc/Optional.h"
#include "Templates/UniquePtr.h"
#include "Templates/IsConstructible.h"
#include "Templates/AndOrNot.h"

class FText;
class FTextHistory;
class FTextFormatData;
class FFormatArgumentValue;
class FHistoricTextFormatData;
class FHistoricTextNumericData;
class FTextFormatPatternDefinition;
class ITextGenerator;

//DECLARE_CYCLE_STAT_EXTERN( TEXT("Format Text"), STAT_TextFormat, STATGROUP_Text, );

namespace ETextFlag
{
	enum Type
	{
		Transient = (1 << 0),
		CultureInvariant = (1 << 1),
		ConvertedProperty = (1 << 2),
		Immutable = (1 << 3),
		InitializedFromString = (1<<4),  // this ftext was initialized using FromString
	};
}

namespace ETextComparisonLevel
{
	enum Type
	{
		Default,	// Locale-specific Default
		Primary,	// Base
		Secondary,	// Accent
		Tertiary,	// Case
		Quaternary,	// Punctuation
		Quinary		// Identical
	};
}

enum class ETextIdenticalModeFlags : uint8
{
	/** No special behavior */
	None = 0,

	/**
	 * Deep compare the text data.
	 *
	 * When set, two pieces of generated text (eg, from FText::Format, FText::AsNumber, FText::AsDate, FText::ToUpper, etc) 
	 * will test their internal data to see if they contain identical inputs (so would produce an identical output).
	 *
	 * When clear, no two separate pieces of generated text will ever compare as identical!
	 */
	DeepCompare = 1<<0,

	/**
	 * Compare invariant data lexically.
	 *
	 * When set, two pieces of invariant text (eg, from FText::AsCultureInvariant, FText::FromString, FText::FromName, or INVTEXT)
	 * will compare their display string data lexically to see if they are identical.
	 *
	 * When clear, no two separate pieces of invariant text will ever compare as identical!
	 */
	LexicalCompareInvariants = 1<<1,
};
ENUM_CLASS_FLAGS(ETextIdenticalModeFlags);

enum class ETextPluralType : uint8
{
	Cardinal,
	Ordinal,
};

enum class ETextPluralForm : uint8
{
	Zero = 0,
	One,	// Singular
	Two,	// Dual
	Few,	// Paucal
	Many,	// Also used for fractions if they have a separate class
	Other,	// General plural form, also used if the language only has a single form
	Count,	// Number of entries in this enum
};

/** Redeclared in KismetTextLibrary for meta-data extraction purposes, be sure to update there as well */
enum class ETextGender : uint8
{
	Masculine,
	Feminine,
	Neuter,
	// Add new enum types at the end only! They are serialized by index.
};
CORE_API bool LexTryParseString(ETextGender& OutValue, const TCHAR* Buffer);
CORE_API void LexFromString(ETextGender& OutValue, const TCHAR* Buffer);
CORE_API const TCHAR* LexToString(ETextGender InValue);

namespace EDateTimeStyle
{
	enum Type
	{
		Default,
		Short,
		Medium,
		Long,
		Full,
		Custom,	// Internal use only
		// Add new enum types at the end only! They are serialized by index.
	};
}
CORE_API bool LexTryParseString(EDateTimeStyle::Type& OutValue, const TCHAR* Buffer);
CORE_API void LexFromString(EDateTimeStyle::Type& OutValue, const TCHAR* Buffer);
CORE_API const TCHAR* LexToString(EDateTimeStyle::Type InValue);

/** Redeclared in KismetTextLibrary for meta-data extraction purposes, be sure to update there as well */
namespace EFormatArgumentType
{
	enum Type
	{
		Int,
		UInt,
		Float,
		Double,
		Text,
		Gender,
		// Add new enum types at the end only! They are serialized by index.
	};
}

typedef TMap<FString, FFormatArgumentValue, FDefaultSetAllocator, FLocKeyMapFuncs<FFormatArgumentValue>> FFormatNamedArguments;
typedef TArray<FFormatArgumentValue> FFormatOrderedArguments;

typedef TSharedRef<FTextFormatPatternDefinition, ESPMode::ThreadSafe> FTextFormatPatternDefinitionRef;
typedef TSharedPtr<FTextFormatPatternDefinition, ESPMode::ThreadSafe> FTextFormatPatternDefinitionPtr;
typedef TSharedRef<const FTextFormatPatternDefinition, ESPMode::ThreadSafe> FTextFormatPatternDefinitionConstRef;
typedef TSharedPtr<const FTextFormatPatternDefinition, ESPMode::ThreadSafe> FTextFormatPatternDefinitionConstPtr;

/** Redeclared in KismetTextLibrary for meta-data extraction purposes, be sure to update there as well */
enum ERoundingMode
{
	/** Rounds to the nearest place, equidistant ties go to the value which is closest to an even value: 1.5 becomes 2, 0.5 becomes 0 */
	HalfToEven,
	/** Rounds to nearest place, equidistant ties go to the value which is further from zero: -0.5 becomes -1.0, 0.5 becomes 1.0 */
	HalfFromZero,
	/** Rounds to nearest place, equidistant ties go to the value which is closer to zero: -0.5 becomes 0, 0.5 becomes 0. */
	HalfToZero,
	/** Rounds to the value which is further from zero, "larger" in absolute value: 0.1 becomes 1, -0.1 becomes -1 */
	FromZero,
	/** Rounds to the value which is closer to zero, "smaller" in absolute value: 0.1 becomes 0, -0.1 becomes 0 */
	ToZero,
	/** Rounds to the value which is more negative: 0.1 becomes 0, -0.1 becomes -1 */
	ToNegativeInfinity,
	/** Rounds to the value which is more positive: 0.1 becomes 1, -0.1 becomes 0 */
	ToPositiveInfinity,


	// Add new enum types at the end only! They are serialized by index.
};
CORE_API bool LexTryParseString(ERoundingMode& OutValue, const TCHAR* Buffer);
CORE_API void LexFromString(ERoundingMode& OutValue, const TCHAR* Buffer);
CORE_API const TCHAR* LexToString(ERoundingMode InValue);

enum EMemoryUnitStandard
{
	/* International Electrotechnical Commission (MiB) 1024-based */
	IEC,
	/* International System of Units 1000-based */
	SI
};

struct CORE_API FNumberFormattingOptions
{
	FNumberFormattingOptions();

	bool AlwaysSign;
	FNumberFormattingOptions& SetAlwaysSign( bool InValue ){ AlwaysSign = InValue; return *this; }

	bool UseGrouping;
	FNumberFormattingOptions& SetUseGrouping( bool InValue ){ UseGrouping = InValue; return *this; }

	ERoundingMode RoundingMode;
	FNumberFormattingOptions& SetRoundingMode( ERoundingMode InValue ){ RoundingMode = InValue; return *this; }

	int32 MinimumIntegralDigits;
	FNumberFormattingOptions& SetMinimumIntegralDigits( int32 InValue ){ MinimumIntegralDigits = InValue; return *this; }

	int32 MaximumIntegralDigits;
	FNumberFormattingOptions& SetMaximumIntegralDigits( int32 InValue ){ MaximumIntegralDigits = InValue; return *this; }

	int32 MinimumFractionalDigits;
	FNumberFormattingOptions& SetMinimumFractionalDigits( int32 InValue ){ MinimumFractionalDigits = InValue; return *this; }

	int32 MaximumFractionalDigits;
	FNumberFormattingOptions& SetMaximumFractionalDigits( int32 InValue ){ MaximumFractionalDigits = InValue; return *this; }

	friend void operator<<(FStructuredArchive::FSlot Slot, FNumberFormattingOptions& Value);

	/** Get the hash code to use for the given formatting options */
	friend uint32 GetTypeHash( const FNumberFormattingOptions& Key );

	/** Check to see if our formatting options match the other formatting options */
	bool IsIdentical( const FNumberFormattingOptions& Other ) const;

	/** Get the default number formatting options with grouping enabled */
	static const FNumberFormattingOptions& DefaultWithGrouping();

	/** Get the default number formatting options with grouping disabled */
	static const FNumberFormattingOptions& DefaultNoGrouping();
};

struct CORE_API FNumberParsingOptions
{
	FNumberParsingOptions();

	bool UseGrouping;
	FNumberParsingOptions& SetUseGrouping( bool InValue ){ UseGrouping = InValue; return *this; }

	/** The number needs to be representable inside its type limits to be considered valid. */
	bool InsideLimits;
	FNumberParsingOptions& SetInsideLimits(bool InValue) { InsideLimits = InValue; return *this; }

	/** Clamp the parsed value to its type limits. */
	bool UseClamping;
	FNumberParsingOptions& SetUseClamping(bool InValue) { UseClamping = InValue; return *this; }

	friend void operator<<(FStructuredArchive::FSlot Slot, FNumberParsingOptions& Value);

	/** Get the hash code to use for the given parsing options */
	friend uint32 GetTypeHash( const FNumberParsingOptions& Key );

	/** Check to see if our parsing options match the other parsing options */
	bool IsIdentical( const FNumberParsingOptions& Other ) const;

	/** Get the default number parsing options with grouping enabled */
	static const FNumberParsingOptions& DefaultWithGrouping();

	/** Get the default number parsing options with grouping disabled */
	static const FNumberParsingOptions& DefaultNoGrouping();
};

/**
 * Cached compiled expression used by the text formatter.
 * The compiled expression will automatically update if the display string is changed.
 * See TextFormatter.cpp for the definition.
 */
class CORE_API FTextFormat
{
	friend class FTextFormatter;

public:
	enum class EExpressionType
	{
		/** Invalid expression */
		Invalid,
		/** Simple expression, containing no arguments or argument modifiers */
		Simple,
		/** Complex expression, containing arguments or argument modifiers */
		Complex,
	};

	/**
	 * Construct an instance using an empty FText.
	 */
	FTextFormat();

	/**
	 * Construct an instance from an FText.
	 * The text will be immediately compiled. 
	 */
	FTextFormat(const FText& InText);

	/**
	 * Construct an instance from an FText and custom format pattern definition.
	 * The text will be immediately compiled.
	 */
	FTextFormat(const FText& InText, FTextFormatPatternDefinitionConstRef InCustomPatternDef);

	/**
	 * Construct an instance from an FString.
	 * The string will be immediately compiled.
	 */
	static FTextFormat FromString(const FString& InString);
	static FTextFormat FromString(FString&& InString);

	/**
	 * Construct an instance from an FString and custom format pattern definition.
	 * The string will be immediately compiled.
	 */
	static FTextFormat FromString(const FString& InString, FTextFormatPatternDefinitionConstRef InCustomPatternDef);
	static FTextFormat FromString(FString&& InString, FTextFormatPatternDefinitionConstRef InCustomPatternDef);

	/**
	 * Test to see whether this instance contains valid compiled data.
	 */
	bool IsValid() const;

	/**
	 * Check whether this instance is considered identical to the other instance, based on the comparison flags provided.
	 */
	bool IdenticalTo(const FTextFormat& Other, const ETextIdenticalModeFlags CompareModeFlags) const;

	/**
	 * Get the source text that we're holding.
	 * If we're holding a string then we'll construct a new text.
	 */
	FText GetSourceText() const;

	/**
	 * Get the source string that we're holding.
	 * If we're holding a text then we'll return its internal string.
	 */
	const FString& GetSourceString() const;

	/**
	 * Get the type of expression currently compiled.
	 */
	EExpressionType GetExpressionType() const;

	/**
	 * Get the format pattern definition being used.
	 */
	FTextFormatPatternDefinitionConstRef GetPatternDefinition() const;

	/**
	 * Validate the format pattern is valid based on the rules of the given culture (or null to use the current language).
	 * @return true if the pattern is valid, or false if not (false may also fill in OutValidationErrors).
	 */
	bool ValidatePattern(const FCulturePtr& InCulture, TArray<FString>& OutValidationErrors) const;

	/**
	 * Append the names of any arguments to the given array.
	 */
	void GetFormatArgumentNames(TArray<FString>& OutArgumentNames) const;

private:
	/**
	 * Construct an instance from an FString.
	 * The string will be immediately compiled.
	 */
	FTextFormat(FString&& InString, FTextFormatPatternDefinitionConstRef InCustomPatternDef);

	/** Cached compiled expression data */
	TSharedRef<FTextFormatData, ESPMode::ThreadSafe> TextFormatData;
};

class FCulture;

class CORE_API FText
{
public:

	static const FText& GetEmpty();

public:

	FText();
	FText(const FText&) = default;
	FText(FText&&) = default;

	FText& operator=(const FText&) = default;
	FText& operator=(FText&&) = default;

	/**
	 * Generate an FText that represents the passed number in the current culture
	 */
	static FText AsNumber(float Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(double Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(int8 Val,		const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(int16 Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(int32 Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(int64 Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(uint8 Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(uint16 Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(uint32 Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(uint64 Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsNumber(long Val,		const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);


	static FText AsCurrency(float Val,  const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(double Val, const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(int8 Val,   const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(int16 Val,  const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(int32 Val,  const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(int64 Val,  const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(uint8 Val,  const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(uint16 Val, const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(uint32 Val, const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(uint64 Val, const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsCurrency(long Val,   const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);

	/**
	 * Generate an FText that represents the passed number as currency in the current culture.
	 * BaseVal is specified in the smallest fractional value of the currency and will be converted for formatting according to the selected culture.
	 * Keep in mind the CurrencyCode is completely independent of the culture it's displayed in (and they do not imply one another).
	 * For example: FText::AsCurrencyBase(650, TEXT("EUR")); would return an FText of "<EUR>6.50" in most English cultures (en_US/en_UK) and "6,50<EUR>" in Spanish (es_ES) (where <EUR> is U+20AC)
	 */
	static FText AsCurrencyBase(int64 BaseVal, const FString& CurrencyCode, const FCulturePtr& TargetCulture = NULL, int32 ForceDecimalPlaces = -1);

	/**
	 * Generate an FText that represents the passed number as a percentage in the current culture
	 */
	static FText AsPercent(float Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);
	static FText AsPercent(double Val,	const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL);

	/**
	 * Generate an FText that represents the passed number as a date and/or time in the current culture
	 * @note The overload using a custom pattern uses strftime-like syntax (see FDateTime::ToFormattedString)
	 */
	static FText AsDate(const FDateTime& DateTime, const EDateTimeStyle::Type DateStyle = EDateTimeStyle::Default, const FString& TimeZone = FString(), const FCulturePtr& TargetCulture = NULL);
	static FText AsDateTime(const FDateTime& DateTime, const EDateTimeStyle::Type DateStyle = EDateTimeStyle::Default, const EDateTimeStyle::Type TimeStyle = EDateTimeStyle::Default, const FString& TimeZone = FString(), const FCulturePtr& TargetCulture = NULL);
	static FText AsDateTime(const FDateTime& DateTime, const FString& CustomPattern, const FString& TimeZone = FString(), const FCulturePtr& TargetCulture = NULL);
	static FText AsTime(const FDateTime& DateTime, const EDateTimeStyle::Type TimeStyle = EDateTimeStyle::Default, const FString& TimeZone = FString(), const FCulturePtr& TargetCulture = NULL);
	static FText AsTimespan(const FTimespan& Timespan, const FCulturePtr& TargetCulture = NULL);

	/**
	 * Gets the time zone string that represents a non-specific, zero offset, culture invariant time zone.
	 */
	static FString GetInvariantTimeZone();

	/**
	 * Generate an FText that represents the passed number as a memory size in the current culture
	 */
	static FText AsMemory(uint64 NumBytes, const FNumberFormattingOptions* const Options = NULL, const FCulturePtr& TargetCulture = NULL, EMemoryUnitStandard UnitStandard = EMemoryUnitStandard::IEC);

	/**
	 * Generate an FText that represents the passed number as a memory size in the current culture
	 */
	static FText AsMemory(uint64 NumBytes, EMemoryUnitStandard UnitStandard);

	/**
	 * Attempts to find an existing FText using the representation found in the loc tables for the specified namespace and key
	 * @return true if OutText was properly set; otherwise false and OutText will be untouched
	 */
	static bool FindText( const FTextKey& Namespace, const FTextKey& Key, FText& OutText, const FString* const SourceString = nullptr );

	/**
	 * Attempts to create an FText instance from a string table ID and key (this is the same as the LOCTABLE macro, except this can also work with non-literal string values).
	 * @return The found text, or a dummy FText if not found.
	 */
	static FText FromStringTable(const FName InTableId, const FString& InKey, const EStringTableLoadingPolicy InLoadingPolicy = EStringTableLoadingPolicy::FindOrLoad);

	/**
	 * Generate an FText representing the pass name
	 */
	static FText FromName( const FName& Val);
	
	/**
	 * Generate an FText representing the passed in string
	 */
	static FText FromString( const FString& String );
	static FText FromString( FString&& String );

	/**
	 * Generate a FText representing the passed string view
	 */
	static FText FromStringView(FStringView InString);

	/**
	 * Generate a culture invariant FText representing the passed in string
	 */
	static FText AsCultureInvariant( const FString& String );
	static FText AsCultureInvariant( FString&& String );

	/**
	 * Generate a culture invariant FText representing the passed in FText
	 */
	static FText AsCultureInvariant( FText Text );

	const FString& ToString() const;

	/** Deep build of the source string for this FText, climbing the history hierarchy */
	FString BuildSourceString() const;

	bool IsNumeric() const;

	int32 CompareTo( const FText& Other, const ETextComparisonLevel::Type ComparisonLevel = ETextComparisonLevel::Default ) const;
	int32 CompareToCaseIgnored( const FText& Other ) const;

	bool EqualTo( const FText& Other, const ETextComparisonLevel::Type ComparisonLevel = ETextComparisonLevel::Default ) const;
	bool EqualToCaseIgnored( const FText& Other ) const;

	/**
	 * Check to see if this FText is identical to the other FText
	 * 
	 * @note This function defaults to only testing that the internal data has the same target (which makes it very fast!), rather than performing any deep or lexical analysis.
	 *       The ETextIdenticalModeFlags can modify this default behavior. See the comments on those flag options for more information.
	 *
	 * @note If you actually want to perform a full lexical comparison, then you need to use EqualTo instead.
	 */
	bool IdenticalTo( const FText& Other, const ETextIdenticalModeFlags CompareModeFlags = ETextIdenticalModeFlags::None ) const;

	class CORE_API FSortPredicate
	{
	public:
		FSortPredicate(const ETextComparisonLevel::Type ComparisonLevel = ETextComparisonLevel::Default);

		bool operator()(const FText& A, const FText& B) const;

	private:
#if UE_ENABLE_ICU
		class FSortPredicateImplementation;
		TSharedRef<FSortPredicateImplementation> Implementation;
#endif
	};

	bool IsEmpty() const;

	bool IsEmptyOrWhitespace() const;

	/**
	 * Transforms the text to lowercase in a culture correct way.
	 * @note The returned instance is linked to the original and will be rebuilt if the active culture is changed.
	 */
	FText ToLower() const;

	/**
	 * Transforms the text to uppercase in a culture correct way.
	 * @note The returned instance is linked to the original and will be rebuilt if the active culture is changed.
	 */
	FText ToUpper() const;

	/**
	 * Removes any whitespace characters from the start of the text.
	 */
	static FText TrimPreceding( const FText& );

	/**
	 * Removes any whitespace characters from the end of the text.
	 */
	static FText TrimTrailing( const FText& );

	/**
	 * Removes any whitespace characters from the start and end of the text.
	 */
	static FText TrimPrecedingAndTrailing( const FText& );

	/**
	 * Check to see if the given character is considered whitespace by the current culture
	 */
	static bool IsWhitespace( const TCHAR Char );

	static void GetFormatPatternParameters(const FTextFormat& Fmt, TArray<FString>& ParameterNames);

	static FText Format(FTextFormat Fmt, const FFormatNamedArguments& InArguments);
	static FText Format(FTextFormat Fmt, FFormatNamedArguments&& InArguments);

	static FText Format(FTextFormat Fmt, const FFormatOrderedArguments& InArguments);
	static FText Format(FTextFormat Fmt, FFormatOrderedArguments&& InArguments);

	template <typename... ArgTypes>
	static FORCEINLINE FText Format(FTextFormat Fmt, ArgTypes... Args)
	{
		static_assert(TAnd<TIsConstructible<FFormatArgumentValue, ArgTypes>...>::Value, "Invalid argument type passed to FText::Format");
		static_assert(sizeof...(Args) > 0, "FText::Format expects at least one non-format argument"); // we do this to ensure that people don't call Format for no good reason

		// We do this to force-select the correct overload, because overload resolution will cause compile
		// errors when it tries to instantiate the FFormatNamedArguments overloads.
		FText (*CorrectFormat)(FTextFormat, FFormatOrderedArguments&&) = Format;

		return CorrectFormat(MoveTemp(Fmt), FFormatOrderedArguments{ MoveTemp(Args)... });
	}

	/**
	 * FormatNamed allows you to pass name <-> value pairs to the function to format automatically
	 *
	 * @usage FText::FormatNamed( FText::FromString( TEXT( "{PlayerName} is really cool" ) ), TEXT( "PlayerName" ), FText::FromString( TEXT( "Awesomegirl" ) ) );
	 *
	 * @param Fmt the format to create from
	 * @param Args a variadic list of FString to Value (must be even numbered)
	 * @return a formatted FText
	 */
	template < typename... TArguments >
	static FText FormatNamed( FTextFormat Fmt, TArguments&&... Args );

	/**
	 * FormatOrdered allows you to pass a variadic list of types to use for formatting in order desired
	 *
	 * @param Fmt the format to create from
	 * @param Args a variadic list of values in order of desired formatting
	 * @return a formatted FText
	 */
	template < typename... TArguments >
	static FText FormatOrdered( FTextFormat Fmt, TArguments&&... Args );

	/**
	 * Join an arbitrary list of formattable values together, separated by the given delimiter
	 * @note Internally this uses FText::Format with a generated culture invariant format pattern
	 *
	 * @param Delimiter The delimiter to insert between the items
	 * @param Args An array of formattable values to join together
	 * @return The joined FText
	 */
	static FText Join(const FText& Delimiter, const FFormatOrderedArguments& Args);
	static FText Join(const FText& Delimiter, const TArray<FText>& Args);

	/**
	 * Join an arbitrary list of formattable items together, separated by the given delimiter
	 * @note Internally this uses FText::Format with a generated culture invariant format pattern
	 *
	 * @param Delimiter The delimiter to insert between the items
	 * @param Args A variadic list of values to join together
	 * @return The joined FText
	 */
	template <typename... ArgTypes>
	static FORCEINLINE FText Join(const FText& Delimiter, ArgTypes... Args)
	{
		static_assert(TAnd<TIsConstructible<FFormatArgumentValue, ArgTypes>...>::Value, "Invalid argument type passed to FText::Join");
		static_assert(sizeof...(Args) > 0, "FText::Join expects at least one non-format argument"); // we do this to ensure that people don't call Join for no good reason

		return Join(Delimiter, FFormatOrderedArguments{ MoveTemp(Args)... });
	}

	/**
	 * Produces a custom-generated FText. Can be used for objects that produce text dependent on localized strings but
	 * that do not fit the standard formats.
	 *
	 * @param TextGenerator the text generator object that will generate the text
	 */
	static FText FromTextGenerator( const TSharedRef<ITextGenerator>& TextGenerator );

	DECLARE_DELEGATE_RetVal_OneParam( TSharedRef<ITextGenerator>, FCreateTextGeneratorDelegate, FStructuredArchive::FRecord );
	/**
	 * Returns the text generator factory function registered under the specified name, if any.
	 *
	 * @param TypeID the name under which to look up the factory function
	 */
	static FCreateTextGeneratorDelegate FindRegisteredTextGenerator( FName TypeID );

	/**
	 * Registers a factory function to be used with serialization of text generators within FText.
	 *
	 * @param TypeID the name under which to register the factory function. Must match ITextGenerator::GetTypeID().
	 * @param FactoryFunction the factory function to create the generator instance
	 */
	static void RegisterTextGenerator( FName TypeID, FCreateTextGeneratorDelegate FactoryFunction );

	/**
	 * Registers a standard text generator factory function.
	 *
	 * @tparam T the text generator class type
	 *
	 * @param TypeID the name under which to register the factor function
	 */
	template < typename T >
	static void RegisterTextGenerator( FName TypeID )
	{
		RegisterTextGenerator(TypeID, FCreateTextGeneratorDelegate::CreateStatic( &CreateTextGenerator<T> ));
	}

	/**
	 * Registers a standard text generator factory function.
	 *
	 * @tparam T the text generator class type
	 *
	 * This function can be used if the class has a public static FName member named "TypeID".
	 */
	template < typename T >
	static void RegisterTextGenerator()
	{
		RegisterTextGenerator<T>( T::TypeID );
	}

	/**
	 * Unregisters a factory function to be used with serialization of text generators within FText.
	 *
	 * @param TypeID the name to remove from registration
	 *
	 * @see RegisterTextGenerator
	 */
	static void UnregisterTextGenerator( FName TypeID );

	/**
	 * Unregisters a standard text generator factory function.
	 *
	 * This function can be used if the class has a public static FName member named "TypeID".
	 *
	 * @tparam T the text generator class type
	 */
	template < typename T >
	static void UnregisterTextGenerator()
	{
		UnregisterTextGenerator( T::TypeID );
	}

	bool IsTransient() const;
	bool IsCultureInvariant() const;
	bool IsInitializedFromString() const;
	bool IsFromStringTable() const;

	bool ShouldGatherForLocalization() const;

#if WITH_EDITORONLY_DATA
	/**
	 * Constructs a new FText with the SourceString of the specified text but with the specified namespace and key
	 */
	static FText ChangeKey( const FTextKey& Namespace, const FTextKey& Key, const FText& Text );
#endif

private:
	/** Special constructor used to create StaticEmptyText without also allocating a history object */
	enum class EInitToEmptyString : uint8 { Value };
	explicit FText( EInitToEmptyString );

	explicit FText( TSharedRef<ITextData, ESPMode::ThreadSafe> InTextData );

	explicit FText( FString&& InSourceString );

	FText( FName InTableId, FString InKey, const EStringTableLoadingPolicy InLoadingPolicy );

	FText( FString&& InSourceString, const FTextKey& InNamespace, const FTextKey& InKey, uint32 InFlags=0 );

	static void SerializeText( FArchive& Ar, FText& Value );
	static void SerializeText(FStructuredArchive::FSlot Slot, FText& Value);

	/** Get any historic text format data from the history used by this FText */
	void GetHistoricFormatData(TArray<FHistoricTextFormatData>& OutHistoricFormatData) const;

	/** Get any historic numeric format data from the history used by this FText */
	bool GetHistoricNumericData(FHistoricTextNumericData& OutHistoricNumericData) const;

	/** Rebuilds the FText under the current culture if needed */
	void Rebuild() const;

	static FText FormatNamedImpl(FTextFormat&& Fmt, FFormatNamedArguments&& InArguments);
	static FText FormatOrderedImpl(FTextFormat&& Fmt, FFormatOrderedArguments&& InArguments);

private:
	template<typename T1, typename T2>
	static FText AsNumberTemplate(T1 Val, const FNumberFormattingOptions* const Options, const FCulturePtr& TargetCulture);
	template<typename T1, typename T2>
	static FText AsCurrencyTemplate(T1 Val, const FString& CurrencyCode, const FNumberFormattingOptions* const Options, const FCulturePtr& TargetCulture);
	template<typename T1, typename T2>
	static FText AsPercentTemplate(T1 Val, const FNumberFormattingOptions* const Options, const FCulturePtr& TargetCulture);

private:
	template < typename T >
	static TSharedRef<ITextGenerator> CreateTextGenerator(FStructuredArchive::FRecord Record);

private:
	/** The internal shared data for this FText */
	TSharedRef<ITextData, ESPMode::ThreadSafe> TextData;

	/** Flags with various information on what sort of FText this is */
	uint32 Flags;

public:
	friend class FTextCache;
	friend class FTextFormatter;
	friend class FTextFormatData;
	friend class FTextSnapshot;
	friend class FTextInspector;
	friend class FTextStringHelper;
	friend class FStringTableRegistry;
	friend class FArchive;
	friend class FArchiveFromStructuredArchiveImpl;
	friend class FJsonArchiveInputFormatter;
	friend class FJsonArchiveOutputFormatter;
	friend class FTextProperty;
	friend class FFormatArgumentValue;
	friend class FTextHistory_NamedFormat;
	friend class FTextHistory_ArgumentDataFormat;
	friend class FTextHistory_OrderedFormat;
	friend class FTextHistory_Transform;
	friend class FScopedTextIdentityPreserver;
};

class CORE_API FFormatArgumentValue
{
public:
	FFormatArgumentValue()
		: Type(EFormatArgumentType::Text)
		, TextValue(FText::GetEmpty())
	{
	}

	FFormatArgumentValue(const int32 Value)
		: Type(EFormatArgumentType::Int)
	{
		IntValue = Value;
	}

	FFormatArgumentValue(const uint32 Value)
		: Type(EFormatArgumentType::UInt)
	{
		UIntValue = Value;
	}

	FFormatArgumentValue(const int64 Value)
		: Type(EFormatArgumentType::Int)
	{
		IntValue = Value;
	}

	FFormatArgumentValue(const uint64 Value)
		: Type(EFormatArgumentType::UInt)
	{
		UIntValue = Value;
	}

	FFormatArgumentValue(const float Value)
		: Type(EFormatArgumentType::Float)
	{
		FloatValue = Value;
	}

	FFormatArgumentValue(const double Value)
		: Type(EFormatArgumentType::Double)
	{
		DoubleValue = Value;
	}

	FFormatArgumentValue(const FText& Value)
		: Type(EFormatArgumentType::Text)
		, TextValue(Value)
	{
	}

	FFormatArgumentValue(FText&& Value)
		: Type(EFormatArgumentType::Text)
		, TextValue(MoveTemp(Value))
	{
	}

	FFormatArgumentValue(ETextGender Value)
		: Type(EFormatArgumentType::Gender)
	{
		UIntValue = (uint64)Value;
	}

	friend void operator<<(FStructuredArchive::FSlot Slot, FFormatArgumentValue& Value);

	bool IdenticalTo(const FFormatArgumentValue& Other, const ETextIdenticalModeFlags CompareModeFlags) const;

	FString ToFormattedString(const bool bInRebuildText, const bool bInRebuildAsSource) const;
	void ToFormattedString(const bool bInRebuildText, const bool bInRebuildAsSource, FString& OutResult) const;

	FString ToExportedString(const bool bStripPackageNamespace = false) const;
	void ToExportedString(FString& OutResult, const bool bStripPackageNamespace = false) const;
	const TCHAR* FromExportedString(const TCHAR* InBuffer);

	FORCEINLINE EFormatArgumentType::Type GetType() const
	{
		return Type;
	}

	FORCEINLINE int64 GetIntValue() const
	{
		check(Type == EFormatArgumentType::Int);
		return IntValue;
	}

	FORCEINLINE uint64 GetUIntValue() const
	{
		check(Type == EFormatArgumentType::UInt);
		return UIntValue;
	}

	FORCEINLINE float GetFloatValue() const
	{
		check(Type == EFormatArgumentType::Float);
		return FloatValue;
	}

	FORCEINLINE double GetDoubleValue() const
	{
		check(Type == EFormatArgumentType::Double);
		return DoubleValue;
	}

	FORCEINLINE const FText& GetTextValue() const
	{
		check(Type == EFormatArgumentType::Text);
		return TextValue.GetValue();
	}

	FORCEINLINE ETextGender GetGenderValue() const
	{
		check(Type == EFormatArgumentType::Gender);
		return (ETextGender)UIntValue;
	}

private:
	EFormatArgumentType::Type Type;
	union
	{
		int64 IntValue;
		uint64 UIntValue;
		float FloatValue;
		double DoubleValue;
	};
	TOptional<FText> TextValue;
};

template < typename T >
inline TSharedRef<ITextGenerator> FText::CreateTextGenerator(FStructuredArchive::FRecord Record)
{
	return MakeShared<T>();
}

/**
 * Used to pass argument/value pairs into FText::Format via UKismetTextLibrary::Format.
 * @note The primary consumer of this type is Blueprints (via a UHT mirror node). It is *not* expected that this be used in general C++ as FFormatArgumentValue is a much better type.
 * The UHT struct is located here: Engine\Source\Runtime\Engine\Classes\Kismet\KismetTextLibrary.h
 */
struct CORE_API FFormatArgumentData
{
	FFormatArgumentData()
	{
		ResetValue();
	}

	void ResetValue();

	FFormatArgumentValue ToArgumentValue() const;

	friend void operator<<(FStructuredArchive::FSlot Slot, FFormatArgumentData& Value);

	FString ArgumentName;

	// This is a non-unioned version of FFormatArgumentValue that only accepts the types needed by Blueprints
	// It's used as a marshaller to create a real FFormatArgumentValue when performing a format
	TEnumAsByte<EFormatArgumentType::Type> ArgumentValueType;
	FText ArgumentValue;
	int64 ArgumentValueInt;
	float ArgumentValueFloat;
	double ArgumentValueDouble;
	ETextGender ArgumentValueGender;
};

namespace TextFormatUtil
{

	template < typename TName, typename TValue >
	void FormatNamed( OUT FFormatNamedArguments& Result, TName&& Name, TValue&& Value )
	{
		Result.Emplace( Forward< TName >( Name ), Forward< TValue >( Value ) );
	}
	
	template < typename TName, typename TValue, typename... TArguments >
	void FormatNamed( OUT FFormatNamedArguments& Result, TName&& Name, TValue&& Value, TArguments&&... Args )
	{
		FormatNamed( Result, Forward< TName >( Name ), Forward< TValue >( Value ) );
		FormatNamed( Result, Forward< TArguments >( Args )... );
	}
	
	template < typename TValue >
	void FormatOrdered( OUT FFormatOrderedArguments& Result, TValue&& Value )
	{
		Result.Emplace( Forward< TValue >( Value ) );
	}
	
	template < typename TValue, typename... TArguments >
	void FormatOrdered( OUT FFormatOrderedArguments& Result, TValue&& Value, TArguments&&... Args )
	{
		FormatOrdered( Result, Forward< TValue >( Value ) );
		FormatOrdered( Result, Forward< TArguments >( Args )... );
	}

} // namespace TextFormatUtil

template < typename... TArguments >
FText FText::FormatNamed( FTextFormat Fmt, TArguments&&... Args )
{
	static_assert( sizeof...( TArguments ) % 2 == 0, "FormatNamed requires an even number of Name <-> Value pairs" );

	FFormatNamedArguments FormatArguments;
	FormatArguments.Reserve( sizeof...( TArguments ) / 2 );
	TextFormatUtil::FormatNamed( FormatArguments, Forward< TArguments >( Args )... );
	return FormatNamedImpl( MoveTemp( Fmt ), MoveTemp( FormatArguments ) );
}

template < typename... TArguments >
FText FText::FormatOrdered( FTextFormat Fmt, TArguments&&... Args )
{
	FFormatOrderedArguments FormatArguments;
	FormatArguments.Reserve( sizeof...( TArguments ) );
	TextFormatUtil::FormatOrdered( FormatArguments, Forward< TArguments >( Args )... );
	return FormatOrderedImpl( MoveTemp( Fmt ), MoveTemp( FormatArguments ) );
}

/** Used to gather information about a historic text format operation */
class CORE_API FHistoricTextFormatData
{
public:
	FHistoricTextFormatData()
	{
	}

	FHistoricTextFormatData(FText InFormattedText, FTextFormat&& InSourceFmt, FFormatNamedArguments&& InArguments)
		: FormattedText(MoveTemp(InFormattedText))
		, SourceFmt(MoveTemp(InSourceFmt))
		, Arguments(MoveTemp(InArguments))
	{
	}

	/** The final formatted text this data is for */
	FText FormattedText;

	/** The pattern used to format the text */
	FTextFormat SourceFmt;

	/** Arguments to replace in the pattern string */
	FFormatNamedArguments Arguments;
};

/** Used to gather information about a historic numeric format operation */
class CORE_API FHistoricTextNumericData
{
public:
	enum class EType : uint8
	{
		AsNumber,
		AsPercent,
	};

	FHistoricTextNumericData()
		: FormatType(EType::AsNumber)
	{
	}

	FHistoricTextNumericData(const EType InFormatType, const FFormatArgumentValue& InSourceValue, const TOptional<FNumberFormattingOptions>& InFormatOptions)
		: FormatType(InFormatType)
		, SourceValue(InSourceValue)
		, FormatOptions(InFormatOptions)
	{
	}

	/** Type of numeric format that was performed */
	EType FormatType;

	/** The source number to format */
	FFormatArgumentValue SourceValue;

	/** Custom formatting options used when formatting this number (if any) */
	TOptional<FNumberFormattingOptions> FormatOptions;
};

/** A snapshot of an FText at a point in time that can be used to detect changes in the FText, including live-culture changes */
class CORE_API FTextSnapshot
{
public:
	FTextSnapshot();

	explicit FTextSnapshot(const FText& InText);

	/** Check to see whether the given text is identical to the text this snapshot was made from */
	bool IdenticalTo(const FText& InText) const;

	/** Check to see whether the display string of the given text is identical to the display string this snapshot was made from */
	bool IsDisplayStringEqualTo(const FText& InText) const;

private:

	/** Get adjusted global history revision used for comparison */
	static uint16 GetGlobalHistoryRevisionForText(const FText& InText);

	/** Get adjusted local history revision used for comparison */
	static uint16 GetLocalHistoryRevisionForText(const FText& InText);

	/** A pointer to the text data for the FText that we took a snapshot of (used for an efficient pointer compare) */
	TSharedPtr<ITextData, ESPMode::ThreadSafe> TextDataPtr;

	/** The localized string of the text when we took the snapshot (if any) */
	FTextConstDisplayStringPtr LocalizedStringPtr;

	/** Global revision index of the text when we took the snapshot, or 0 if there was no history */
	uint16 GlobalHistoryRevision = 0;

	/** Local revision index of the text when we took the snapshot, or 0 if there was no history */
	uint16 LocalHistoryRevision = 0;

	/** Flags with various information on what sort of FText we took a snapshot of */
	uint32 Flags = 0;
};

class CORE_API FTextInspector
{
private:
	FTextInspector() {}
	~FTextInspector() {}

public:
	static bool ShouldGatherForLocalization(const FText& Text);
	static TOptional<FString> GetNamespace(const FText& Text);
	static TOptional<FString> GetKey(const FText& Text);
	static FTextId GetTextId(const FText& Text);
	static const FString* GetSourceString(const FText& Text);
	static const FString& GetDisplayString(const FText& Text);
	UE_DEPRECATED(5.0, "GetSharedDisplayString is no longer guaranteed to return a valid result and should NOT be used! If you wanted to get the text ID, use FTextInspector::GetTextId instead. If you wanted a key for unique text instances, use FTextInspector::GetSharedDataId instead.")
	static FTextConstDisplayStringPtr GetSharedDisplayString(const FText& Text);
	static bool GetTableIdAndKey(const FText& Text, FName& OutTableId, FString& OutKey);
	static bool GetTableIdAndKey(const FText& Text, FName& OutTableId, FTextKey& OutKey);
	static uint32 GetFlags(const FText& Text);
	static void GetHistoricFormatData(const FText& Text, TArray<FHistoricTextFormatData>& OutHistoricFormatData);
	static bool GetHistoricNumericData(const FText& Text, FHistoricTextNumericData& OutHistoricNumericData);
	static const void* GetSharedDataId(const FText& Text);
};

class CORE_API FTextStringHelper
{
public:
	/**
	 * Create an FText instance from the given stream of text.
	 * @note This uses ReadFromBuffer internally, but will fallback to FText::FromString if ReadFromBuffer fails to parse the buffer.
	 *
	 * @param Buffer			The buffer of text to read from (null terminated).
	 * @param TextNamespace		An optional namespace to use when parsing texts that use LOCTEXT (default is an empty namespace).
	 * @param PackageNamespace	The package namespace of the containing object (if loading for a property - see TextNamespaceUtil::GetPackageNamespace).
	 * @param bRequiresQuotes	True if the read text literal must be surrounded by quotes (eg, when loading from a delimited list).
	 *
	 * @return The parsed FText instance.
	 */
	static FText CreateFromBuffer(const TCHAR* Buffer, const TCHAR* TextNamespace = nullptr, const TCHAR* PackageNamespace = nullptr, const bool bRequiresQuotes = false);

	/**
	 * Attempt to extract an FText instance from the given stream of text.
	 *
	 * @param Buffer			The buffer of text to read from (null terminated).
	 * @param OutValue			The text value to fill with the read text.
	 * @param TextNamespace		An optional namespace to use when parsing texts that use LOCTEXT (default is an empty namespace).
	 * @param PackageNamespace	The package namespace of the containing object (if loading for a property - see TextNamespaceUtil::GetPackageNamespace).
	 * @param bRequiresQuotes	True if the read text literal must be surrounded by quotes (eg, when loading from a delimited list).
	 *
	 * @return The updated buffer after we parsed this text, or nullptr on failure
	 */
	static const TCHAR* ReadFromBuffer(const TCHAR* Buffer, FText& OutValue, const TCHAR* TextNamespace = nullptr, const TCHAR* PackageNamespace = nullptr, const bool bRequiresQuotes = false);
	
	UE_DEPRECATED(4.22, "FTextStringHelper::ReadFromString is deprecated. Use FTextStringHelper::ReadFromBuffer instead.")
	static bool ReadFromString(const TCHAR* Buffer, FText& OutValue, const TCHAR* TextNamespace = nullptr, const TCHAR* PackageNamespace = nullptr, int32* OutNumCharsRead = nullptr, const bool bRequiresQuotes = false, const EStringTableLoadingPolicy InLoadingPolicy = EStringTableLoadingPolicy::FindOrLoad);

	/**
	 * Write the given FText instance to a stream of text
	 *
	 * @param Buffer				 The buffer of text to write to.
	 * @param Value					 The text value to write into the buffer.
	 * @param bRequiresQuotes		 True if the written text literal must be surrounded by quotes (eg, when saving as a delimited list)
	 * @param bStripPackageNamespace True to strip the package namespace from the written NSLOCTEXT value (eg, when saving cooked data)
	 */
	static void WriteToBuffer(FString& Buffer, const FText& Value, const bool bRequiresQuotes = false, const bool bStripPackageNamespace = false);
	
	UE_DEPRECATED(4.22, "FTextStringHelper::WriteToString is deprecated. Use FTextStringHelper::WriteToBuffer instead.")
	static bool WriteToString(FString& Buffer, const FText& Value, const bool bRequiresQuotes = false);

	/**
	 * Test to see whether a given buffer contains complex text.
	 *
	 * @return True if it does, false otherwise
	 */
	static bool IsComplexText(const TCHAR* Buffer);

private:
	static const TCHAR* ReadFromBuffer_ComplexText(const TCHAR* Buffer, FText& OutValue, const TCHAR* TextNamespace, const TCHAR* PackageNamespace);
};

class CORE_API FTextBuilder
{
public:
	/**
	 * Increase the running indentation of the builder.
	 */
	void Indent();

	/**
	 * Decrease the running indentation of the builder.
	 */
	void Unindent();

	/**
	 * Append an empty line to the builder, indented by the running indentation of the builder.
	 */
	void AppendLine();

	/**
	 * Append the given text line to the builder, indented by the running indentation of the builder.
	 */
	void AppendLine(const FText& Text);

	/**
	 * Append the given string line to the builder, indented by the running indentation of the builder.
	 */
	void AppendLine(const FString& String);

	/**
	 * Append the given name line to the builder, indented by the running indentation of the builder.
	 */
	void AppendLine(const FName& Name);

	/**
	 * Append the given formatted text line to the builder, indented by the running indentation of the builder.
	 */
	void AppendLineFormat(const FTextFormat& Pattern, const FFormatNamedArguments& Arguments);

	/**
	 * Append the given formatted text line to the builder, indented by the running indentation of the builder.
	 */
	void AppendLineFormat(const FTextFormat& Pattern, const FFormatOrderedArguments& Arguments);

	/**
	 * Append the given formatted text line to the builder, indented by the running indentation of the builder.
	 */
	template <typename... ArgTypes>
	FORCEINLINE void AppendLineFormat(FTextFormat Pattern, ArgTypes... Args)
	{
		static_assert(TAnd<TIsConstructible<FFormatArgumentValue, ArgTypes>...>::Value, "Invalid argument type passed to FTextBuilder::AppendLineFormat");
		static_assert(sizeof...(Args) > 0, "FTextBuilder::AppendLineFormat expects at least one non-format argument"); // we do this to ensure that people don't call AppendLineFormat for no good reason

		BuildAndAppendLine(FText::Format(MoveTemp(Pattern), FFormatOrderedArguments{ MoveTemp(Args)... }));
	}

	/**
	 * Clear the builder and reset it to its default state.
	 */
	void Clear();

	/**
	 * Check to see if the builder has any data.
	 */
	bool IsEmpty();

	/**
	 * Build the current set of input into a FText.
	 */
	FText ToText() const;

private:
	void BuildAndAppendLine(FString&& Data);
	void BuildAndAppendLine(FText&& Data);

	TArray<FText> Lines;
	int32 IndentCount = 0;
};

/** Unicode character helper functions */
struct CORE_API FUnicodeChar
{
	static bool CodepointToString(const uint32 InCodepoint, FString& OutString);
};

/**
 * Unicode Bidirectional text support 
 * http://www.unicode.org/reports/tr9/
 */
namespace TextBiDi
{
	/** Lists the potential reading directions for text */
	enum class ETextDirection : uint8
	{
		/** Contains only LTR text - requires simple LTR layout */
		LeftToRight,
		/** Contains only RTL text - requires simple RTL layout */
		RightToLeft,
		/** Contains both LTR and RTL text - requires more complex layout using multiple runs of text */
		Mixed,
	};

	/** A single complex layout entry. Defines the starting position, length, and reading direction for a sub-section of text */
	struct FTextDirectionInfo
	{
		int32 StartIndex;
		int32 Length;
		ETextDirection TextDirection;
	};

	/** Defines the interface for a re-usable BiDi object */
	class CORE_API ITextBiDi
	{
	public:
		virtual ~ITextBiDi() {}

		/** See TextBiDi::ComputeTextDirection */
		virtual ETextDirection ComputeTextDirection(const FText& InText) = 0;
		virtual ETextDirection ComputeTextDirection(const FString& InString) = 0;
		virtual ETextDirection ComputeTextDirection(const TCHAR* InString, const int32 InStringStartIndex, const int32 InStringLen) = 0;

		/** See TextBiDi::ComputeTextDirection */
		virtual ETextDirection ComputeTextDirection(const FText& InText, const ETextDirection InBaseDirection, TArray<FTextDirectionInfo>& OutTextDirectionInfo) = 0;
		virtual ETextDirection ComputeTextDirection(const FString& InString, const ETextDirection InBaseDirection, TArray<FTextDirectionInfo>& OutTextDirectionInfo) = 0;
		virtual ETextDirection ComputeTextDirection(const TCHAR* InString, const int32 InStringStartIndex, const int32 InStringLen, const ETextDirection InBaseDirection, TArray<FTextDirectionInfo>& OutTextDirectionInfo) = 0;

		/** See TextBiDi::ComputeBaseDirection */
		virtual ETextDirection ComputeBaseDirection(const FText& InText) = 0;
		virtual ETextDirection ComputeBaseDirection(const FString& InString) = 0;
		virtual ETextDirection ComputeBaseDirection(const TCHAR* InString, const int32 InStringStartIndex, const int32 InStringLen) = 0;
	};

	/**
	 * Create a re-usable BiDi object.
	 * This may yield better performance than the utility functions if you're performing a lot of BiDi requests, as this object can re-use allocated data between requests.
	 */
	CORE_API TUniquePtr<ITextBiDi> CreateTextBiDi();

	/**
	 * Utility function which will compute the reading direction of the given text.
	 * @note You may want to use the version that returns you the advanced layout data in the Mixed case.
	 * @return LeftToRight if all of the text is LTR, RightToLeft if all of the text is RTL, or Mixed if the text contains both LTR and RTL text.
	 */
	CORE_API ETextDirection ComputeTextDirection(const FText& InText);
	CORE_API ETextDirection ComputeTextDirection(const FString& InString);
	CORE_API ETextDirection ComputeTextDirection(const TCHAR* InString, const int32 InStringStartIndex, const int32 InStringLen);

	/**
	 * Utility function which will compute the reading direction of the given text, as well as populate any advanced layout data for the text.
	 * The base direction is the overall reading direction of the text (see ComputeBaseDirection). This will affect where some characters (such as brackets and quotes) are placed within the resultant FTextDirectionInfo data.
	 * @return LeftToRight if all of the text is LTR, RightToLeft if all of the text is RTL, or Mixed if the text contains both LTR and RTL text.
	 */
	CORE_API ETextDirection ComputeTextDirection(const FText& InText, const ETextDirection InBaseDirection, TArray<FTextDirectionInfo>& OutTextDirectionInfo);
	CORE_API ETextDirection ComputeTextDirection(const FString& InString, const ETextDirection InBaseDirection, TArray<FTextDirectionInfo>& OutTextDirectionInfo);
	CORE_API ETextDirection ComputeTextDirection(const TCHAR* InString, const int32 InStringStartIndex, const int32 InStringLen, const ETextDirection InBaseDirection, TArray<FTextDirectionInfo>& OutTextDirectionInfo);

	/**
	 * Utility function which will compute the base direction of the given text.
	 * This provides the text flow direction that should be used when combining bidirectional text runs together.
	 * @return RightToLeft if the first character in the string has a bidirectional character type of R or AL, otherwise LeftToRight.
	 */
	CORE_API ETextDirection ComputeBaseDirection(const FText& InText);
	CORE_API ETextDirection ComputeBaseDirection(const FString& InString);
	CORE_API ETextDirection ComputeBaseDirection(const TCHAR* InString, const int32 InStringStartIndex, const int32 InStringLen);

	/**
	 * Utility function which tests to see whether the given character is a bidirectional control character.
	 */
	CORE_API bool IsControlCharacter(const TCHAR InChar);
} // namespace TextBiDi

Expose_TNameOf(FText)
