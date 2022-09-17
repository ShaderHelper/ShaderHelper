// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/StringFwd.h"
#include "Memory/MemoryView.h"
#include "Memory/SharedBuffer.h"
#include "Serialization/CompactBinary.h"

class FArchive;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Determine the size in bytes of the compact binary field at the start of the view.
 *
 * This may be called on an incomplete or invalid field, in which case the returned size is zero.
 * A size can always be extracted from a valid field with no name if a view of at least the first
 * 10 bytes is provided, regardless of field size. For fields with names, the size of view needed
 * to calculate a size is at most 10 + MaxNameLen + MeasureVarUInt(MaxNameLen).
 *
 * This function can be used when streaming a field, for example, to determine the size of buffer
 * to fill before attempting to construct a field from it.
 *
 * @param View A memory view that may contain the start of a field.
 * @param Type HasFieldType means that View contains the type. Otherwise, use the given type.
 */
CORE_API uint64 MeasureCompactBinary(FMemoryView View, ECbFieldType Type = ECbFieldType::HasFieldType);

/**
 * Try to determine the type and size of the compact binary field at the start of the view.
 *
 * This may be called on an incomplete or invalid field, in which case it will return false, with
 * OutSize being 0 for invalid fields, otherwise the minimum view size necessary to make progress
 * in measuring the field on the next call to this function.
 *
 * @note A return of true from this function does not indicate that the entire field is valid.
 *
 * @param InView A memory view that may contain the start of a field.
 * @param OutType The type (with flags) of the field. None is written until a value is available.
 * @param OutSize The total field size for a return of true, 0 for invalid fields, or the size to
 *                make progress in measuring the field on the next call to this function.
 * @param InType HasFieldType means that InView contains the type. Otherwise, use the given type.
 * @return true if the size of the field was determined, otherwise false.
 */
CORE_API bool TryMeasureCompactBinary(
	FMemoryView InView,
	ECbFieldType& OutType,
	uint64& OutSize,
	ECbFieldType InType = ECbFieldType::HasFieldType);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Load a compact binary field from an archive.
 *
 * The field may be an array or an object, which the caller can convert to by using AsArray or
 * AsObject as appropriate. The buffer allocator is called to provide the buffer for the field
 * to load into once its size has been determined.
 *
 * @param Ar Archive to read the field from. An error state is set on failure.
 * @param Allocator Allocator for the buffer that the field is loaded into.
 * @return A field with a reference to the allocated buffer, or a default field on failure.
 */
CORE_API FCbField LoadCompactBinary(FArchive& Ar, FCbBufferAllocator Allocator = FUniqueBuffer::Alloc);

/** Save a compact binary value to an archive. */
CORE_API void SaveCompactBinary(FArchive& Ar, const FCbFieldView& Field);
CORE_API void SaveCompactBinary(FArchive& Ar, const FCbArrayView& Array);
CORE_API void SaveCompactBinary(FArchive& Ar, const FCbObjectView& Object);

/** Serialize a compact binary value to/from an archive. */
CORE_API FArchive& operator<<(FArchive& Ar, FCbField& Field);
CORE_API FArchive& operator<<(FArchive& Ar, FCbArray& Array);
CORE_API FArchive& operator<<(FArchive& Ar, FCbObject& Object);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CORE_API void CompactBinaryToJson(const FCbObjectView& Object, FUtf8StringBuilderBase& Builder);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
