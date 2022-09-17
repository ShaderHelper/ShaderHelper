// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "UObject/NameTypes.h"
#include "Templates/Function.h"

#define ALLOW_NAME_BATCH_SAVING PLATFORM_LITTLE_ENDIAN && !PLATFORM_TCHAR_IS_4_BYTES

#if ALLOW_NAME_BATCH_SAVING
// Save comparison entries in given order to a name blob and a versioned hash blob.
CORE_API void SaveNameBatch(TArrayView<const FNameEntryId> Names, TArray<uint8>& OutNameData, TArray<uint8>& OutHashData);

// Save comparison entries in given order to an archive
CORE_API void SaveNameBatch(TArrayView<const FNameEntryId> Names, FArchive& Out);
#endif

// Reserve memory in preparation for batch loading
//
// @param Bytes for existing and new names.
CORE_API void ReserveNameBatch(uint32 NameDataBytes, uint32 HashDataBytes);

// Load a name blob with precalculated hashes.
//
// Names are rehashed if hash algorithm version doesn't match.
//
// @param NameData, HashData must be 8-byte aligned.
CORE_API void LoadNameBatch(TArray<FNameEntryId>& OutNames, TArrayView<const uint8> NameData, TArrayView<const uint8> HashData);

// Load names and precalculated hashes from an archive
//
// Names are rehashed if hash algorithm version doesn't match.
CORE_API TArray<FNameEntryId> LoadNameBatch(FArchive& Ar);

// Load names and precalculated hashes from an archive using multiple workers
//
// May load synchronously in some cases, like small batches.
//
// Names are rehashed if hash algorithm version doesn't match.
//
// @param Ar is drained synchronously
// @param MaxWorkers > 0
// @return function that waits before returning result, like a simple future.
CORE_API TFunction<TArray<FNameEntryId>()> LoadNameBatchAsync(FArchive& Ar, uint32 MaxWorkers);