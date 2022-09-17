// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/StringFwd.h"
#include "IO/IoDispatcher.h"
#include "Memory/MemoryFwd.h"
#include "Misc/EnumClassFlags.h"
#include "Templates/UniquePtr.h"

#define UE_API COREUOBJECT_API

class FArchive;
class FCompressedBuffer;
class UObject;

template <typename FuncType> class TFunctionRef;

namespace UE::DerivedData { class FBuildDefinition; }
namespace UE::DerivedData { struct FValueId; }
namespace UE::DerivedData::Private { class FEditorDerivedData; }

namespace UE
{

enum class EDerivedDataFlags : uint32
{
	None            = 0,
	Required        = 1 << 0,
	Optional        = 1 << 1,
	MemoryMapped    = 1 << 2,
};

ENUM_CLASS_FLAGS(EDerivedDataFlags);

using FDerivedDataBufferAllocator = TFunctionRef<FUniqueBuffer (uint64 Size)>;

class FDerivedData
{
public:
	inline bool HasData() const { return ChunkId != FIoChunkId::InvalidChunkId; }

	inline const FIoChunkId& GetChunkId() const { return ChunkId; }

	inline EDerivedDataFlags GetFlags() const { return Flags; }

	UE_API void Serialize(FArchive& Ar, UObject* Owner);

	UE_API static FUniqueBuffer LoadData(FArchive& Ar, FDerivedDataBufferAllocator Allocator);

#if WITH_EDITORONLY_DATA
	UE_API static void SaveData(FArchive& Ar, const FDerivedData& Data);

	UE_API FDerivedData();
	UE_API ~FDerivedData();

	UE_API FDerivedData(FDerivedData&& Other);
	UE_API FDerivedData(const FDerivedData& Other);
	UE_API FDerivedData& operator=(FDerivedData&& Other);
	UE_API FDerivedData& operator=(const FDerivedData& Other);

	UE_API explicit FDerivedData(const FSharedBuffer& Data);
	UE_API explicit FDerivedData(const FCompositeBuffer& Data);
	UE_API explicit FDerivedData(const FCompressedBuffer& Data);

	UE_API FDerivedData(FStringView CacheKey, FStringView CacheContext);

	UE_API FDerivedData(const DerivedData::FBuildDefinition& BuildDefinition, const DerivedData::FValueId& ValueId);

	UE_API void SetFlags(EDerivedDataFlags Flags);

private:
	TUniquePtr<DerivedData::Private::FEditorDerivedData> EditorData;
#endif // WITH_EDITORONLY_DATA

private:
	FIoChunkId ChunkId = FIoChunkId::InvalidChunkId;
	EDerivedDataFlags Flags = EDerivedDataFlags::Required;
};

} // UE

namespace UE::DerivedData::IoStore
{

UE_API void InitializeIoDispatcher();
UE_API void TearDownIoDispatcher();

} // UE::DerivedData::IoStore

#undef UE_API
