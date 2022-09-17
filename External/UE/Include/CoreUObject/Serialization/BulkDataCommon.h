// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/NumericLimits.h"

// NOTE: This file only needs to exist as long as we need to maintain the editor and runtime versions of Bulkdata.
// Code common to both can be placed here.

/**
 * Flags serialized with the bulk data.
 */
enum EBulkDataFlags : uint32
{
	/** Empty flag set. */
	BULKDATA_None = 0,
	/**
	 * INTERNAL SET ONLY - callers of bulkdata should not set this flag on the bulk data
	 * It is overwritten according to global configuration by Serialize.
	 * If set, payload is stored not inline; it is stored either at the end of the file
	 * or in a separate file.
	 */
	BULKDATA_PayloadAtEndOfFile = 1 << 0,
	/** If set, payload should be [un]compressed using ZLIB during serialization. */
	BULKDATA_SerializeCompressedZLIB = 1 << 1,
	/** Force usage of SerializeElement over bulk serialization. */
	BULKDATA_ForceSingleElementSerialization = 1 << 2,
	/** Bulk data is only used once at runtime in the game. */
	BULKDATA_SingleUse = 1 << 3,
	/** Bulk data won't be used and doesn't need to be loaded. */
	BULKDATA_Unused = 1 << 5,
	/** Forces the payload to be saved inline, regardless of its size. */
	BULKDATA_ForceInlinePayload = 1 << 6,
	/** Flag to check if either compression mode is specified. */
	BULKDATA_SerializeCompressed = (BULKDATA_SerializeCompressedZLIB),
	/** Forces the payload to be always streamed, regardless of its size. */
	BULKDATA_ForceStreamPayload = 1 << 7,
	/**
	 * INTERNAL SET ONLY - callers of bulkdata should not set this flag on the bulk data
	 * It is overwritten according to global configuration by Serialize.
	 * If set, payload is stored in a separate file such as .ubulk.
	 * */
	BULKDATA_PayloadInSeperateFile = 1 << 8,
	/** DEPRECATED: If set, payload is compressed using platform specific bit window. */
	BULKDATA_SerializeCompressedBitWindow = 1 << 9,
	/** There is a new default to inline unless you opt out. */
	BULKDATA_Force_NOT_InlinePayload = 1 << 10,
	/** This payload is optional and may not be on device. */
	BULKDATA_OptionalPayload = 1 << 11,
	/** This payload will be memory mapped, this requires alignment, no compression etc. */
	BULKDATA_MemoryMappedPayload = 1 << 12,
	/** Bulk data size is 64 bits long. */
	BULKDATA_Size64Bit = 1 << 13,
	/** Duplicate non-optional payload in optional bulk data. */
	BULKDATA_DuplicateNonOptionalPayload = 1 << 14,
	/** Indicates that an old ID is present in the data, at some point when the DDCs are flushed we can remove this. */
	BULKDATA_BadDataVersion = 1 << 15,
	/** BulkData did not have it's offset changed during the cook and does not need the fix up at load time */
	BULKDATA_NoOffsetFixUp = 1 << 16,
	/**
	 * INTERNAL SET ONLY - callers of bulkdata should not set this flag on the bulk data
	 * If set, payload is stored in the workspace domain version of the file.
	 */
	BULKDATA_WorkspaceDomainPayload = 1 << 17,
	/**
	 * INTERNAL SET ONLY - callers of bulkdata should not set this flag on the bulk data
	 * If true, the BulkData can be loaded from its file at any time
	 */
	BULKDATA_LazyLoadable = 1 << 18,

	/* Runtime only flags below this point! Note that they take the high bits in reverse order! */

	/** Assigned at runtime to indicate that the BulkData should be using the IoDispatcher when loading, not filepaths. */
	BULKDATA_UsesIoDispatcher = 1u << 31u,
	/** Assigned at runtime to indicate that the BulkData allocation is a memory mapped region of a file and not raw data. */
	BULKDATA_DataIsMemoryMapped = 1 << 30,
	/** Assigned at runtime to indicate that the BulkData object has an async loading request in flight and will need to wait on it. */
	BULKDATA_HasAsyncReadPending = 1 << 29,
	/** Assigned at runtime to indicate that the BulkData object should be considered for discard even if it cannot load from disk. */
	BULKDATA_AlwaysAllowDiscard = 1 << 28,
};

/**
 * Allows FArchive to serialize EBulkDataFlags, this will not be required once EBulkDataFlags is promoted
 * to be a enum class.
 */
FORCEINLINE FArchive& operator<<(FArchive& Ar, EBulkDataFlags& Flags)
{
	Ar << (uint32&)Flags;
	return Ar;
}

/** Serialize the given Value as an int32 or int64 depending on InBulkDataFlags&BULKDATA_Size64Bit. */
inline void SerializeBulkDataSizeInt(FArchive& Ar, int64& Value, EBulkDataFlags InBulkDataFlags)
{
	if (InBulkDataFlags & BULKDATA_Size64Bit)
	{
		Ar << Value;
	}
	else
	{
		check(!Ar.IsSaving() || (MIN_int32 <= Value && Value <= MAX_int32));
		int32 ValueAsInt32 = static_cast<int32>(Value);
		Ar << ValueAsInt32;
		Value = ValueAsInt32;
	}
}

/**
 * Enumeration for bulk data lock status.
 */
enum EBulkDataLockStatus
{
	/** Unlocked array				*/
	LOCKSTATUS_Unlocked = 0,
	/** Locked read-only			*/
	LOCKSTATUS_ReadOnlyLock = 1,
	/** Locked read-write-realloc	*/
	LOCKSTATUS_ReadWriteLock = 2,
};

/**
 * Enumeration for bulk data lock behavior
 */
enum EBulkDataLockFlags
{
	LOCK_READ_ONLY = 1,
	LOCK_READ_WRITE = 2,
};

namespace BulkDataExt
{
	extern const FString Export;		// Stored in the export data
	extern const FString Default;		// Stored in a separate file
	extern const FString MemoryMapped;	// Stored in a separate file aligned for memory mapping
	extern const FString Optional;		// Stored in a separate file that is optional
}
