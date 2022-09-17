// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/UnrealString.h"
#include "HAL/CriticalSection.h"
#include "Containers/StringConv.h"
#include "Containers/StringFwd.h"
#include "UObject/UnrealNames.h"
#include "Templates/Atomic.h"
#include "Serialization/MemoryLayout.h"
#include "Misc/StringBuilder.h"

/*----------------------------------------------------------------------------
	Definitions.
----------------------------------------------------------------------------*/

/** 
 * Do we want to support case-variants for FName?
 * This will add an extra NAME_INDEX variable to FName, but means that ToString() will return you the exact same 
 * string that FName::Init was called with (which is useful if your FNames are shown to the end user)
 * Currently this is enabled for the Editor and any Programs (such as UHT), but not the Runtime
 */
#ifndef WITH_CASE_PRESERVING_NAME
	#define WITH_CASE_PRESERVING_NAME WITH_EDITORONLY_DATA
#endif

class FText;

/** Maximum size of name, including the null terminator. */
enum {NAME_SIZE	= 1024};

/** Opaque id to a deduplicated name */
struct FNameEntryId
{
	FNameEntryId() : Value(0) {}
	FNameEntryId(ENoInit) {}

	/** Slow alphabetical order that is stable / deterministic over process runs */
	CORE_API int32 CompareLexical(FNameEntryId Rhs) const;
	bool LexicalLess(FNameEntryId Rhs) const { return CompareLexical(Rhs) < 0; }

	/** Fast non-alphabetical order that is only stable during this process' lifetime */
	int32 CompareFast(FNameEntryId Rhs) const { return Value - Rhs.Value; };
	bool FastLess(FNameEntryId Rhs) const { return CompareFast(Rhs) < 0; }

	/** Fast non-alphabetical order that is only stable during this process' lifetime */
	bool operator<(FNameEntryId Rhs) const { return Value < Rhs.Value; }

	/** Fast non-alphabetical order that is only stable during this process' lifetime */
	bool operator>(FNameEntryId Rhs) const { return Rhs.Value < Value; }
	bool operator==(FNameEntryId Rhs) const { return Value == Rhs.Value; }
	bool operator!=(FNameEntryId Rhs) const { return Value != Rhs.Value; }

	explicit operator bool() const { return Value != 0; }

	UE_DEPRECATED(4.23, "NAME_INDEX is replaced by FNameEntryId, which is no longer a contiguous integer. "
						"Please use 'GetTypeHash(MyId)' instead of 'MyId' for hash functions. "
						"ToUnstableInt() can be used in other advanced cases.")
	operator int32() const;

	/** Get process specific integer */
	uint32 ToUnstableInt() const { return Value; }

	/** Create from unstable int produced by this process */
	CORE_API static FNameEntryId FromUnstableInt(uint32 UnstableInt);

	FORCEINLINE static FNameEntryId FromEName(EName Ename)
	{
		return Ename == NAME_None ? FNameEntryId() : FromValidEName(Ename);
	}

private:
	uint32 Value;

	CORE_API static FNameEntryId FromValidEName(EName Ename);
};

CORE_API uint32 GetTypeHash(FNameEntryId Id);
CORE_API bool operator==(FNameEntryId Id, EName Ename);
inline bool operator==(EName Ename, FNameEntryId Id) { return Id == Ename; }
inline bool operator!=(EName Ename, FNameEntryId Id) { return !(Id == Ename); }
inline bool operator!=(FNameEntryId Id, EName Ename) { return !(Id == Ename); }

/** Serialize as process specific unstable int */
CORE_API FArchive& operator<<(FArchive& Ar, FNameEntryId& InId);

/**
 * Legacy typedef - this is no longer an index
 *
 * Use GetTypeHash(FName) or GetTypeHash(FNameEntryId) for hashing
 * To compare with ENames use FName(EName) or FName::ToEName() instead
 */
typedef FNameEntryId NAME_INDEX;

#define checkName checkSlow

/** Externally, the instance number to represent no instance number is NAME_NO_NUMBER, 
    but internally, we add 1 to indices, so we use this #define internally for 
	zero'd memory initialization will still make NAME_None as expected */
#define NAME_NO_NUMBER_INTERNAL	0

/** Conversion routines between external representations and internal */
#define NAME_INTERNAL_TO_EXTERNAL(x) (x - 1)
#define NAME_EXTERNAL_TO_INTERNAL(x) (x + 1)

/** Special value for an FName with no number */
#define NAME_NO_NUMBER NAME_INTERNAL_TO_EXTERNAL(NAME_NO_NUMBER_INTERNAL)


/** this is the character used to separate a subobject root from its subobjects in a path name. */
#define SUBOBJECT_DELIMITER				TEXT(":")

/** this is the character used to separate a subobject root from its subobjects in a path name, as a char */
#define SUBOBJECT_DELIMITER_CHAR		':'

/** These are the characters that cannot be used in general FNames */
#define INVALID_NAME_CHARACTERS			TEXT("\"' ,\n\r\t")

/** These characters cannot be used in object names */
#define INVALID_OBJECTNAME_CHARACTERS	TEXT("\"' ,/.:|&!~\n\r\t@#(){}[]=;^%$`")

/** These characters cannot be used in ObjectPaths, which includes both the package path and part after the first . */
#define INVALID_OBJECTPATH_CHARACTERS	TEXT("\"' ,|&!~\n\r\t@#(){}[]=;^%$`")

/** These characters cannot be used in long package names */
#define INVALID_LONGPACKAGE_CHARACTERS	TEXT("\\:*?\"<>|' ,.&!~\n\r\t@#")

/** These characters can be used in relative directory names (lowercase versions as well) */
#define VALID_SAVEDDIRSUFFIX_CHARACTERS	TEXT("_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")

enum class ENameCase : uint8
{
	CaseSensitive,
	IgnoreCase,
};

enum ELinkerNameTableConstructor    {ENAME_LinkerConstructor};

/** Enumeration for finding name. */
enum EFindName
{
	/** Find a name; return 0 if it doesn't exist. */
	FNAME_Find,

	/** Find a name or add it if it doesn't exist. */
	FNAME_Add,

	/** Finds a name and replaces it. Adds it if missing. This is only used by UHT and is generally not safe for threading. 
	 * All this really is used for is correcting the case of names. In MT conditions you might get a half-changed name.
	 */
	FNAME_Replace_Not_Safe_For_Threading,
};

/*----------------------------------------------------------------------------
	FNameEntry.
----------------------------------------------------------------------------*/

/** Implementation detail exposed for debug visualizers */
struct FNameEntryHeader
{
	uint16 bIsWide : 1;
#if WITH_CASE_PRESERVING_NAME
	uint16 Len : 15;
#else
	static constexpr uint32 ProbeHashBits = 5;
	uint16 LowercaseProbeHash : ProbeHashBits;
	uint16 Len : 10;
#endif
};

/**
 * A global deduplicated name stored in the global name table.
 */
struct FNameEntry
{
private:
#if WITH_CASE_PRESERVING_NAME
	FNameEntryId ComparisonId;
#endif
	FNameEntryHeader Header;
	union
	{
		ANSICHAR	AnsiName[NAME_SIZE];
		WIDECHAR	WideName[NAME_SIZE];
	};

	FNameEntry(const FNameEntry&) = delete;
	FNameEntry(FNameEntry&&) = delete;
	FNameEntry& operator=(const FNameEntry&) = delete;
	FNameEntry& operator=(FNameEntry&&) = delete;

public:
	/** Returns whether this name entry is represented via WIDECHAR or ANSICHAR. */
	FORCEINLINE bool IsWide() const
	{
		return Header.bIsWide;
	}

	FORCEINLINE int32 GetNameLength() const
	{
		return Header.Len;
	}

	/**
	 * Copy unterminated name to TCHAR buffer without allocating.
	 *
	 * @param OutSize must be at least GetNameLength()
	 */
	void GetUnterminatedName(TCHAR* OutName, uint32 OutSize) const;

	/** Copy null-terminated name to TCHAR buffer without allocating. */
	void GetName(TCHAR(&OutName)[NAME_SIZE]) const;

	/** Copy null-terminated name to ANSICHAR buffer without allocating. Entry must not be wide. */
	CORE_API void GetAnsiName(ANSICHAR(&OutName)[NAME_SIZE]) const;

	/** Copy null-terminated name to WIDECHAR buffer without allocating. Entry must be wide. */
	CORE_API void GetWideName(WIDECHAR(&OutName)[NAME_SIZE]) const;

	/** Copy name to a dynamically allocated FString. */
	CORE_API FString GetPlainNameString() const;

	/** Appends name to string. May allocate. */
	CORE_API void AppendNameToString(FString& OutString) const;

	/** Appends name to string builder. */
	CORE_API void AppendNameToString(FStringBuilderBase& OutString) const;

	/** Appends name to string builder. Entry must not be wide. */
	CORE_API void AppendAnsiNameToString(FAnsiStringBuilderBase& OutString) const;

	/** Appends name to string with path separator using FString::PathAppend(). */
	CORE_API void AppendNameToPathString(FString& OutString) const;


	/**
	 * Returns the size in bytes for FNameEntry structure. This is != sizeof(FNameEntry) as we only allocated as needed.
	 *
	 * @param	Length			Length of name
	 * @param	bIsPureAnsi		Whether name is pure ANSI or not
	 * @return	required size of FNameEntry structure to hold this string (might be wide or ansi)
	 */
	static int32 GetSize( int32 Length, bool bIsPureAnsi );
	static CORE_API int32 GetSize(const TCHAR* Name);

	CORE_API int32 GetSizeInBytes() const;

	CORE_API void Write(FArchive& Ar) const;

	static CORE_API int32 GetDataOffset();
	struct CORE_API FNameStringView MakeView(union FNameBuffer& OptionalDecodeBuffer) const;
private:
	friend class FName;
	friend struct FNameHelper;
	friend class FNameEntryAllocator;
	friend class FNamePoolShardBase;

	static void Encode(ANSICHAR* Name, uint32 Len);
	static void Encode(WIDECHAR* Name, uint32 Len);
	static void Decode(ANSICHAR* Name, uint32 Len);
	static void Decode(WIDECHAR* Name, uint32 Len);

	void StoreName(const ANSICHAR* InName, uint32 Len);
	void StoreName(const WIDECHAR* InName, uint32 Len);
	void CopyUnterminatedName(ANSICHAR* OutName) const;
	void CopyUnterminatedName(WIDECHAR* OutName) const;
	void CopyAndConvertUnterminatedName(TCHAR* OutName) const;
	const ANSICHAR* GetUnterminatedName(ANSICHAR(&OptionalDecodeBuffer)[NAME_SIZE]) const;
	const WIDECHAR* GetUnterminatedName(WIDECHAR(&OptionalDecodeBuffer)[NAME_SIZE]) const;
};

/**
 *  This struct is only used during loading/saving and is not part of the runtime costs
 */
struct FNameEntrySerialized
{
	FNameEntryId Index;
	bool bIsWide = false;

	union
	{
		ANSICHAR	AnsiName[NAME_SIZE];
		WIDECHAR	WideName[NAME_SIZE];
	};

	// These are not used anymore but recalculated on save to maintain serialization format
	uint16 NonCasePreservingHash = 0;
	uint16 CasePreservingHash = 0;

	FNameEntrySerialized(const FNameEntry& NameEntry);
	FNameEntrySerialized(enum ELinkerNameTableConstructor) {}

	/**
	 * Returns direct access to null-terminated name if narrow
	 */
	ANSICHAR const* GetAnsiName() const
	{
		check(!bIsWide);
		return AnsiName;
	}

	/**
	 * Returns direct access to null-terminated name if wide
	 */
	WIDECHAR const* GetWideName() const
	{
		check(bIsWide);
		return WideName;
	}

	/**
	 * Returns FString of name portion minus number.
	 */
	CORE_API FString GetPlainNameString() const;	

	friend CORE_API FArchive& operator<<(FArchive& Ar, FNameEntrySerialized& E);
	friend FArchive& operator<<(FArchive& Ar, FNameEntrySerialized* E)
	{
		return Ar << *E;
	}
};

/**
 * The minimum amount of data required to reconstruct a name
 * This is smaller than FName, but you lose the case-preserving behavior
 */
struct FMinimalName
{
	FMinimalName() {}
	
	FMinimalName(EName N)
		: Index(FNameEntryId::FromEName(N))
	{
	}

	FMinimalName(FNameEntryId InIndex, int32 InNumber)
		: Index(InIndex)
		, Number(InNumber)
	{
	}

	FORCEINLINE bool IsNone() const
	{
		return !Index && Number == NAME_NO_NUMBER_INTERNAL;
	}

	friend FORCEINLINE FArchive& operator<<(FArchive& Ar, FMinimalName& E)
	{
		Ar << E.Index;
		Ar << E.Number;
		return Ar;
	}

	FORCEINLINE bool operator<(FMinimalName Rhs) const { return Index == Rhs.Index ? Number < Rhs.Number : Index < Rhs.Index; };

	/** Index into the Names array (used to find String portion of the string/number pair) */
	FNameEntryId	Index;
	/** Number portion of the string/number pair (stored internally as 1 more than actual, so zero'd memory will be the default, no-instance case) */
	int32			Number = NAME_NO_NUMBER_INTERNAL;
};

/**
 * The full amount of data required to reconstruct a case-preserving name
 * This will be the same size as FName when WITH_CASE_PRESERVING_NAME is 1, and is used to store an FName in cases where 
 * the size of FName must be constant between build configurations (eg, blueprint bytecode)
 */
struct FScriptName
{
	FScriptName() {}
	
	FScriptName(EName Ename)
		: ComparisonIndex(FNameEntryId::FromEName(Ename))
		, DisplayIndex(ComparisonIndex)
	{
	}

	FScriptName(FNameEntryId InComparisonIndex, FNameEntryId InDisplayIndex, int32 InNumber)
		: ComparisonIndex(InComparisonIndex)
		, DisplayIndex(InDisplayIndex)
		, Number(InNumber)
	{
	}

	FORCEINLINE bool IsNone() const
	{
		return !ComparisonIndex && Number == NAME_NO_NUMBER_INTERNAL;
	}

	CORE_API FString ToString() const;

	/** Index into the Names array (used to find String portion of the string/number pair used for comparison) */
	FNameEntryId	ComparisonIndex;
	/** Index into the Names array (used to find String portion of the string/number pair used for display) */
	FNameEntryId	DisplayIndex;
	/** Number portion of the string/number pair (stored internally as 1 more than actual, so zero'd memory will be the default, no-instance case) */
	uint32			Number = NAME_NO_NUMBER_INTERNAL;
};

/**
 * Public name, available to the world.  Names are stored as a combination of
 * an index into a table of unique strings and an instance number.
 * Names are case-insensitive, but case-preserving (when WITH_CASE_PRESERVING_NAME is 1)
 */
class CORE_API FName
{
public:
	FORCEINLINE FNameEntryId GetComparisonIndex() const
	{
		checkName(IsWithinBounds(ComparisonIndex));
		return ComparisonIndex;
	}

	FORCEINLINE FNameEntryId GetDisplayIndex() const
	{
		const FNameEntryId Index = GetDisplayIndexFast();
		checkName(IsWithinBounds(Index));
		return Index;
	}

	FORCEINLINE int32 GetNumber() const
	{
		return Number;
	}

	FORCEINLINE void SetNumber(const int32 NewNumber)
	{
		Number = NewNumber;
	}
	
	/** Get name without number part as a dynamically allocated string */
	FString GetPlainNameString() const;

	/** Convert name without number part into TCHAR buffer and returns string length. Doesn't allocate. */
	uint32 GetPlainNameString(TCHAR(&OutName)[NAME_SIZE]) const;

	/** Copy ANSI name without number part. Must *only* be used for ANSI FNames. Doesn't allocate. */
	void GetPlainANSIString(ANSICHAR(&AnsiName)[NAME_SIZE]) const;

	/** Copy wide name without number part. Must *only* be used for wide FNames. Doesn't allocate. */
	void GetPlainWIDEString(WIDECHAR(&WideName)[NAME_SIZE]) const;

	const FNameEntry* GetComparisonNameEntry() const;
	const FNameEntry* GetDisplayNameEntry() const;

	/**
	 * Converts an FName to a readable format
	 *
	 * @return String representation of the name
	 */
	FString ToString() const;

	/**
	 * Converts an FName to a readable format, in place
	 * 
	 * @param Out String to fill with the string representation of the name
	 */
	void ToString(FString& Out) const;

	/**
	 * Converts an FName to a readable format, in place
	 * 
	 * @param Out StringBuilder to fill with the string representation of the name
	 */
	void ToString(FStringBuilderBase& Out) const;

	/**
	 * Get the number of characters, excluding null-terminator, that ToString() would yield
	 */
	uint32 GetStringLength() const;

	/**
	 * Buffer size required for any null-terminated FName string, i.e. [name] '_' [digits] '\0'
	 */
	static constexpr uint32 StringBufferSize = NAME_SIZE + 1 + 10; // NAME_SIZE includes null-terminator

	/**
	 * Convert to string buffer to avoid dynamic allocations and returns string length
	 *
	 * Fails hard if OutLen < GetStringLength() + 1. StringBufferSize guarantees success.
	 *
	 * Note that a default constructed FName returns "None" instead of ""
	 */
	uint32 ToString(TCHAR* Out, uint32 OutSize) const;

	template<int N>
	uint32 ToString(TCHAR (&Out)[N]) const
	{
		return ToString(Out, N);
	}

	/**
	 * Converts an FName to a readable format, in place, appending to an existing string (ala GetFullName)
	 * 
	 * @param Out String to append with the string representation of the name
	 */
	void AppendString(FString& Out) const;

	/**
	 * Converts an FName to a readable format, in place, appending to an existing string (ala GetFullName)
	 * 
	 * @param Out StringBuilder to append with the string representation of the name
	 */
	void AppendString(FStringBuilderBase& Out) const;

	/**
	 * Converts an ANSI FName to a readable format appended to the string builder.
	 *
	 * @param Out A string builder to write the readable representation of the name into.
	 *
	 * @return Whether the string is ANSI. A return of false indicates that the string was wide and was not written.
	 */
	bool TryAppendAnsiString(FAnsiStringBuilderBase& Out) const;

	/**
	 * Check to see if this FName matches the other FName, potentially also checking for any case variations
	 */
	FORCEINLINE bool IsEqual(const FName& Other, const ENameCase CompareMethod = ENameCase::IgnoreCase, const bool bCompareNumber = true ) const
	{
		return ((CompareMethod == ENameCase::IgnoreCase) ? ComparisonIndex == Other.ComparisonIndex : GetDisplayIndexFast() == Other.GetDisplayIndexFast())
			&& (!bCompareNumber || GetNumber() == Other.GetNumber());
	}

	FORCEINLINE bool operator==(FName Other) const
	{
#if PLATFORM_64BITS && !WITH_CASE_PRESERVING_NAME
		return ToComparableInt() == Other.ToComparableInt();
#else
		return (ComparisonIndex == Other.ComparisonIndex) & (GetNumber() == Other.GetNumber());
#endif
	}

	FORCEINLINE bool operator!=(FName Other) const
	{
		return !(*this == Other);
	}

	FORCEINLINE bool operator==(EName Ename) const
	{
		return (ComparisonIndex == Ename) & (GetNumber() == 0);
	}
	
	FORCEINLINE bool operator!=(EName Ename) const
	{
		return (ComparisonIndex != Ename) | (GetNumber() != 0);
	}

	UE_DEPRECATED(4.23, "Please use FastLess() / FNameFastLess or LexicalLess() / FNameLexicalLess instead. "
		"Default lexical sort order is deprecated to avoid unintended expensive sorting. ")
	FORCEINLINE bool operator<( const FName& Other ) const
	{
		return LexicalLess(Other);
	}

	UE_DEPRECATED(4.23, "Please use B.FastLess(A) or B.LexicalLess(A) instead of A > B.")
	FORCEINLINE bool operator>(const FName& Other) const
	{
		return Other.LexicalLess(*this);
	}

	/** Fast non-alphabetical order that is only stable during this process' lifetime. */
	FORCEINLINE bool FastLess(const FName& Other) const
	{
		return CompareIndexes(Other) < 0;
	}

	/** Slow alphabetical order that is stable / deterministic over process runs. */
	FORCEINLINE bool LexicalLess(const FName& Other) const
	{
		return Compare(Other) < 0;
	}

	/** True for FName(), FName(NAME_None) and FName("None") */
	FORCEINLINE bool IsNone() const
	{
#if PLATFORM_64BITS && !WITH_CASE_PRESERVING_NAME
		return ToComparableInt() == 0;
#else
		return !ComparisonIndex && GetNumber() == NAME_NO_NUMBER_INTERNAL;
#endif
	}

	/**
	 * Paranoid sanity check
	 *
	 * All FNames are valid except for stomped memory, dangling pointers, etc.
	 * Should only be used to investigate such bugs and not in production code.
	 */
	bool IsValid() const { return IsWithinBounds(ComparisonIndex); }

	/** Paranoid sanity check, same as IsValid() */
	bool IsValidIndexFast() const { return IsValid(); }


	/**
	 * Checks to see that a given name-like string follows the rules that Unreal requires.
	 *
	 * @param	InName			String containing the name to test.
	 * @param	InInvalidChars	The set of invalid characters that the name cannot contain.
	 * @param	OutReason		If the check fails, this string is filled in with the reason why.
	 * @param	InErrorCtx		Error context information to show in the error message (default is "Name").
	 *
	 * @return	true if the name is valid
	 */
	static bool IsValidXName( const FName InName, const FString& InInvalidChars, FText* OutReason = nullptr, const FText* InErrorCtx = nullptr );
	static bool IsValidXName( const TCHAR* InName, const FString& InInvalidChars, FText* OutReason = nullptr, const FText* InErrorCtx = nullptr );
	static bool IsValidXName( const FString& InName, const FString& InInvalidChars, FText* OutReason = nullptr, const FText* InErrorCtx = nullptr );
	static bool IsValidXName( const FStringView& InName, const FString& InInvalidChars, FText* OutReason = nullptr, const FText* InErrorCtx = nullptr );

	/**
	 * Checks to see that a FName follows the rules that Unreal requires.
	 *
	 * @param	InInvalidChars	The set of invalid characters that the name cannot contain
	 * @param	OutReason		If the check fails, this string is filled in with the reason why.
	 * @param	InErrorCtx		Error context information to show in the error message (default is "Name").
	 *
	 * @return	true if the name is valid
	 */
	bool IsValidXName( const FString& InInvalidChars = INVALID_NAME_CHARACTERS, FText* OutReason = nullptr, const FText* InErrorCtx = nullptr ) const
	{
		return IsValidXName(*this, InInvalidChars, OutReason, InErrorCtx);
	}

	/**
	 * Takes an FName and checks to see that it follows the rules that Unreal requires.
	 *
	 * @param	OutReason		If the check fails, this string is filled in with the reason why.
	 * @param	InInvalidChars	The set of invalid characters that the name cannot contain
	 *
	 * @return	true if the name is valid
	 */
	bool IsValidXName( FText& OutReason, const FString& InInvalidChars = INVALID_NAME_CHARACTERS ) const
	{
		return IsValidXName(*this, InInvalidChars, &OutReason);
	}

	/**
	 * Takes an FName and checks to see that it follows the rules that Unreal requires for object names.
	 *
	 * @param	OutReason		If the check fails, this string is filled in with the reason why.
	 *
	 * @return	true if the name is valid
	 */
	bool IsValidObjectName( FText& OutReason ) const
	{
		return IsValidXName(*this, INVALID_OBJECTNAME_CHARACTERS, &OutReason);
	}

	/**
	 * Takes an FName and checks to see that it follows the rules that Unreal requires for package or group names.
	 *
	 * @param	OutReason		If the check fails, this string is filled in with the reason why.
	 * @param	bIsGroupName	if true, check legality for a group name, else check legality for a package name
	 *
	 * @return	true if the name is valid
	 */
	bool IsValidGroupName( FText& OutReason, bool bIsGroupName=false ) const
	{
		return IsValidXName(*this, INVALID_LONGPACKAGE_CHARACTERS, &OutReason);
	}

	/**
	 * Compares name to passed in one. Sort is alphabetical ascending.
	 *
	 * @param	Other	Name to compare this against
	 * @return	< 0 is this < Other, 0 if this == Other, > 0 if this > Other
	 */
	int32 Compare( const FName& Other ) const;

	/**
	 * Fast compares name to passed in one using indexes. Sort is allocation order ascending.
	 *
	 * @param	Other	Name to compare this against
	 * @return	< 0 is this < Other, 0 if this == Other, > 0 if this > Other
	 */
	FORCEINLINE int32 CompareIndexes(const FName& Other) const
	{
		if (int32 ComparisonDiff = ComparisonIndex.CompareFast(Other.ComparisonIndex))
		{
			return ComparisonDiff;
		}

			return GetNumber() - Other.GetNumber();
		}

	/**
	 * Create an FName with a hardcoded string index.
	 *
	 * @param N The hardcoded value the string portion of the name will have. The number portion will be NAME_NO_NUMBER
	 */
	FORCEINLINE FName(EName Ename) : FName(Ename, NAME_NO_NUMBER_INTERNAL) {}

	/**
	 * Create an FName with a hardcoded string index and (instance).
	 *
	 * @param N The hardcoded value the string portion of the name will have
	 * @param InNumber The hardcoded value for the number portion of the name
	 */
	FORCEINLINE FName(EName Ename, int32 InNumber)
		: ComparisonIndex(FNameEntryId::FromEName(Ename))
#if WITH_CASE_PRESERVING_NAME
		, DisplayIndex(ComparisonIndex)
#endif
		, Number(InNumber)
	{
	}


	/**
	 * Create an FName from an existing string, but with a different instance.
	 *
	 * @param Other The FName to take the string values from
	 * @param InNumber The hardcoded value for the number portion of the name
	 */
	FORCEINLINE FName( const FName& Other, int32 InNumber )
		: ComparisonIndex( Other.ComparisonIndex )
#if WITH_CASE_PRESERVING_NAME
		, DisplayIndex( Other.DisplayIndex )
#endif
		, Number( InNumber )
	{
	}

	/**
	 * Create an FName from its component parts
	 * Only call this if you *really* know what you're doing
	 */
	FORCEINLINE FName( const FNameEntryId InComparisonIndex, const FNameEntryId InDisplayIndex, const int32 InNumber )
		: ComparisonIndex( InComparisonIndex )
#if WITH_CASE_PRESERVING_NAME
		, DisplayIndex( InDisplayIndex )
#endif
		, Number( InNumber )
	{
	}

#if WITH_CASE_PRESERVING_NAME
	static FNameEntryId GetComparisonIdFromDisplayId(FNameEntryId DisplayId);
#else
	static FNameEntryId GetComparisonIdFromDisplayId(FNameEntryId DisplayId) { return DisplayId; }
#endif

	/**
	 * Only call this if you *really* know what you're doing
	 */
	static FName CreateFromDisplayId(FNameEntryId DisplayId, int32 Number)
	{
		return FName(GetComparisonIdFromDisplayId(DisplayId), DisplayId, Number);
	}

	/**
	 * Default constructor, initialized to None
	 */
	FORCEINLINE FName()
		: Number(NAME_NO_NUMBER_INTERNAL)
	{
	}

	/**
	 * Scary no init constructor, used for something obscure in UObjectBase
	 */
	explicit FName(ENoInit)
		: ComparisonIndex(NoInit)
#if WITH_CASE_PRESERVING_NAME
		, DisplayIndex(NoInit)
#endif
	{}

	/**
	 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
	 * doesn't already exist, then the name will be NAME_None
	 *
	 * @param Name			Value for the string portion of the name
	 * @param FindType		Action to take (see EFindName)
	 */
	FName(const WIDECHAR* Name, EFindName FindType=FNAME_Add);
	FName(const ANSICHAR* Name, EFindName FindType=FNAME_Add);
	FName(const UTF8CHAR* Name, EFindName FindType=FNAME_Add);

	/** Create FName from non-null string with known length  */
	FName(int32 Len, const WIDECHAR* Name, EFindName FindType=FNAME_Add);
	FName(int32 Len, const ANSICHAR* Name, EFindName FindType=FNAME_Add);
	FName(int32 Len, const UTF8CHAR* Name, EFindName FindType=FNAME_Add);

	template <typename CharRangeType,
		typename CharType = typename TRemoveCV<typename TRemovePointer<decltype(GetData(DeclVal<CharRangeType>()))>::Type>::Type,
		typename = decltype(ImplicitConv<TStringView<CharType>>(DeclVal<CharRangeType>()))>
	inline explicit FName(CharRangeType&& Name, EFindName FindType = FNAME_Add)
		: FName(NoInit)
	{
		TStringView<CharType> View = Forward<CharRangeType>(Name);
		*this = FName(View.Len(), View.GetData(), FindType);
	}

	/**
	 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
	 * doesn't already exist, then the name will be NAME_None
	 *
	 * @param Name Value for the string portion of the name
	 * @param Number Value for the number portion of the name
	 * @param FindType Action to take (see EFindName)
	 * @param bSplitName true if the trailing number should be split from the name when Number == NAME_NO_NUMBER_INTERNAL, or false to always use the name as-is
	 */
	FName(const WIDECHAR* Name, int32 InNumber, EFindName FindType = FNAME_Add);
	FName(const ANSICHAR* Name, int32 InNumber, EFindName FindType = FNAME_Add);
	FName(const UTF8CHAR* Name, int32 InNumber, EFindName FindType = FNAME_Add);
	FName(int32 Len, const WIDECHAR* Name, int32 Number, EFindName FindType = FNAME_Add);
	FName(int32 Len, const ANSICHAR* Name, int32 InNumber, EFindName FindType = FNAME_Add);
	FName(int32 Len, const UTF8CHAR* Name, int32 InNumber, EFindName FindType = FNAME_Add);

	template <typename CharRangeType,
		typename CharType = typename TRemoveCV<typename TRemovePointer<decltype(GetData(DeclVal<CharRangeType>()))>::Type>::Type,
		typename = decltype(ImplicitConv<TStringView<CharType>>(DeclVal<CharRangeType>()))>
	inline FName(CharRangeType&& Name, int32 InNumber, EFindName FindType = FNAME_Add)
		: FName(NoInit)
	{
		TStringView<CharType> View = Forward<CharRangeType>(Name);
		*this = FName(View.Len(), View.GetData(), InNumber, FindType);
	}

	/**
	 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
	 * doesn't already exist, then the name will be NAME_None
	 *
	 * @param Name Value for the string portion of the name
	 * @param Number Value for the number portion of the name
	 * @param FindType Action to take (see EFindName)
	 * @param bSplitName true if the trailing number should be split from the name when Number == NAME_NO_NUMBER_INTERNAL, or false to always use the name as-is
	 */
	FName( const TCHAR* Name, int32 InNumber, EFindName FindType, bool bSplitName);

	/**
	 * Constructor used by FLinkerLoad when loading its name table; Creates an FName with an instance
	 * number of 0 that does not attempt to split the FName into string and number portions. Also,
	 * this version skips calculating the hashes of the names if possible
	 */
	FName(const FNameEntrySerialized& LoadedEntry);

	/**
	 * Equality operator.
	 *
	 * @param	Other	String to compare this name to
	 * @return true if name matches the string, false otherwise
	 */
	bool operator==(const ANSICHAR* Other) const;
	bool operator==(const WIDECHAR* Other) const;

	/**
	 * Inequality operator.
	 *
	 * @param	Other	String to compare this name to
	 * @return true if name does not match the string, false otherwise
	 */
	template <typename CharType>
	bool operator!=(const CharType* Other) const
	{
		return !operator==(Other);
	}

	static void DisplayHash( class FOutputDevice& Ar );
	static FString SafeString(FNameEntryId InDisplayIndex, int32 InstanceNumber = NAME_NO_NUMBER_INTERNAL);

	/**
	 * @return Size of all name entries.
	 */
	static int32 GetNameEntryMemorySize();

	/**
	* @return Size of Name Table object as a whole
	*/
	static int32 GetNameTableMemorySize();

	/**
	 * @return number of ansi names in name table
	 */
	static int32 GetNumAnsiNames();

	/**
	 * @return number of wide names in name table
	 */
	static int32 GetNumWideNames();

	static TArray<const FNameEntry*> DebugDump();

	static FNameEntry const* GetEntry(EName Ename);
	static FNameEntry const* GetEntry(FNameEntryId Id);

	//@}

	/** Run autotest on FNames. */
	static void AutoTest();
	
	/**
	 * Takes a string and breaks it down into a human readable string.
	 * For example - "bCreateSomeStuff" becomes "Create Some Stuff?" and "DrawScale3D" becomes "Draw Scale 3D".
	 * 
	 * @param	InDisplayName	[In, Out] The name to sanitize
	 * @param	bIsBool				True if the name is a bool
	 *
	 * @return	the sanitized version of the display name
	 */
	static FString NameToDisplayString( const FString& InDisplayName, const bool bIsBool );

	/** Get the EName that this FName represents or nullptr */
	const EName* ToEName() const;

	/** 
		Tear down system and free all allocated memory 
	
		FName must not be used after teardown
	 */
	static void TearDown();

private:

	/** Index into the Names array (used to find String portion of the string/number pair used for comparison) */
	FNameEntryId	ComparisonIndex;
#if WITH_CASE_PRESERVING_NAME
	/** Index into the Names array (used to find String portion of the string/number pair used for display) */
	FNameEntryId	DisplayIndex;
#endif // WITH_CASE_PRESERVING_NAME
	/** Number portion of the string/number pair (stored internally as 1 more than actual, so zero'd memory will be the default, no-instance case) */
	uint32			Number;

#if PLATFORM_64BITS && !WITH_CASE_PRESERVING_NAME
	FORCEINLINE uint64 ToComparableInt() const
	{
		static_assert(sizeof(*this) == sizeof(uint64), "");
		alignas(uint64) FName AlignedCopy = *this;
		return reinterpret_cast<uint64&>(AlignedCopy);
	}
#endif

	friend const TCHAR* DebugFName(int32);
	friend const TCHAR* DebugFName(int32, int32);
	friend const TCHAR* DebugFName(FName&);

	FORCEINLINE FNameEntryId GetDisplayIndexFast() const
	{
#if WITH_CASE_PRESERVING_NAME
		return DisplayIndex;
#else
		return ComparisonIndex;
#endif
	}





	static bool IsWithinBounds(FNameEntryId Id);
};

template<> struct TIsZeroConstructType<class FName> { enum { Value = true }; };
Expose_TNameOf(FName)

namespace Freeze
{
	CORE_API void IntrinsicWriteMemoryImage(FMemoryImageWriter& Writer, const FName& Object, const FTypeLayoutDesc&);
	CORE_API uint32 IntrinsicAppendHash(const FName* DummyObject, const FTypeLayoutDesc& TypeDesc, const FPlatformTypeLayoutParameters& LayoutParams, FSHA1& Hasher);
	CORE_API void IntrinsicWriteMemoryImage(FMemoryImageWriter& Writer, const FMinimalName& Object, const FTypeLayoutDesc&);
	CORE_API void IntrinsicWriteMemoryImage(FMemoryImageWriter& Writer, const FScriptName& Object, const FTypeLayoutDesc&);
	CORE_API uint32 IntrinsicUnfrozenCopy(const FMemoryUnfreezeContent& Context, const FName& Object, void* OutDst);
}

DECLARE_INTRINSIC_TYPE_LAYOUT(FName);
DECLARE_INTRINSIC_TYPE_LAYOUT(FMinimalName);
DECLARE_INTRINSIC_TYPE_LAYOUT(FScriptName);

FORCEINLINE uint32 GetTypeHash(FName Name)
{
	return GetTypeHash(Name.GetComparisonIndex()) + Name.GetNumber();
}

FORCEINLINE uint32 GetTypeHash(FMinimalName Name)
{
	return GetTypeHash(Name.Index) + Name.Number;
}

FORCEINLINE uint32 GetTypeHash(FScriptName Name)
{
	return GetTypeHash(Name.ComparisonIndex) + Name.Number;
}

FORCEINLINE FString LexToString(const FName& Name)
{
	return Name.ToString();
}

FORCEINLINE void LexFromString(FName& Name, const TCHAR* Str)
{
	Name = FName(Str);
}

FORCEINLINE FMinimalName NameToMinimalName(const FName& InName)
{
	return FMinimalName(InName.GetComparisonIndex(), InName.GetNumber());
}

FORCEINLINE FName MinimalNameToName(const FMinimalName& InName)
{
	return FName(InName.Index, InName.Index, InName.Number);
}

FORCEINLINE FScriptName NameToScriptName(const FName& InName)
{
	return FScriptName(InName.GetComparisonIndex(), InName.GetDisplayIndex(), InName.GetNumber());
}

FORCEINLINE FName ScriptNameToName(const FScriptName& InName)
{
	return FName(InName.ComparisonIndex, InName.DisplayIndex, InName.Number);
}

inline FStringBuilderBase& operator<<(FStringBuilderBase& Builder, const FName& Name)
{
	Name.AppendString(Builder);
	return Builder;
}

CORE_API FStringBuilderBase& operator<<(FStringBuilderBase& Builder, FNameEntryId Id);

/**
 * Equality operator with CharType* on left hand side and FName on right hand side
 * 
 * @param	LHS		CharType to compare to FName
 * @param	RHS		FName to compare to CharType
 * @return True if strings match, false otherwise.
 */
template <typename CharType>
inline bool operator==(const CharType *LHS, const FName &RHS)
{
	return RHS == LHS;
}

/**
 * Inequality operator with CharType* on left hand side and FName on right hand side
 *
 * @param	LHS		CharType to compare to FName
 * @param	RHS		FName to compare to CharType
 * @return True if strings don't match, false otherwise.
 */
template <typename CharType>
inline bool operator!=(const CharType *LHS, const FName &RHS)
{
	return RHS != LHS;
}

/** FNames act like PODs. */
template <> struct TIsPODType<FName> { enum { Value = true }; };

/** Fast non-alphabetical order that is only stable during this process' lifetime */
struct FNameFastLess
{
	FORCEINLINE bool operator()(const FName& A, const FName& B) const
	{
		return A.CompareIndexes(B) < 0;
	}

	FORCEINLINE bool operator()(FNameEntryId A, FNameEntryId B) const
	{
		return A.FastLess(B);
	}
};

UE_DEPRECATED(4.23, "Please use FNameFastLess instead.")
typedef FNameFastLess FNameSortIndexes;

/** Slow alphabetical order that is stable / deterministic over process runs */
struct FNameLexicalLess
{
	FORCEINLINE bool operator()(const FName& A, const FName& B) const
	{
		return A.Compare(B) < 0;
	}

	FORCEINLINE bool operator()(FNameEntryId A, FNameEntryId B) const
	{
		return A.LexicalLess(B);
	}
};

FORCEINLINE bool operator==(const FMinimalName& Lhs, const FMinimalName& Rhs)
{
	return Lhs.Number == Rhs.Number && Lhs.Index == Rhs.Index;
}

FORCEINLINE bool operator!=(const FMinimalName& Lhs, const FMinimalName& Rhs)
{
	return !operator==(Lhs, Rhs);
}

FORCEINLINE bool operator==(const FScriptName& Lhs, const FScriptName& Rhs)
{
	return Lhs.Number == Rhs.Number && Lhs.ComparisonIndex == Rhs.ComparisonIndex;
}

FORCEINLINE bool operator!=(const FScriptName& Lhs, const FScriptName& Rhs)
{
	return !operator==(Lhs, Rhs);
}

FORCEINLINE bool operator==(const FName& Lhs, const FMinimalName& Rhs)
{
	return Lhs.GetNumber() == Rhs.Number && Lhs.GetComparisonIndex() == Rhs.Index;
}

FORCEINLINE bool operator!=(const FName& Lhs, const FMinimalName& Rhs)
{
	return !operator==(Lhs, Rhs);
}

FORCEINLINE bool operator==(const FMinimalName& Lhs, const FName& Rhs)
{
	return Lhs.Number == Rhs.GetNumber() && Lhs.Index == Rhs.GetComparisonIndex();
}

FORCEINLINE bool operator!=(const FMinimalName& Lhs, const FName& Rhs)
{
	return !operator==(Lhs, Rhs);
}

FORCEINLINE bool operator==(const FName& Lhs, const FScriptName& Rhs)
{
	return Lhs.GetNumber() == Rhs.Number && Lhs.GetComparisonIndex() == Rhs.ComparisonIndex;
}

FORCEINLINE bool operator!=(const FName& Lhs, const FScriptName& Rhs)
{
	return !operator==(Lhs, Rhs);
}

FORCEINLINE bool operator==(const FScriptName& Lhs, const FName& Rhs)
{
	return Lhs.Number == Rhs.GetNumber() && Lhs.ComparisonIndex == Rhs.GetComparisonIndex();
}

FORCEINLINE bool operator!=(const FScriptName& Lhs, const FName& Rhs)
{
	return !operator==(Lhs, Rhs);
}

#ifndef WITH_CUSTOM_NAME_ENCODING
inline void FNameEntry::Encode(ANSICHAR*, uint32) {}
inline void FNameEntry::Encode(WIDECHAR*, uint32) {}
inline void FNameEntry::Decode(ANSICHAR*, uint32) {}
inline void FNameEntry::Decode(WIDECHAR*, uint32) {}
#endif

struct FNameDebugVisualizer
{
	CORE_API static uint8** GetBlocks();
private:
	static constexpr uint32 EntryStride = alignof(FNameEntry);
	static constexpr uint32 OffsetBits = 16;
	static constexpr uint32 BlockBits = 13;
	static constexpr uint32 OffsetMask = (1 << OffsetBits) - 1;
	static constexpr uint32 UnusedMask = UINT32_MAX << BlockBits << OffsetBits;
	static constexpr uint32 MaxLength = NAME_SIZE;
};

/** Lazily constructed FName that helps avoid allocating FNames during static initialization */
class FLazyName
{
public:
	FLazyName()
		: Either(FNameEntryId())
	{}

	/** @param Literal must be a string literal */
	template<int N>
	FLazyName(const WIDECHAR(&Literal)[N])
		: Either(Literal)
		, Number(ParseNumber(Literal, N - 1))
		, bLiteralIsWide(true)
	{}

	/** @param Literal must be a string literal */
	template<int N>
	FLazyName(const ANSICHAR(&Literal)[N])
		: Either(Literal)
		, Number(ParseNumber(Literal, N - 1))
		, bLiteralIsWide(false)
	{}

	explicit FLazyName(FName Name)
		: Either(Name.GetComparisonIndex())
		, Number(Name.GetNumber())
	{}
	
	operator FName() const
	{
		return Resolve();
	}

	CORE_API FName Resolve() const;

private:
	struct FLiteralOrName
	{
		// NOTE: uses high bit of pointer for flag; this may be an issue in future when high byte of address may be used for features like hardware ASAN
		static constexpr uint64 LiteralFlag = uint64(1) << (sizeof(uint64) * 8 - 1);

		explicit FLiteralOrName(const ANSICHAR* Literal)
			: Int(reinterpret_cast<uint64>(Literal) | LiteralFlag)
		{}
		
		explicit FLiteralOrName(const WIDECHAR* Literal)
			: Int(reinterpret_cast<uint64>(Literal) | LiteralFlag)
		{}

		explicit FLiteralOrName(FNameEntryId Name)
			: Int(Name.ToUnstableInt())
		{}

		bool IsName() const
		{
			return (LiteralFlag & Int) == 0;
		}

		bool IsLiteral() const
		{
			return (LiteralFlag & Int) != 0;
		}

		FNameEntryId AsName() const
		{
			return FNameEntryId::FromUnstableInt(static_cast<uint32>(Int));
		}
		
		const ANSICHAR* AsAnsiLiteral() const
		{
			return reinterpret_cast<const ANSICHAR*>(Int & ~LiteralFlag);
		}

		const WIDECHAR* AsWideLiteral() const
		{
			return reinterpret_cast<const WIDECHAR*>(Int & ~LiteralFlag);
		}

		uint64 Int;
	};

	mutable FLiteralOrName Either;
	mutable uint32 Number = 0;

	// Distinguishes WIDECHAR* and ANSICHAR* literals, doesn't indicate if literal contains any wide characters 
	bool bLiteralIsWide = false;
	
	CORE_API static uint32 ParseNumber(const WIDECHAR* Literal, int32 Len);
	CORE_API static uint32 ParseNumber(const ANSICHAR* Literal, int32 Len);

public:

	friend bool operator==(FName Name, const FLazyName& Lazy)
	{
		// If !Name.IsNone(), we have started creating FNames
		// and might as well resolve and cache Lazy
		if (Lazy.Either.IsName() || !Name.IsNone())
		{
			return Name == Lazy.Resolve();
		}
		else if (!Lazy.bLiteralIsWide)
		{
			return Name == Lazy.Either.AsAnsiLiteral();
		}
		else
		{
			return Name == Lazy.Either.AsWideLiteral();
		}
	}

	friend bool operator==(const FLazyName& Lazy, FName Name)
	{
		return Name == Lazy;
	}

	CORE_API friend bool operator==(const FLazyName& A, const FLazyName& B);

};

/**
 * A string builder with inline storage for FNames.
 */
class FNameBuilder : public TStringBuilder<FName::StringBufferSize>
{
public:
	FNameBuilder() = default;

	inline explicit FNameBuilder(const FName InName)
	{
		InName.AppendString(*this);
	}
};

template <> struct TIsContiguousContainer<FNameBuilder> { static constexpr bool Value = true; };

/** Update the Hash with the FName's text and number */
class FBlake3;
CORE_API void AppendHash(FBlake3& Builder, FName In);