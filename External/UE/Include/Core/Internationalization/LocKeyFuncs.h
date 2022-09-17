// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Containers/Set.h"
#include "Containers/Map.h"

/** Case sensitive immutable hashed string used as a localization key */
class FLocKey
{
public:
	FLocKey()
		: Str()
		, Hash(0)
	{
	}

	FLocKey(const TCHAR* InStr)
		: Str(InStr)
		, Hash(ProduceHash(Str))
	{
	}

	FLocKey(const FString& InStr)
		: Str(InStr)
		, Hash(ProduceHash(Str))
	{
	}

	FLocKey(FString&& InStr)
		: Str(MoveTemp(InStr))
		, Hash(ProduceHash(Str))
	{
	}

	FLocKey(const FLocKey& InOther)
		: Str(InOther.Str)
		, Hash(InOther.Hash)
	{
	}

	FLocKey(FLocKey&& InOther)
		: Str(MoveTemp(InOther.Str))
		, Hash(InOther.Hash)
	{
		InOther.Hash = 0;
	}

	FLocKey& operator=(const FLocKey& InOther)
	{
		if (this != &InOther)
		{
			Str = InOther.Str;
			Hash = InOther.Hash;
		}

		return *this;
	}

	FLocKey& operator=(FLocKey&& InOther)
	{
		if (this != &InOther)
		{
			Str = MoveTemp(InOther.Str);
			Hash = InOther.Hash;

			InOther.Hash = 0;
		}

		return *this;
	}

	FORCEINLINE bool operator==(const FLocKey& Other) const
	{
		return Hash == Other.Hash && Compare(Other) == 0;
	}

	FORCEINLINE bool operator!=(const FLocKey& Other) const
	{
		return Hash != Other.Hash || Compare(Other) != 0;
	}

	FORCEINLINE bool operator<(const FLocKey& Other) const
	{
		return Compare(Other) < 0;
	}

	FORCEINLINE bool operator<=(const FLocKey& Other) const
	{
		return Compare(Other) <= 0;
	}

	FORCEINLINE bool operator>(const FLocKey& Other) const
	{
		return Compare(Other) > 0;
	}

	FORCEINLINE bool operator>=(const FLocKey& Other) const
	{
		return Compare(Other) >= 0;
	}

	friend inline uint32 GetTypeHash(const FLocKey& Id)
	{
		return Id.Hash;
	}

	FORCEINLINE bool IsEmpty() const
	{
		return Str.IsEmpty();
	}

	FORCEINLINE bool Equals(const FLocKey& Other) const
	{
		return *this == Other;
	}

	FORCEINLINE int32 Compare(const FLocKey& Other) const
	{
		return FCString::Strcmp(*Str, *Other.Str);
	}

	FORCEINLINE const FString& GetString() const
	{
		return Str;
	}

	static FORCEINLINE uint32 ProduceHash(const FString& InStr, const uint32 InBaseHash = 0)
	{
		return FCrc::StrCrc32<TCHAR>(*InStr, InBaseHash);
	}

private:
	/** String representation of this LocKey */
	FString Str;

	/** Hash representation of this LocKey */
	uint32 Hash;
};

/** Case sensitive hashing function for TSet */
struct FLocKeySetFuncs : BaseKeyFuncs<FString, FString>
{
	static FORCEINLINE const FString& GetSetKey(const FString& Element)
	{
		return Element;
	}
	static FORCEINLINE bool Matches(const FString& A, const FString& B)
	{
		return A.Equals(B, ESearchCase::CaseSensitive);
	}
	static FORCEINLINE uint32 GetKeyHash(const FString& Key)
	{
		return FLocKey::ProduceHash(Key);
	}
};

/** Case sensitive hashing function for TMap */
template <typename ValueType>
struct FLocKeyMapFuncs : BaseKeyFuncs<ValueType, FString, /*bInAllowDuplicateKeys*/false>
{
	static FORCEINLINE const FString& GetSetKey(const TPair<FString, ValueType>& Element)
	{
		return Element.Key;
	}
	static FORCEINLINE bool Matches(const FString& A, const FString& B)
	{
		return A.Equals(B, ESearchCase::CaseSensitive);
	}
	static FORCEINLINE uint32 GetKeyHash(const FString& Key)
	{
		return FLocKey::ProduceHash(Key);
	}
};

/** Case sensitive hashing function for TMultiMap */
template <typename ValueType>
struct FLocKeyMultiMapFuncs : BaseKeyFuncs<ValueType, FString, /*bInAllowDuplicateKeys*/true>
{
	static FORCEINLINE const FString& GetSetKey(const TPair<FString, ValueType>& Element)
	{
		return Element.Key;
	}
	static FORCEINLINE bool Matches(const FString& A, const FString& B)
	{
		return A.Equals(B, ESearchCase::CaseSensitive);
	}
	static FORCEINLINE uint32 GetKeyHash(const FString& Key)
	{
		return FLocKey::ProduceHash(Key);
	}
};
