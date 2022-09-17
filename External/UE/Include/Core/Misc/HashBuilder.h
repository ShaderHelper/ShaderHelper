// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Crc.h"
#include "Concepts/GetTypeHashable.h"
#include "Containers/UnrealString.h"

/**
 * Class for computing a hash of multiple types, going through GetTypeHash when the type implements it, and
 * fallbacks to CRC32 when the type doesn't.
 *
 * Note: this hash builder should be used for transient hashes, as some types implements run-dependent hash
 * computations, such as GetTypeHash(FName).
 */
class FHashBuilder
{
public:
	explicit FHashBuilder(uint32 InHash = 0)
		: Hash(~InHash)
	{}

	void AppendRaw(const void* Data, int64 Num)
	{
		Hash = FCrc::MemCrc32(Data, Num, Hash);
	}

	template <typename T>
	typename TEnableIf<TIsPODType<T>::Value, FHashBuilder&>::Type AppendRaw(const T& InData)
	{
		AppendRaw(&InData, sizeof(T), Hash);
		return *this;
	}

	template <typename T>
	typename TEnableIf<!TModels<CGetTypeHashable, T>::Value, FHashBuilder&>::Type Append(const T& InData)
	{
		return AppendRaw(InData);
	}

	template <typename T>
	typename TEnableIf<TModels<CGetTypeHashable, T>::Value, FHashBuilder&>::Type Append(const T& InData)
	{
		Hash = HashCombineFast(Hash, GetTypeHash(InData));
		return *this;
	}

	template <typename T>
	FHashBuilder& Append(const TArray<T>& InArray)
	{
		for (auto& Value: InArray)
		{
			Append(Value);
		}
		return *this;
	}

	template <typename T>
	FHashBuilder& Append(const TSet<T>& InArray)
	{
		for (auto& Value: InArray)
		{
			Append(Value);
		}
		return *this;
	}

	template <typename T>
	FHashBuilder& operator<<(const T& InData)
	{
		return Append(InData);
	}

	uint32 GetHash() const
	{
		return ~Hash;
	}

private:
	uint32 Hash;
};
