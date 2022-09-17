// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/EnableIf.h"
#include "Traits/IsCharType.h"
#include "GenericPlatform/GenericPlatformStricmp.h"
#include <type_traits>

namespace UE::Core::Private
{
	// The Dest parameter is just used for overload resolution
	CORE_API int32 GetConvertedLength(const UTF8CHAR* Dest, const ANSICHAR* Src, int32 SrcLen);
	CORE_API int32 GetConvertedLength(const UTF8CHAR* Dest, const WIDECHAR* Src, int32 SrcLen);
	CORE_API int32 GetConvertedLength(const UTF8CHAR* Dest, const UCS2CHAR* Src, int32 SrcLen);
	CORE_API int32 GetConvertedLength(const ANSICHAR* Dest, const UTF8CHAR* Src, int32 SrcLen);
	CORE_API int32 GetConvertedLength(const WIDECHAR* Dest, const UTF8CHAR* Src, int32 SrcLen);
	CORE_API int32 GetConvertedLength(const UCS2CHAR* Dest, const UTF8CHAR* Src, int32 SrcLen);

	CORE_API UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const ANSICHAR* Src, int32 SrcLen);
	CORE_API UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const WIDECHAR* Src, int32 SrcLen);
	CORE_API UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const UCS2CHAR* Src, int32 SrcLen);
	CORE_API ANSICHAR* Convert(ANSICHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen);
	CORE_API WIDECHAR* Convert(WIDECHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen);
	CORE_API UCS2CHAR* Convert(UCS2CHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen);
}

// These will be moved inside GenericPlatformString.cpp when the platform layer handles UTF-16
// instead of StringConv.h.
#define HIGH_SURROGATE_START_CODEPOINT    ((uint16)0xD800)
#define HIGH_SURROGATE_END_CODEPOINT      ((uint16)0xDBFF)
#define LOW_SURROGATE_START_CODEPOINT     ((uint16)0xDC00)
#define LOW_SURROGATE_END_CODEPOINT       ((uint16)0xDFFF)
#define ENCODED_SURROGATE_START_CODEPOINT ((uint32)0x10000)
#define ENCODED_SURROGATE_END_CODEPOINT   ((uint32)0x10FFFF)

#define UNICODE_BOGUS_CHAR_CODEPOINT '?'
static_assert(sizeof(UNICODE_BOGUS_CHAR_CODEPOINT) <= sizeof(ANSICHAR) && (UNICODE_BOGUS_CHAR_CODEPOINT) >= 32 && (UNICODE_BOGUS_CHAR_CODEPOINT) <= 127, "The Unicode Bogus character point is expected to fit in a single ANSICHAR here");

/**
 * Generic string implementation for most platforms
 */
struct FGenericPlatformString : public FGenericPlatformStricmp
{
	/**
	 * Tests whether an encoding has fixed-width characters
	 */
	template <typename Encoding>
	static constexpr bool IsFixedWidthEncoding()
	{
		static_assert(TIsCharType<Encoding>::Value, "Encoding is not a char type");

		return
			std::is_same_v<Encoding, ANSICHAR> ||
			std::is_same_v<Encoding, UCS2CHAR> || // this may not be true when PLATFORM_UCS2CHAR_IS_UTF16CHAR == 1, but this is the legacy behavior
			std::is_same_v<Encoding, WIDECHAR> || // the UCS2CHAR comment also applies to WIDECHAR
#if PLATFORM_TCHAR_IS_CHAR16
			std::is_same_v<Encoding, wchar_t>  || // the UCS2CHAR comment also applies to wchar_t
#endif
			std::is_same_v<Encoding, UTF32CHAR>;
	}

	/**
	 * Function which returns whether one encoding type is binary compatible with another.
	 *
	 * Unlike TAreEncodingsCompatible, this is not commutative.  For example, ANSI is compatible with
	 * UTF-8, but UTF-8 is not compatible with ANSI.
	 */
	template <typename SrcEncoding, typename DestEncoding>
	static constexpr bool IsCharEncodingCompatibleWith()
	{
		if constexpr (std::is_same_v<SrcEncoding, DestEncoding>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, ANSICHAR> && std::is_same_v<DestEncoding, UTF8CHAR>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, UCS2CHAR> && std::is_same_v<DestEncoding, UTF16CHAR>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, WIDECHAR> && std::is_same_v<DestEncoding, UCS2CHAR>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, UCS2CHAR> && std::is_same_v<DestEncoding, WIDECHAR>)
		{
			return true;
		}
#if PLATFORM_TCHAR_IS_CHAR16
		else if constexpr (std::is_same_v<SrcEncoding, WIDECHAR> && std::is_same_v<DestEncoding, wchar_t>)
		{
			return true;
		}
		else if constexpr (std::is_same_v<SrcEncoding, wchar_t> && std::is_same_v<DestEncoding, WIDECHAR>)
		{
			return true;
		}
#endif
		else
		{
			return false;
		}
	};

	/**
	 * Tests whether you can simply (i.e. by assignment) encode code units from the source encoding as the destination encoding.
	 */
	template <typename SourceEncoding, typename DestEncoding>
	static constexpr bool IsCharEncodingSimplyConvertibleTo()
	{
		if constexpr (IsCharEncodingCompatibleWith<SourceEncoding, DestEncoding>())
		{
			// Binary-compatible conversions are always simple
			return true;
		}
		else if constexpr (IsFixedWidthEncoding<SourceEncoding>() && sizeof(DestEncoding) >= sizeof(SourceEncoding))
		{
			// Converting from a fixed-width encoding to a wider or same encoding should always be possible,
			// as should ANSICHAR->UTF8CHAR and UCS2CHAR->UTF16CHAR,
			return true;
		}
		else
		{
			return false;
		}
	}

	/**
	 * Tests whether a particular codepoint can be converted to the destination encoding.
	 *
	 * @param Ch The character to test.
	 * @return True if Ch can be encoded as a DestEncoding.
	 */
	template <typename DestEncoding, typename SourceEncoding>
	static constexpr bool CanConvertCodepoint(SourceEncoding Codepoint)
	{
		// It is assumed that the incoming codepoint is already valid and we're only testing if it can be converted to DestEncoding.

		static_assert(TIsCharType<SourceEncoding>::Value, "Source encoding is not a char type");
		static_assert(TIsCharType<DestEncoding  >::Value, "Destination encoding is not a char type");

		// This is only defined for fixed-width encodings, because codepoints cannot be represented in a single variable-width code unit.
		static_assert(IsFixedWidthEncoding<SourceEncoding>(), "Source encoding is not fixed-width");

		if constexpr (IsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>())
		{
			// Simple conversions mean conversion is always possible
			return true;
		}
		else if constexpr (!IsFixedWidthEncoding<DestEncoding>())
		{
			// Converting all codepoints to a variable-width encoding should always be possible
			return true;
		}
		else if constexpr (std::is_same_v<DestEncoding, ANSICHAR>)
		{
			return (uint32)Codepoint <= 0x7F;
		}
		else
		{
			// The logic above should hopefully mean this branch is only taken for UTF32CHAR->UCS2CHAR.
			// There's a variety of '16-bit' char types between platforms though, so let's just test sizes.
			static_assert(sizeof(SourceEncoding) == 4 && sizeof(DestEncoding) == 2, "Unimplemented conversion");

			// Can't encode more than 16-bit in UCS-2
			return (uint32)Codepoint <= 0xFFFF;
		}
	}


	/**
	 * Returns the string representing the name of the given encoding type.
	 *
	 * @return The name of the CharType as a TCHAR string.
	 */
	template <typename Encoding>
	static const TCHAR* GetEncodingTypeName();

	static const ANSICHAR* GetEncodingName()
	{
#if PLATFORM_TCHAR_IS_4_BYTES
		return "UTF-32LE";
#else
		return "UTF-16LE";
#endif
	}

	/**
	 * True if the encoding type of the string is some form of unicode
	 */
	static constexpr bool IsUnicodeEncoded = true;


	/**
	 * Metafunction which tests whether a given character type represents a fixed-width encoding.
	 */
	template <typename T>
	struct UE_DEPRECATED(5.0, "TIsFixedWidthEncoding is deprecated, use IsFixedWidthEncoding<T>() instead.") TIsFixedWidthEncoding
	{
		enum { Value = IsFixedWidthEncoding<T>() };
	};


	/**
	 * Metafunction which tests whether two encodings are compatible.
	 *
	 * We'll say the encodings are compatible if they're both fixed-width and have the same size.  This
	 * should be good enough and catches things like UCS2CHAR and WIDECHAR being equivalent.
	 * Specializations of this template can be provided for any other special cases.
	 * Same size is a minimum requirement.
	 */
	template <typename EncodingA, typename EncodingB>
	struct UE_DEPRECATED(5.0, "TAreEncodingsCompatible is deprecated, use IsCharEncodingCompatibleWith<SrcEncoding, DestEncoding>() instead.") TAreEncodingsCompatible
	{
		enum { Value = IsFixedWidthEncoding<EncodingA>() && IsFixedWidthEncoding<EncodingB>() && sizeof(EncodingA) == sizeof(EncodingB) };
	};

	/**
	 * Converts the [Src, Src+SrcSize) string range from SourceEncoding to DestEncoding and writes it to the [Dest, Dest+DestSize) range.
	 * The Src range should contain a null terminator if a null terminator is required in the output.
	 * If the Dest range is not big enough to hold the converted output, NULL is returned.  In this case, nothing should be assumed about the contents of Dest.
	 *
	 * @param Dest      The start of the destination buffer.
	 * @param DestSize  The size of the destination buffer.
	 * @param Src       The start of the string to convert.
	 * @param SrcSize   The number of Src elements to convert.
	 * @return          A pointer to one past the last-written element.
	 */
	template <typename SourceEncoding, typename DestEncoding>
	static FORCEINLINE DestEncoding* Convert(DestEncoding* Dest, int32 DestSize, const SourceEncoding* Src, int32 SrcSize)
	{
		if constexpr (IsCharEncodingCompatibleWith<SourceEncoding, DestEncoding>())
		{
			if (DestSize < SrcSize)
			{
				return nullptr;
			}

			return (DestEncoding*)Memcpy(Dest, Src, SrcSize * sizeof(SourceEncoding)) + SrcSize;
		}
		else if constexpr (IsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>())
		{
			const int32 Size = DestSize <= SrcSize ? DestSize : SrcSize;
			for (int I = 0; I < Size; ++I)
			{
				SourceEncoding SrcCh = Src[I];
				Dest[I] = (DestEncoding)SrcCh;
			}

			return DestSize < SrcSize ? nullptr : Dest + Size;
		}
		else if constexpr (IsFixedWidthEncoding<SourceEncoding>() && IsFixedWidthEncoding<DestEncoding>())
		{
			const int32 Size = DestSize <= SrcSize ? DestSize : SrcSize;
			bool bInvalidChars = false;
			for (int I = 0; I < Size; ++I)
			{
				SourceEncoding SrcCh = Src[I];
				Dest[I] = (DestEncoding)SrcCh;
				bInvalidChars |= !CanConvertCodepoint<DestEncoding>(SrcCh);
			}

			if (bInvalidChars)
			{
				for (int I = 0; I < Size; ++I)
				{
					if (!CanConvertCodepoint<DestEncoding>(Src[I]))
					{
						Dest[I] = UNICODE_BOGUS_CHAR_CODEPOINT;
					}
				}

				LogBogusChars<DestEncoding>(Src, Size);
			}

			return DestSize < SrcSize ? nullptr : Dest + Size;
		}
		else
		{
			return UE::Core::Private::Convert(Dest, DestSize, Src, SrcSize);
		}
	}


	/**
	 * Returns the required buffer length for the [Src, Src+SrcSize) string when converted to the DestChar encoding.
	 * The Src range should contain a null terminator if a null terminator is required in the output.
	 * 
	 * @param  Src     The start of the string to convert.
	 * @param  SrcSize The number of Src elements to convert.
	 * @return         The number of DestChar elements that Src will be converted into.
	 */
	template <typename DestEncoding, typename SourceEncoding>
	static int32 ConvertedLength(const SourceEncoding* Src, int32 SrcSize)
	{
		if constexpr (IsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>() || (IsFixedWidthEncoding<SourceEncoding>() && IsFixedWidthEncoding<DestEncoding>()))
		{
			return SrcSize;
		}
		else
		{
			return UE::Core::Private::GetConvertedLength((DestEncoding*)nullptr, Src, SrcSize);
		}
	}

	CORE_API static int32 Strncmp(const ANSICHAR* String1, const ANSICHAR* String2, SIZE_T Count);
	CORE_API static int32 Strncmp(const WIDECHAR* String1, const ANSICHAR* String2, SIZE_T Count);
	CORE_API static int32 Strncmp(const UTF8CHAR* String1, const ANSICHAR* String2, SIZE_T Count);
	CORE_API static int32 Strncmp(const ANSICHAR* String1, const WIDECHAR* String2, SIZE_T Count);
	CORE_API static int32 Strncmp(const WIDECHAR* String1, const WIDECHAR* String2, SIZE_T Count);
	CORE_API static int32 Strncmp(const UTF8CHAR* String1, const WIDECHAR* String2, SIZE_T Count);
	CORE_API static int32 Strncmp(const ANSICHAR* String1, const UTF8CHAR* String2, SIZE_T Count);
	CORE_API static int32 Strncmp(const WIDECHAR* String1, const UTF8CHAR* String2, SIZE_T Count);
	CORE_API static int32 Strncmp(const UTF8CHAR* String1, const UTF8CHAR* String2, SIZE_T Count);

private:
	/**
	 * Forwarding function because we can't call FMemory::Memcpy directly due to #include ordering issues.
	 *
	 * @param Dest  The destination buffer.
	 * @param Src   The source buffer.
	 * @param Count The number of bytes to copy.
	 * @return      Dest
	 */
	static CORE_API void* Memcpy(void* Dest, const void* Src, SIZE_T Count);


	/**
	 * Logs a message about bogus characters which were detected during string conversion.
	 *
	 * @param Src     Pointer to the possibly-not-null-terminated string being converted.
	 * @param SrcSize Number of characters in the Src string.
	 */
	template <typename DestEncoding, typename SourceEncoding>
	static CORE_API void LogBogusChars(const SourceEncoding* Src, int32 SrcSize);
};
