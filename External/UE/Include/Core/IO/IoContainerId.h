// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FArchive;
class FStructuredArchiveSlot;

/**
 * Container ID.
 */
class FIoContainerId
{
public:
	inline FIoContainerId() = default;
	inline FIoContainerId(const FIoContainerId& Other) = default;
	inline FIoContainerId(FIoContainerId&& Other) = default;
	inline FIoContainerId& operator=(const FIoContainerId& Other) = default;

	CORE_API static FIoContainerId FromName(const FName& Name);

	uint64 Value() const
	{
		return Id;
	}

	inline bool IsValid() const
	{ 
		return Id != InvalidId;
	}

	inline bool operator<(FIoContainerId Other) const
	{
		return Id < Other.Id;
	}

	inline bool operator==(FIoContainerId Other) const
	{
		return Id == Other.Id;
	}

	inline bool operator!=(FIoContainerId Other) const
	{
		return Id != Other.Id;
	}

	inline friend uint32 GetTypeHash(const FIoContainerId& In)
	{
		return uint32(In.Id);
	}

	CORE_API friend FArchive& operator<<(FArchive& Ar, FIoContainerId& ContainerId);

	CORE_API friend void operator<<(FStructuredArchiveSlot Slot, FIoContainerId& Value);

private:
	inline explicit FIoContainerId(const uint64 InId)
		: Id(InId) { }

	static constexpr uint64 InvalidId = uint64(-1);

	uint64 Id = InvalidId;
};

