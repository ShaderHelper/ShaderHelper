// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Index and name number into a name map.
 */
class FMappedName
{
	static constexpr uint32 InvalidIndex = ~uint32(0);
	static constexpr uint32 IndexBits = 30u;
	static constexpr uint32 IndexMask = (1u << IndexBits) - 1u;
	static constexpr uint32 TypeMask = ~IndexMask;
	static constexpr uint32 TypeShift = IndexBits;

public:
	enum class EType
	{
		Package,
		Container,
		Global
	};

	inline FMappedName() = default;

	static inline FMappedName Create(const uint32 InIndex, const uint32 InNumber, EType InType)
	{
		check(InIndex <= MAX_int32);
		return FMappedName((uint32(InType) << TypeShift) | InIndex, InNumber);
	}

	static inline FMappedName FromMinimalName(const FMinimalName& MinimalName)
	{
		return *reinterpret_cast<const FMappedName*>(&MinimalName);
	}

	static inline bool IsResolvedToMinimalName(const FMinimalName& MinimalName)
	{
		// Not completely safe, relies on that no FName will have its Index and Number equal to Max_uint32
		const FMappedName MappedName = FromMinimalName(MinimalName);
		return MappedName.IsValid();
	}

	static inline FName SafeMinimalNameToName(const FMinimalName& MinimalName)
	{
		return IsResolvedToMinimalName(MinimalName) ? MinimalNameToName(MinimalName) : NAME_None;
	}

	inline FMinimalName ToUnresolvedMinimalName() const
	{
		return *reinterpret_cast<const FMinimalName*>(this);
	}

	inline bool IsValid() const
	{
		return Index != InvalidIndex && Number != InvalidIndex;
	}

	inline EType GetType() const
	{
		return static_cast<EType>(uint32((Index & TypeMask) >> TypeShift));
	}

	inline bool IsGlobal() const
	{
		return ((Index & TypeMask) >> TypeShift) != 0;
	}

	inline uint32 GetIndex() const
	{
		return Index & IndexMask;
	}

	inline uint32 GetNumber() const
	{
		return Number;
	}

	inline bool operator!=(FMappedName Other) const
	{
		return Index != Other.Index || Number != Other.Number;
	}

	CORE_API friend FArchive& operator<<(FArchive& Ar, FMappedName& MappedName);

private:
	inline FMappedName(const uint32 InIndex, const uint32 InNumber)
		: Index(InIndex)
		, Number(InNumber) { }

	uint32 Index = InvalidIndex;
	uint32 Number = InvalidIndex;
};

/*
 * Maps serialized name entries to names.
 */
class FNameMap
{
public:
	inline int32 Num() const
	{
		return NameEntries.Num();
	}

	CORE_API void Load(FArchive& Ar, FMappedName::EType NameMapType);

	FName GetName(const FMappedName& MappedName) const
	{
		check(MappedName.GetType() == NameMapType);
		check(MappedName.GetIndex() < uint32(NameEntries.Num()));

		FNameEntryId NameEntry = NameEntries[MappedName.GetIndex()];
		return FName::CreateFromDisplayId(NameEntry, MappedName.GetNumber());
	}

	bool TryGetName(const FMappedName& MappedName, FName& OutName) const
	{
		check(MappedName.GetType() == NameMapType);

		uint32 Index = MappedName.GetIndex();
		if (Index < uint32(NameEntries.Num()))
		{
			FNameEntryId NameEntry = NameEntries[MappedName.GetIndex()];
			OutName = FName::CreateFromDisplayId(NameEntry, MappedName.GetNumber());

			return true;
		}

		return false;
	}

	FMinimalName GetMinimalName(const FMappedName& MappedName) const
	{
		check(MappedName.GetType() == NameMapType);
		check(MappedName.GetIndex() < uint32(NameEntries.Num()));

		FNameEntryId DisplayId = NameEntries[MappedName.GetIndex()];
		FNameEntryId ComparisonId = FName::GetComparisonIdFromDisplayId(DisplayId);
		return FMinimalName(ComparisonId, MappedName.GetNumber());
	}

private:
	TArray<FNameEntryId> NameEntries;
	FMappedName::EType NameMapType = FMappedName::EType::Global;
};
