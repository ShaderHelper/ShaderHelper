// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/StringView.h"
#include "UObject/NameTypes.h"

class UObject;
class FLinkerTables;
struct FObjectImport;

// @TODO: OBJPTR: Should this be changed to FObjectImportPathId? It is already written to target the specific patterns in import paths,
//		 if we go further in the future and do things like store classname/classpackage info, to the path segments, it would be
//		 even more specific to imports.

/**
 * FObjectPathId is a type tailored to represent object paths, with a focus on avoiding overhead for common import path patterns
 * It is primarily for use by FObjectRef/FPackedObjectRef/FObjectHandle. Most object paths used by FObjectRef will be imports,
 * and are frequently a single FName without any outer components. FObjectPathId can transform to/from most single FNames without
 * any need for a table lookup. For the less common case of an imported object path composed of more than one FName, or if the imported
 * object path has an exceedingly high number component, FObjectPathId transformations will have to do a table lookup.
 *
 * NOTE: FObjectPathId is NOT case sensitive.  Two FObjectPathIds that were constructed with the same path but of differing case
 *		 will compare as equal.  This also means that constructing an FObjectPathId from a path then calling Resolve on it may
 *		 produce the same set of names as in the original path, but of different case.  THIS WILL CAUSE NON-DETERMINISM.
 */
class FObjectPathId
{
public:
	FObjectPathId() = default;
	FObjectPathId(const FObjectPathId& Other) = default;
	COREUOBJECT_API explicit FObjectPathId(const UObject* Object);
	COREUOBJECT_API FObjectPathId(const FObjectImport& Import, const FLinkerTables& LinkerTables);

	enum EInvalid {Invalid = 0};
	explicit COREUOBJECT_API FObjectPathId(EInvalid): PathId(static_cast<uint64>(EPathId::FlagSimple))
	{
	}

	COREUOBJECT_API static FName MakeImportPathIdAndPackageName(const FObjectImport& Import, const FLinkerTables& LinkerTables, FObjectPathId& OutPathId);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// Should only be used by unit tests
	explicit FObjectPathId(FWideStringView StringPath);
	explicit FObjectPathId(FAnsiStringView StringPath);
#endif

	FObjectPathId& operator=(const FObjectPathId& Other) = default;
	inline bool operator==(const FObjectPathId& Other) const { return PathId == Other.PathId; }
	inline bool IsNone() const { return PathId == static_cast<uint64>(EPathId::None); }
	inline bool IsValid() const { return PathId != static_cast<uint64>(EPathId::FlagSimple); }

	// @TODO: OBJPTR: Is there a better interface to output the resolved name without:
	// 		 1) Requiring multiple lookups in the table in case of complex paths
	//		 2) Requiring string allocation
	//		 3) Constraining how we might change the internal storage of object paths in the future
	using ResolvedNameContainerType = TArray<FName, TInlineAllocator<3>>;
	COREUOBJECT_API void Resolve(ResolvedNameContainerType& OutContainer) const;

private:
	enum class EPathId : uint64
	{
		None = 0,
		FlagSimple = 0x01,
	};

	uint64 PathId = static_cast<uint64>(EPathId::None);

	friend uint32 GetTypeHash(FObjectPathId PathId);
};

FORCEINLINE uint32 GetTypeHash(FObjectPathId PathId)
{
	return GetTypeHash(PathId.PathId);
}
