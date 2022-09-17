// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Traits/IntType.h"
#include <ctype.h>
#include <wctype.h>

/*-----------------------------------------------------------------------------
	Character type functions.
-----------------------------------------------------------------------------*/

/**
 * Templated literal struct to allow selection of wide or ansi string literals
 * based on the character type provided, and not on compiler switches.
 */
template <typename T> struct TLiteral
{
	static const ANSICHAR  Select(const ANSICHAR  ansi, const WIDECHAR ) { return ansi; }
	static const ANSICHAR* Select(const ANSICHAR* ansi, const WIDECHAR*) { return ansi; }
};

template <> struct TLiteral<WIDECHAR>
{
	static const WIDECHAR  Select(const ANSICHAR,  const WIDECHAR  wide) { return wide; }
	static const WIDECHAR* Select(const ANSICHAR*, const WIDECHAR* wide) { return wide; }
};

#define LITERAL(CharType, StringLiteral) TLiteral<CharType>::Select(StringLiteral, TEXT(StringLiteral))


template <typename CharType, const unsigned int Size>
struct TCharBase
{
	static constexpr CharType LineFeed           = (CharType)0xa;
	static constexpr CharType VerticalTab        = (CharType)0xb;
	static constexpr CharType FormFeed           = (CharType)0xc;
	static constexpr CharType CarriageReturn     = (CharType)0xd;
	static constexpr CharType NextLine           = (CharType)0x85;
	static constexpr CharType LineSeparator      = (CharType)0x2028;
	static constexpr CharType ParagraphSeparator = (CharType)0x2029;

	static bool IsLinebreak(CharType Char)
	{
		return ((uint32(Char) - LineFeed) <= uint32(CarriageReturn - LineFeed)) |
			(Char == NextLine) | (Char == LineSeparator) | (Char == ParagraphSeparator);
	}

};

template <typename CharType>
struct TCharBase<CharType, 1>
{
	static constexpr CharType LineFeed       = (CharType)0xa;
	static constexpr CharType VerticalTab    = (CharType)0xb;
	static constexpr CharType FormFeed       = (CharType)0xc;
	static constexpr CharType CarriageReturn = (CharType)0xd;

	static bool IsLinebreak(CharType Char)
	{
		return ((uint32(Char) - LineFeed) <= uint32(CarriageReturn - LineFeed));
	}
};


/**
 * TChar
 * Set of utility functions operating on a single character. The functions
 * are specialized for ANSICHAR and TCHAR character types. You can use the
 * typedefs FChar and FCharAnsi for convenience.
 */
template <typename CharType>
struct TChar : TCharBase<CharType, sizeof(CharType)>
{
	/**
	* Only converts ASCII characters, same as CRT to[w]upper() with standard C locale
	*/
	static CharType ToUpper(CharType Char)
	{
		return (CharType)(ToUnsigned(Char) - ((uint32(Char) - 'a' < 26u) << 5));
	}

	/**
	* Only converts ASCII characters, same as CRT to[w]upper() with standard C locale
	*/
	static CharType ToLower(CharType Char)
	{
		return (CharType)(ToUnsigned(Char) + ((uint32(Char) - 'A' < 26u) << 5));
	}

	static bool IsUpper(CharType Char);
	static bool IsLower(CharType Char);
	static bool IsAlpha(CharType Char);
	static bool IsGraph(CharType Char);
	static bool IsPrint(CharType Char);
	static bool IsPunct(CharType Char);
	static bool IsAlnum(CharType Char);
	static bool IsDigit(CharType Char);
	static bool IsHexDigit(CharType Char);
	static bool IsWhitespace(CharType Char);
	static bool IsControl(CharType Char);

	static bool IsOctDigit(CharType Char)
	{
		return uint32(Char) - '0' < 8u;
	}

	static int32 ConvertCharDigitToInt(CharType Char)
	{
		return static_cast<int32>(Char) - static_cast<int32>('0');
	}

	static bool IsIdentifier(CharType Char)
	{
		return IsAlnum(Char) || IsUnderscore(Char);
	}

	static bool IsUnderscore(CharType Char)
	{
		return Char == LITERAL(CharType, '_');
	}

	/**
	* Avoid sign extension problems with signed characters smaller than int
	*
	* E.g. 'Ö' - 'A' is negative since the char 'Ö' (0xD6) is negative and gets
	* sign-extended to the 32-bit int 0xFFFFFFD6 before subtraction happens.
	*
	* Mainly needed for subtraction and addition.
	*/
	static constexpr FORCEINLINE uint32 ToUnsigned(CharType Char)
	{
		return (typename TUnsignedIntType<sizeof(CharType)>::Type)Char;
	}
};

typedef TChar<TCHAR>    FChar;
typedef TChar<WIDECHAR> FCharWide;
typedef TChar<ANSICHAR> FCharAnsi;

/*-----------------------------------------------------------------------------
	WIDECHAR specialized functions
-----------------------------------------------------------------------------*/

template <> inline bool TChar<WIDECHAR>::IsUpper(WIDECHAR Char) { return ::iswupper(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsLower(WIDECHAR Char) { return ::iswlower(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsAlpha(WIDECHAR Char) { return ::iswalpha(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsGraph(WIDECHAR Char) { return ::iswgraph(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsPrint(WIDECHAR Char) { return ::iswprint(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsPunct(WIDECHAR Char) { return ::iswpunct(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsAlnum(WIDECHAR Char) { return ::iswalnum(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsDigit(WIDECHAR Char) { return ::iswdigit(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsHexDigit(WIDECHAR Char) { return ::iswxdigit(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsWhitespace(WIDECHAR Char) { return ::iswspace(Char) != 0; }
template <> inline bool TChar<WIDECHAR>::IsControl(WIDECHAR Char) { return ::iswcntrl(Char) != 0; }


/*-----------------------------------------------------------------------------
	ANSICHAR specialized functions
-----------------------------------------------------------------------------*/
template <> inline bool TChar<ANSICHAR>::IsUpper(ANSICHAR Char) { return ::isupper((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsLower(ANSICHAR Char) { return ::islower((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsAlpha(ANSICHAR Char) { return ::isalpha((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsGraph(ANSICHAR Char) { return ::isgraph((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsPrint(ANSICHAR Char) { return ::isprint((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsPunct(ANSICHAR Char) { return ::ispunct((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsAlnum(ANSICHAR Char) { return ::isalnum((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsDigit(ANSICHAR Char) { return ::isdigit((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsHexDigit(ANSICHAR Char) { return ::isxdigit((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsWhitespace(ANSICHAR Char) { return ::isspace((unsigned char)Char) != 0; }
template <> inline bool TChar<ANSICHAR>::IsControl(ANSICHAR Char) { return ::iscntrl((unsigned char)Char) != 0; }

template <> inline bool TChar<UTF8CHAR>::IsWhitespace(UTF8CHAR Char) { return ::isspace((unsigned char)Char) != 0; }
