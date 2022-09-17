// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/StringFwd.h"
#include "HAL/UnrealMemory.h"
#include "Math/NumericLimits.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Crc.h"
#include "Misc/CString.h"
#include "String/Find.h"
#include "Templates/UnrealTemplate.h"
#include "Traits/ElementType.h"
#include "Traits/IsCharType.h"
#include "Traits/IsContiguousContainer.h"
#include <type_traits>

namespace UE::Core::Private
{
	/** Allow GetData to called unqualified from a scope with its own overload of GetData. */
	template <typename... ArgTypes>
	constexpr inline auto StringViewGetData(ArgTypes&&... Args) -> decltype(GetData(Forward<ArgTypes>(Args)...))
	{
		return GetData(Forward<ArgTypes>(Args)...);
	}
} // UE::Core::Private

/**
 * A string view is a non-owning view of a range of characters.
 *
 * Ensure that the underlying string is valid for the lifetime of the string view.
 *
 * Be careful when constructing a string view from a temporary. Make a local copy if necessary.
 *
 * FStringView View = Object->GetPathName(); // Invalid
 *
 * FString PathName = Object->GetPathName(); // Valid
 * FStringView View = PathName;
 *
 * void ProcessPath(FStringView Path);       // Valid
 * ProcessPath(Object->GetPathName());
 *
 * A string view is implicitly constructible from null-terminated strings, from contiguous ranges
 * of characters such as FString and TStringBuilder, and from literals such as TEXT("..."_SV).
 *
 * A string view is cheap to copy and is meant to be passed by value. Avoid passing by reference.
 *
 * A string view is not guaranteed to represent a null-terminated string.
 *
 * Log or format a string view using UE_LOG(TEXT("%.*s"), View.Len(), View.GetData());
 *
 * A string view is a good fit for function parameters where the function has no requirements for
 * how the string is stored. A caller may use FString, FStringView, TStringBuilder, a char array,
 * a null-terminated string, or any other type which can convert to a string view.
 *
 * The UE::String namespace contains many functions that can operate on string views.
 * Most of these functions can be found in String/___.h in Core.
 *
 * @code
 *	void UseString(FStringView InString);
 *
 *	FString String(TEXT("ABC"));
 *	const TCHAR* CString = *String;
 *	TStringBuilder<16> StringBuilder;
 *	StringBuilder.Append(TEXT("ABC"));
 *
 *	UseString(String);
 *	UseString(CString);
 *	UseString(StringBuilder);
 *	UseString(TEXT("ABC"));
 *	UseString(TEXT("ABC"_SV));
 * @endcode
 */
template <typename CharType>
class TStringView
{
public:
	using ElementType = CharType;
	using SizeType UE_DEPRECATED(5.0, "Use int32 instead.") = int32;
	using ViewType = TStringView<CharType>;

public:
	/** Construct an empty view. */
	constexpr TStringView() = default;

	/** Construct a view of the null-terminated string pointed to by InData. */
	constexpr inline TStringView(const CharType* InData)
		: DataPtr(InData)
		, Size(InData ? TCString<CharType>::Strlen(InData) : 0)
	{
	}

	/** Construct a view of InSize characters beginning at InData. */
	constexpr inline TStringView(const CharType* InData, int32 InSize)
		: DataPtr(InData)
		, Size(InSize)
	{
	}

	/** Construct a view of the null-terminated string pointed to by InData. */
	template <typename OtherCharType,
		std::enable_if_t<FPlatformString::IsCharEncodingCompatibleWith<OtherCharType, CharType>()>* = nullptr>
	constexpr inline TStringView(const OtherCharType* InData)
		: DataPtr((const CharType*)InData)
		, Size(InData ? TCString<CharType>::Strlen((const CharType*)InData) : 0)
	{
	}

	/** Construct a view of InSize characters beginning at InData. */
	template <typename OtherCharType,
		std::enable_if_t<FPlatformString::IsCharEncodingCompatibleWith<OtherCharType, CharType>()>* = nullptr>
	constexpr inline TStringView(const OtherCharType* InData, int32 InSize)
		: DataPtr((const CharType*)InData)
		, Size(InSize)
	{
	}

	/** Construct a view from a contiguous range of characters, such as FString or TStringBuilder. */
	template <typename CharRangeType,
		std::enable_if_t<
			TIsContiguousContainer<CharRangeType>::Value &&
			TIsCharType<TElementType_T<CharRangeType>>::Value &&
			FPlatformString::IsCharEncodingCompatibleWith<TElementType_T<CharRangeType>, CharType>() &&
			!std::is_array_v<std::remove_reference_t<CharRangeType>> &&
			!std::is_same_v<CharRangeType, ViewType>
		>* = nullptr>
	constexpr inline TStringView(const CharRangeType& InRange)
		: DataPtr((const CharType*)UE::Core::Private::StringViewGetData(InRange))
		, Size(IntCastChecked<int32>(GetNum(InRange)))
	{
	}

	/** Access the character at the given index in the view. */
	inline const CharType& operator[](int32 Index) const;

	/** Returns a pointer to the start of the view. This is NOT guaranteed to be null-terminated! */
	[[nodiscard]] constexpr inline const CharType* GetData() const { return DataPtr; }

	// Capacity

	/** Returns the length of the string view. */
	[[nodiscard]] constexpr inline int32 Len() const { return Size; }

	/** Returns whether the string view is empty. */
	[[nodiscard]] constexpr inline bool IsEmpty() const { return Size == 0; }

	// Modifiers

	/** Modifies the view to remove the given number of characters from the start. */
	inline void RemovePrefix(int32 CharCount) { DataPtr += CharCount; Size -= CharCount; }
	/** Modifies the view to remove the given number of characters from the end. */
	inline void RemoveSuffix(int32 CharCount) { Size -= CharCount; }
	/** Resets to an empty view */
	inline void Reset() { DataPtr = nullptr; Size = 0; }

	// Operations

	/**
	 * Copy characters from the view into a destination buffer without null termination.
	 *
	 * @param Dest Buffer to write into. Must have space for at least CharCount characters.
	 * @param CharCount The maximum number of characters to copy.
	 * @param Position The offset into the view from which to start copying.
	 *
	 * @return The number of characters written to the destination buffer.
	 */
	inline int32 CopyString(CharType* Dest, int32 CharCount, int32 Position = 0) const;

	/** Alias for Mid. */
	[[nodiscard]] inline ViewType SubStr(int32 Position, int32 CharCount) const { return Mid(Position, CharCount); }

	/** Returns the left-most part of the view by taking the given number of characters from the left. */
	[[nodiscard]] inline ViewType Left(int32 CharCount) const;
	/** Returns the left-most part of the view by chopping the given number of characters from the right. */
	[[nodiscard]] inline ViewType LeftChop(int32 CharCount) const;
	/** Returns the right-most part of the view by taking the given number of characters from the right. */
	[[nodiscard]] inline ViewType Right(int32 CharCount) const;
	/** Returns the right-most part of the view by chopping the given number of characters from the left. */
	[[nodiscard]] inline ViewType RightChop(int32 CharCount) const;
	/** Returns the middle part of the view by taking up to the given number of characters from the given position. */
	[[nodiscard]] inline ViewType Mid(int32 Position, int32 CharCount = MAX_int32) const;
	/** Returns the middle part of the view between any whitespace at the start and end. */
	[[nodiscard]] inline ViewType TrimStartAndEnd() const;
	/** Returns the right part of the view after any whitespace at the start. */
	[[nodiscard]] inline ViewType TrimStart() const;
	/** Returns the left part of the view before any whitespace at the end. */
	[[nodiscard]] inline ViewType TrimEnd() const;

	/** Modifies the view to be the given number of characters from the left. */
	inline void LeftInline(int32 CharCount) { *this = Left(CharCount); }
	/** Modifies the view by chopping the given number of characters from the right. */
	inline void LeftChopInline(int32 CharCount) { *this = LeftChop(CharCount); }
	/** Modifies the view to be the given number of characters from the right. */
	inline void RightInline(int32 CharCount) { *this = Right(CharCount); }
	/** Modifies the view by chopping the given number of characters from the left. */
	inline void RightChopInline(int32 CharCount) { *this = RightChop(CharCount); }
	/** Modifies the view to be the middle part by taking up to the given number of characters from the given position. */
	inline void MidInline(int32 Position, int32 CharCount = MAX_int32) { *this = Mid(Position, CharCount); }
	/** Modifies the view to be the middle part between any whitespace at the start and end. */
	inline void TrimStartAndEndInline() { *this = TrimStartAndEnd(); }
	/** Modifies the view to be the right part after any whitespace at the start. */
	inline void TrimStartInline() { *this = TrimStart(); }
	/** Modifies the view to be the left part before any whitespace at the end. */
	inline void TrimEndInline() { *this = TrimEnd(); }

	// Comparison

	/**
	 * Check whether this view is equivalent to a character range.
	 *
	 * @param Other        A character range that is comparable with the character type of this view.
	 * @param SearchCase   Whether the comparison should ignore case.
	 */
	template <typename OtherRangeType, decltype(MakeStringView(DeclVal<OtherRangeType>()))* = nullptr>
	[[nodiscard]] inline bool Equals(OtherRangeType&& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const
	{
		const auto OtherView = MakeStringView(Forward<OtherRangeType>(Other));
		return Len() == OtherView.Len() && Compare(OtherView, SearchCase) == 0;
	}

	/**
	 * Check whether this view is equivalent to a string view.
	 *
	 * @param Other        A string that is comparable with the character type of this view.
	 * @param SearchCase   Whether the comparison should ignore case.
	 */
	template <typename OtherCharType,
		std::enable_if_t<TIsCharType<OtherCharType>::Value>* = nullptr>
	[[nodiscard]] inline bool Equals(const OtherCharType* Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const;

	/**
	 * Compare this view with a character range.
	 *
	 * @param Other        A character range that is comparable with the character type of this view.
	 * @param SearchCase   Whether the comparison should ignore case.
	 * @return 0 is equal, negative if this view is less, positive if this view is greater.
	 */
	template <typename OtherRangeType, decltype(MakeStringView(DeclVal<OtherRangeType>()))* = nullptr>
	[[nodiscard]] inline int32 Compare(OtherRangeType&& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const
	{
		return Compare(MakeStringView(Forward<OtherRangeType>(Other)), SearchCase);
	}

	/**
	 * Compare this view with a string view.
	 *
	 * @param Other        A string view that is comparable with the character type of this view.
	 * @param SearchCase   Whether the comparison should ignore case.
	 * @return 0 is equal, negative if this view is less, positive if this view is greater.
	 */
	template <typename OtherCharType,
		std::enable_if_t<TIsCharType<OtherCharType>::Value>* = nullptr>
		[[nodiscard]] inline int32 Compare(TStringView<OtherCharType> Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const;

	/**
	 * Compare this view with a null-terminated string.
	 *
	 * @param Other        A null-terminated string that is comparable with the character type of this view.
	 * @param SearchCase   Whether the comparison should ignore case.
	 * @return 0 is equal, negative if this view is less, positive if this view is greater.
	 */
	template <typename OtherCharType>
	[[nodiscard]] inline int32 Compare(const OtherCharType* Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const;

	/** Returns whether this view starts with the prefix character compared case-sensitively. */
	[[nodiscard]] inline bool StartsWith(CharType Prefix) const { return Size >= 1 && DataPtr[0] == Prefix; }
	/** Returns whether this view starts with the prefix with optional case sensitivity. */
	[[nodiscard]] inline bool StartsWith(ViewType Prefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;

	/** Returns whether this view ends with the suffix character compared case-sensitively. */
	[[nodiscard]] inline bool EndsWith(CharType Suffix) const { return Size >= 1 && DataPtr[Size-1] == Suffix; }
	/** Returns whether this view ends with the suffix with optional case sensitivity. */
	[[nodiscard]] inline bool EndsWith(ViewType Suffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;

	// Searching/Finding

	/**
	 * Search the view for the first occurrence of a search string.
	 *
	 * @param Search          The string to search for. Comparison is lexicographic.
	 * @param StartPosition   The character position to start searching from.
	 * @return The index of the first occurrence of the search string if found, otherwise INDEX_NONE.
	 */
	[[nodiscard]] inline int32 Find(ViewType Search, int32 StartPosition = 0) const;

	/**
	 * Returns whether this view contains the specified substring.
	 *
	 * @param Search   Text to search for
	 * @return True if the view contains the search string, otherwise false.
	 */
	[[nodiscard]] inline bool Contains(ViewType Search) const
	{
		return Find(Search) != INDEX_NONE;
	}

	/**
	 * Search the view for the first occurrence of a character.
	 *
	 * @param Search           The character to search for. Comparison is lexicographic.
	 * @param OutIndex [out]   The position at which the character was found, or INDEX_NONE if not found.
	 * @return True if the character was found in the view, otherwise false.
	 */
	inline bool FindChar(CharType Search, int32& OutIndex) const;

	/**
	 * Search the view for the last occurrence of a character.
	 *
	 * @param Search           The character to search for. Comparison is lexicographic.
	 * @param OutIndex [out]   The position at which the character was found, or INDEX_NONE if not found.
	 * @return True if the character was found in the view, otherwise false.
	 */
	inline bool FindLastChar(CharType Search, int32& OutIndex) const;

public:
	/**
	 * DO NOT USE DIRECTLY
	 * STL-like iterators to enable range-based for loop support.
	 */
	constexpr inline const CharType* begin() const { return DataPtr; }
	constexpr inline const CharType* end() const { return DataPtr + Size; }

protected:
	const CharType* DataPtr = nullptr;
	int32 Size = 0;
};

template <typename CharRangeType>
TStringView(CharRangeType&& Range)
	-> TStringView<TElementType_T<CharRangeType>>;

template <typename CharPtrOrRangeType>
constexpr inline auto MakeStringView(CharPtrOrRangeType&& CharPtrOrRange) -> decltype(TStringView(Forward<CharPtrOrRangeType>(CharPtrOrRange)))
{
	return TStringView(Forward<CharPtrOrRangeType>(CharPtrOrRange));
}

template <typename CharPtrType, typename SizeType>
constexpr inline auto MakeStringView(CharPtrType&& CharPtr, SizeType&& Size) -> decltype(TStringView(Forward<CharPtrType>(CharPtr), Forward<SizeType>(Size)))
{
	return TStringView(Forward<CharPtrType>(CharPtr), Forward<SizeType>(Size));
}

template <typename CharType>
constexpr inline auto GetNum(TStringView<CharType> String)
{
	return String.Len();
}

//////////////////////////////////////////////////////////////////////////

#if PLATFORM_TCHAR_IS_UTF8CHAR
inline FStringView operator "" _SV(const ANSICHAR* String, size_t Size)
{
	return FStringView((const UTF8CHAR*)String, Size);
}
#else
constexpr inline FStringView operator "" _SV(const TCHAR* String, size_t Size)
{
	return FStringView(String, Size);
}
#endif

constexpr inline FAnsiStringView operator "" _ASV(const ANSICHAR* String, size_t Size)
{
	return FAnsiStringView(String, Size);
}

constexpr inline FWideStringView operator "" _WSV(const WIDECHAR* String, size_t Size)
{
	return FWideStringView(String, Size);
}

/*constexpr*/ inline FUtf8StringView operator "" _U8SV(const ANSICHAR* String, size_t Size)
{
	// Would like this operator to be constexpr, but cannot be until after this operator can take a UTF8CHAR*
	// rather than an ANSICHAR*, which won't be until we have C++20 char8_t string literals.
	return FUtf8StringView(reinterpret_cast<const UTF8CHAR*>(String), Size);
}

#if PLATFORM_TCHAR_IS_UTF8CHAR
	#define TEXTVIEW(str) (str##_SV)
#else
	#define TEXTVIEW(str) TEXT(str##_SV)
#endif
#define ANSITEXTVIEW(str) (str##_ASV)
#define WIDETEXTVIEW(str) PREPROCESSOR_JOIN(WIDETEXT(str), _WSV)
#define UTF8TEXTVIEW(str) (str##_U8SV)

//////////////////////////////////////////////////////////////////////////

/** Case insensitive string hash function. */
template <typename CharType>
FORCEINLINE uint32 GetTypeHash(TStringView<CharType> View)
{
	// This must match the GetTypeHash behavior of FString
	return FCrc::Strihash_DEPRECATED(View.Len(), View.GetData());
}

template <typename CharType>
inline const CharType& TStringView<CharType>::operator[](int32 Index) const
{
	checkf(Index >= 0 && Index < Size, TEXT("Index out of bounds on StringView: index %i on a view with a length of %i"), Index, Size);
	return DataPtr[Index];
}

template <typename CharType>
inline int32 TStringView<CharType>::CopyString(CharType* Dest, int32 CharCount, int32 Position) const
{
	const int32 CopyCount = FMath::Min(Size - Position, CharCount);
	FMemory::Memcpy(Dest, DataPtr + Position, CopyCount * sizeof(CharType));
	return CopyCount;
}

template <typename CharType>
inline TStringView<CharType> TStringView<CharType>::Left(int32 CharCount) const
{
	return ViewType(DataPtr, FMath::Clamp(CharCount, 0, Size));
}

template <typename CharType>
inline TStringView<CharType> TStringView<CharType>::LeftChop(int32 CharCount) const
{
	return ViewType(DataPtr, FMath::Clamp(Size - CharCount, 0, Size));
}

template <typename CharType>
inline TStringView<CharType> TStringView<CharType>::Right(int32 CharCount) const
{
	const int32 OutLen = FMath::Clamp(CharCount, 0, Size);
	return ViewType(DataPtr + Size - OutLen, OutLen);
}

template <typename CharType>
inline TStringView<CharType> TStringView<CharType>::RightChop(int32 CharCount) const
{
	const int32 OutLen = FMath::Clamp(Size - CharCount, 0, Size);
	return ViewType(DataPtr + Size - OutLen, OutLen);
}

template <typename CharType>
inline TStringView<CharType> TStringView<CharType>::Mid(int32 Position, int32 CharCount) const
{
	const CharType* CurrentStart  = GetData();
	const int32 CurrentLength = Len();

	// Clamp minimum position at the start of the string, adjusting the length down if necessary
	const int32 NegativePositionOffset = (Position < 0) ? Position : 0;
	CharCount += NegativePositionOffset;
	Position  -= NegativePositionOffset;

	// Clamp maximum position at the end of the string
	Position = (Position > CurrentLength) ? CurrentLength : Position;

	// Clamp count between 0 and the distance to the end of the string
	CharCount = FMath::Clamp(CharCount, 0, (CurrentLength - Position));

	ViewType Result = ViewType(CurrentStart + Position, CharCount);
	return Result;
}

template <typename CharType>
inline TStringView<CharType> TStringView<CharType>::TrimStartAndEnd() const
{
	return TrimStart().TrimEnd();
}

template <typename CharType>
inline TStringView<CharType> TStringView<CharType>::TrimStart() const
{
	int32 SpaceCount = 0;
	for (CharType Char : *this)
	{
		if (!TChar<CharType>::IsWhitespace(Char))
		{
			break;
		}
		++SpaceCount;
	}
	return ViewType(DataPtr + SpaceCount, Size - SpaceCount);
}

template <typename CharType>
inline TStringView<CharType> TStringView<CharType>::TrimEnd() const
{
	int32 NewSize = Size;
	while (NewSize && TChar<CharType>::IsWhitespace(DataPtr[NewSize - 1]))
	{
		--NewSize;
	}
	return ViewType(DataPtr, NewSize);
}

template <typename CharType>
template <typename OtherCharType,
	std::enable_if_t<TIsCharType<OtherCharType>::Value>*>
inline bool TStringView<CharType>::Equals(const OtherCharType* const Other, ESearchCase::Type SearchCase) const
{
	if (IsEmpty())
	{
		return !*Other;
	}

	if (SearchCase == ESearchCase::CaseSensitive)
	{
		return FPlatformString::Strncmp(GetData(), Other, Len()) == 0 && !Other[Len()];
	}
	else
	{
		return FPlatformString::Strnicmp(GetData(), Other, Len()) == 0 && !Other[Len()];
	}
}

template <typename CharType>
template <typename OtherCharType,
	std::enable_if_t<TIsCharType<OtherCharType>::Value>*>
inline int32 TStringView<CharType>::Compare(TStringView<OtherCharType> OtherView, ESearchCase::Type SearchCase) const
{
	const int32 SelfLen = Len();
	const int32 OtherLen = OtherView.Len();
	const int32 MinLen = FMath::Min(SelfLen, OtherLen);

	if (!SelfLen || !OtherLen)
	{
		return SelfLen - OtherLen;
	}

	int Result;
	if (SearchCase == ESearchCase::CaseSensitive)
	{
		Result = FPlatformString::Strncmp(GetData(), OtherView.GetData(), MinLen);
	}
	else
	{
		Result = FPlatformString::Strnicmp(GetData(), OtherView.GetData(), MinLen);
	}

	if (Result != 0)
	{
		return Result;
	}

	return SelfLen - OtherLen;
}

template <typename CharType>
template <typename OtherCharType>
inline int32 TStringView<CharType>::Compare(const OtherCharType* const Other, ESearchCase::Type SearchCase) const
{
	if (IsEmpty())
	{
		return -!!*Other;
	}

	int Result;
	if (SearchCase == ESearchCase::CaseSensitive)
	{
		Result = FPlatformString::Strncmp(GetData(), Other, Len());
	}
	else
	{
		Result = FPlatformString::Strnicmp(GetData(), Other, Len());
	}

	if (Result != 0)
	{
		return Result;
	}

	// Equal if Other[Len()] is '\0' and less otherwise. !!Other[Len()] is 0 for '\0' and 1 otherwise.
	return -!!Other[Len()];
}

template <typename CharType>
inline bool TStringView<CharType>::StartsWith(ViewType Prefix, ESearchCase::Type SearchCase) const
{
	return Prefix.Equals(Left(Prefix.Len()), SearchCase);
}

template <typename CharType>
inline bool TStringView<CharType>::EndsWith(ViewType Suffix, ESearchCase::Type SearchCase) const
{
	return Suffix.Equals(Right(Suffix.Len()), SearchCase);
}

template <typename CharType>
inline int32 TStringView<CharType>::Find(const ViewType Search, const int32 StartPosition) const
{
	const int32 Index = UE::String::FindFirst(RightChop(StartPosition), Search);
	return Index == INDEX_NONE ? INDEX_NONE : Index + StartPosition;
}

template <typename CharType>
inline bool TStringView<CharType>::FindChar(const CharType Search, int32& OutIndex) const
{
	OutIndex = UE::String::FindFirstChar(*this, Search);
	return OutIndex != INDEX_NONE;
}


template <typename CharType>
inline bool TStringView<CharType>::FindLastChar(const CharType Search, int32& OutIndex) const
{
	OutIndex = UE::String::FindLastChar(*this, Search);
	return OutIndex != INDEX_NONE;
}

// Case-insensitive string view comparison operators

template <typename CharType>
inline bool operator==(TStringView<CharType> Lhs, TStringView<CharType> Rhs)
{
	return Lhs.Equals(Rhs, ESearchCase::IgnoreCase);
}

template <typename CharType>
inline bool operator!=(TStringView<CharType> Lhs, TStringView<CharType> Rhs)
{
	return !(Lhs == Rhs);
}

template <typename CharType>
inline bool operator<(TStringView<CharType> Lhs, TStringView<CharType> Rhs)
{
	return Lhs.Compare(Rhs, ESearchCase::IgnoreCase) < 0;
}

// Case-insensitive character range comparison operators

template <typename CharType, typename CharRangeType>
inline auto operator==(TStringView<CharType> Lhs, CharRangeType&& Rhs)
	-> decltype(Lhs.Equals(ImplicitConv<TStringView<CharType>>(Forward<CharRangeType>(Rhs)), ESearchCase::IgnoreCase))
{
	return Lhs.Equals(ImplicitConv<TStringView<CharType>>(Forward<CharRangeType>(Rhs)), ESearchCase::IgnoreCase);
}

template <typename CharType, typename CharRangeType>
inline auto operator==(CharRangeType&& Lhs, TStringView<CharType> Rhs)
	-> decltype(ImplicitConv<TStringView<CharType>>(Forward<CharRangeType>(Lhs)).Equals(Rhs, ESearchCase::IgnoreCase))
{
	return ImplicitConv<TStringView<CharType>>(Forward<CharRangeType>(Lhs)).Equals(Rhs, ESearchCase::IgnoreCase);
}

template <typename CharType, typename CharRangeType>
inline auto operator!=(TStringView<CharType> Lhs, CharRangeType&& Rhs) -> decltype(!(Lhs == Forward<CharRangeType>(Rhs)))
{
	return !(Lhs == Forward<CharRangeType>(Rhs));
}

template <typename CharType, typename CharRangeType>
inline auto operator!=(CharRangeType&& Lhs, TStringView<CharType> Rhs) -> decltype(!(Rhs == Forward<CharRangeType>(Lhs)))
{
	return !(Rhs == Forward<CharRangeType>(Lhs));
}

template <typename CharType, typename CharRangeType>
inline auto operator<(TStringView<CharType> Lhs, CharRangeType&& Rhs)
	-> decltype(Lhs.Compare(ImplicitConv<TStringView<CharType>>(Forward<CharRangeType>(Rhs)), ESearchCase::IgnoreCase) < 0)
{
	return Lhs.Compare(ImplicitConv<TStringView<CharType>>(Forward<CharRangeType>(Rhs)), ESearchCase::IgnoreCase) < 0;
}

template <typename CharType, typename CharRangeType>
inline auto operator<(CharRangeType&& Lhs, TStringView<CharType> Rhs)
	-> decltype(ImplicitConv<TStringView<CharType>>(Forward<CharRangeType>(Lhs)).Compare(Rhs, ESearchCase::IgnoreCase) < 0)
{
	return ImplicitConv<TStringView<CharType>>(Forward<CharRangeType>(Lhs)).Compare(Rhs, ESearchCase::IgnoreCase) < 0;
}

// Case-insensitive C-string comparison operators

template <typename CharType>
inline bool operator==(TStringView<CharType> Lhs, const CharType* Rhs)
{
	return Lhs.Equals(Rhs, ESearchCase::IgnoreCase);
}

template <typename CharType>
inline bool operator==(const CharType* Lhs, TStringView<CharType> Rhs)
{
	return Rhs == Lhs;
}

template <typename CharType>
inline bool operator!=(TStringView<CharType> Lhs, const CharType* Rhs)
{
	return !(Lhs == Rhs);
}

template <typename CharType>
inline bool operator!=(const CharType* Lhs, TStringView<CharType> Rhs)
{
	return !(Lhs == Rhs);
}
