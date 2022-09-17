// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/ArrayView.h"
#include "Containers/StringFwd.h"

namespace UE::String
{

/**
 * Search the view for the first occurrence of the search string.
 *
 * @param View The string to search within.
 * @param Search The string to search for.
 * @param SearchCase Whether the comparison should ignore case.
 * @return The position at which the search string was found, or INDEX_NONE if not found.
 */
CORE_API int32 FindFirst(FAnsiStringView View, FAnsiStringView Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);
CORE_API int32 FindFirst(FWideStringView View, FWideStringView Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);

/**
 * Search the view for the last occurrence of the search string.
 *
 * @param View The string to search within.
 * @param Search The string to search for.
 * @param SearchCase Whether the comparison should ignore case.
 * @return The position at which the search string was found, or INDEX_NONE if not found.
 */
CORE_API int32 FindLast(FAnsiStringView View, FAnsiStringView Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);
CORE_API int32 FindLast(FWideStringView View, FWideStringView Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);

/**
 * Search the view for the first occurrence of any search string.
 *
 * @param View The string to search within.
 * @param Search The strings to search for.
 * @param SearchCase Whether the comparison should ignore case.
 * @return The position at which any search string was found, or INDEX_NONE if not found.
 */
CORE_API int32 FindFirstOfAny(FAnsiStringView View, TConstArrayView<FAnsiStringView> Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);
CORE_API int32 FindFirstOfAny(FWideStringView View, TConstArrayView<FWideStringView> Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);

/**
 * Search the view for the last occurrence of any search string.
 *
 * @param View The string to search within.
 * @param Search The strings to search for.
 * @param SearchCase Whether the comparison should ignore case.
 * @return The position at which any search string was found, or INDEX_NONE if not found.
 */
CORE_API int32 FindLastOfAny(FAnsiStringView View, TConstArrayView<FAnsiStringView> Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);
CORE_API int32 FindLastOfAny(FWideStringView View, TConstArrayView<FWideStringView> Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);

/**
 * Search the view for the first occurrence of the search character.
 *
 * @param View The string to search within.
 * @param Search The character to search for.
 * @param SearchCase Whether the comparison should ignore case.
 * @return The position at which the search character was found, or INDEX_NONE if not found.
 */
CORE_API int32 FindFirstChar(FAnsiStringView View, ANSICHAR Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);
CORE_API int32 FindFirstChar(FWideStringView View, WIDECHAR Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);

/**
 * Search the view for the last occurrence of the search character.
 *
 * @param View The string to search within.
 * @param Search The character to search for.
 * @param SearchCase Whether the comparison should ignore case.
 * @return The position at which the search character was found, or INDEX_NONE if not found.
 */
CORE_API int32 FindLastChar(FAnsiStringView View, ANSICHAR Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);
CORE_API int32 FindLastChar(FWideStringView View, WIDECHAR Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);

/**
 * Search the view for the first occurrence of any search character.
 *
 * @param View The string to search within.
 * @param Search The characters to search for.
 * @param SearchCase Whether the comparison should ignore case.
 * @return The position at which any search character was found, or INDEX_NONE if not found.
 */
CORE_API int32 FindFirstOfAnyChar(FAnsiStringView View, TConstArrayView<ANSICHAR> Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);
CORE_API int32 FindFirstOfAnyChar(FWideStringView View, TConstArrayView<WIDECHAR> Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);

/**
 * Search the view for the last occurrence of any search character.
 *
 * @param View The string to search within.
 * @param Search The characters to search for.
 * @param SearchCase Whether the comparison should ignore case.
 * @return The position at which any search character was found, or INDEX_NONE if not found.
 */
CORE_API int32 FindLastOfAnyChar(FAnsiStringView View, TConstArrayView<ANSICHAR> Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);
CORE_API int32 FindLastOfAnyChar(FWideStringView View, TConstArrayView<WIDECHAR> Search, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive);

} // UE::String
