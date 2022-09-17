// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/TypeHash.h"
#include "Containers/StringConv.h"
#include "Containers/UnrealString.h"
#include "Serialization/StructuredArchive.h"

namespace TextKeyUtil
{
	/** Utility to produce a hash for a UTF-16 string (as used by FTextKey) */
	CORE_API uint32 HashString(const FTCHARToUTF16& InStr);
	FORCEINLINE uint32 HashString(const FTCHARToUTF16& InStr, const uint32 InBaseHash)
	{
		return HashCombine(HashString(InStr), InBaseHash);
	}

	/** Utility to produce a hash for a string (as used by FTextKey) */
	FORCEINLINE uint32 HashString(const TCHAR* InStr)
	{
		FTCHARToUTF16 UTF16String(InStr);
		return HashString(UTF16String);
	}
	FORCEINLINE uint32 HashString(const TCHAR* InStr, const uint32 InBaseHash)
	{
		FTCHARToUTF16 UTF16String(InStr);
		return HashString(UTF16String, InBaseHash);
	}

	/** Utility to produce a hash for a string (as used by FTextKey) */
	FORCEINLINE uint32 HashString(const TCHAR* InStr, const int32 InStrLen)
	{
		FTCHARToUTF16 UTF16String(InStr, InStrLen);
		return HashString(UTF16String);
	}
	FORCEINLINE uint32 HashString(const TCHAR* InStr, const int32 InStrLen, const uint32 InBaseHash)
	{
		FTCHARToUTF16 UTF16String(InStr, InStrLen);
		return HashString(UTF16String, InBaseHash);
	}

	/** Utility to produce a hash for a string (as used by FTextKey) */
	FORCEINLINE uint32 HashString(const FString& InStr)
	{
		return HashString(*InStr, InStr.Len());
	}
	FORCEINLINE uint32 HashString(const FString& InStr, const uint32 InBaseHash)
	{
		return HashString(*InStr, InStr.Len(), InBaseHash);
	}
}

/**
 * Optimized representation of a case-sensitive string, as used by localization keys.
 * This references an entry within a internal table to avoid memory duplication, as well as offering optimized comparison and hashing performance.
 */
class CORE_API FTextKey
{
public:
	FTextKey();
	FTextKey(const TCHAR* InStr);
	FTextKey(const FString& InStr);
	FTextKey(FString&& InStr);

	/** Get the underlying chars buffer this text key represents */
	FORCEINLINE const TCHAR* GetChars() const
	{
		return StrPtr;
	}

	/** Compare for equality */
	friend FORCEINLINE bool operator==(const FTextKey& A, const FTextKey& B)
	{
		return A.StrPtr == B.StrPtr;
	}

	/** Compare for inequality */
	friend FORCEINLINE bool operator!=(const FTextKey& A, const FTextKey& B)
	{
		return A.StrPtr != B.StrPtr;
	}

	/** Get the hash of this text key */
	friend FORCEINLINE uint32 GetTypeHash(const FTextKey& A)
	{
		return A.StrHash;
	}

	/** Serialize this text key as if it were an FString */
	void SerializeAsString(FArchive& Ar);

	/** Serialize this text key including its hash value (this method is sensitive to hashing algorithm changes, so only use it for generated files that can be rebuilt from another source) */
	void SerializeWithHash(FArchive& Ar);

	/** Serialize this text key including its hash value, discarding the hash on load (to upgrade from an older hashing algorithm) */
	void SerializeDiscardHash(FArchive& Ar);

	/** Serialize this text key as if it were an FString */
	void SerializeAsString(FStructuredArchiveSlot Slot);

	/** Serialize this text key including its hash value (this method is sensitive to hashing algorithm changes, so only use it for generated files that can be rebuilt from another source) */
	void SerializeWithHash(FStructuredArchiveSlot Slot);

	/** Serialize this text key including its hash value, discarding the hash on load (to upgrade from an older hashing algorithm) */
	void SerializeDiscardHash(FStructuredArchiveSlot Slot);

	/** Is this text key empty? */
	FORCEINLINE bool IsEmpty() const
	{
		return *StrPtr == 0;
	}

	/** Reset this text key to be empty */
	void Reset();

	/** Compact any slack within the internal table */
	static void CompactDataStructures();

	/** Do not use any FTextKey or FTextId after calling this */
	static void TearDown();

private:
	/** Pointer to the string buffer we reference from the internal table */
	const TCHAR* StrPtr;

	/** Hash of this text key */
	uint32 StrHash;
};

/**
 * Optimized representation of a text identity (a namespace and key pair).
 */
class CORE_API FTextId
{
public:
	FTextId() = default;

	FTextId(const FTextKey& InNamespace, const FTextKey& InKey)
		: Namespace(InNamespace)
		, Key(InKey)
	{
	}

	/** Get the namespace component of this text identity */
	FORCEINLINE const FTextKey& GetNamespace() const
	{
		return Namespace;
	}

	/** Get the key component of this text identity */
	FORCEINLINE const FTextKey& GetKey() const
	{
		return Key;
	}

	/** Compare for equality */
	friend FORCEINLINE bool operator==(const FTextId& A, const FTextId& B)
	{
		return A.Namespace == B.Namespace && A.Key == B.Key;
	}

	/** Compare for inequality */
	friend FORCEINLINE bool operator!=(const FTextId& A, const FTextId& B)
	{
		return A.Namespace != B.Namespace || A.Key != B.Key;
	}

	/** Get the hash of this text identity */
	friend FORCEINLINE uint32 GetTypeHash(const FTextId& A)
	{
		return HashCombine(GetTypeHash(A.Namespace), GetTypeHash(A.Key));
	}

	/** Serialize this text identity as if it were FStrings */
	void SerializeAsString(FArchive& Ar)
	{
		Namespace.SerializeAsString(Ar);
		Key.SerializeAsString(Ar);
	}

	/** Serialize this text identity including its hash values (this method is sensitive to hashing algorithm changes, so only use it for generated files that can be rebuilt from another source) */
	void SerializeWithHash(FArchive& Ar)
	{
		Namespace.SerializeWithHash(Ar);
		Key.SerializeWithHash(Ar);
	}

	/** Serialize this text identity including its hash values, discarding the hash on load (to upgrade from an older hashing algorithm) */
	void SerializeDiscardHash(FArchive& Ar)
	{
		Namespace.SerializeDiscardHash(Ar);
		Key.SerializeDiscardHash(Ar);
	}

	/** Serialize this text identity as if it were FStrings */
	void SerializeAsString(FStructuredArchiveSlot Slot)
	{
		FStructuredArchiveRecord Record = Slot.EnterRecord();
		Namespace.SerializeAsString(Record.EnterField(SA_FIELD_NAME(TEXT("Namespace"))));
		Key.SerializeAsString(Record.EnterField(SA_FIELD_NAME(TEXT("Key"))));
	}

	/** Serialize this text identity including its hash values (this method is sensitive to hashing algorithm changes, so only use it for generated files that can be rebuilt from another source) */
	void SerializeWithHash(FStructuredArchiveSlot Slot)
	{
		FStructuredArchiveRecord Record = Slot.EnterRecord();
		Namespace.SerializeWithHash(Record.EnterField(SA_FIELD_NAME(TEXT("Namespace"))));
		Key.SerializeWithHash(Record.EnterField(SA_FIELD_NAME(TEXT("Key"))));
	}

	/** Serialize this text identity including its hash values, discarding the hash on load (to upgrade from an older hashing algorithm) */
	void SerializeDiscardHash(FStructuredArchiveSlot Slot)
	{
		FStructuredArchiveRecord Record = Slot.EnterRecord();
		Namespace.SerializeDiscardHash(Record.EnterField(SA_FIELD_NAME(TEXT("Namespace"))));
		Key.SerializeDiscardHash(Record.EnterField(SA_FIELD_NAME(TEXT("Key"))));
	}

	/** Is this text identity empty? */
	FORCEINLINE bool IsEmpty() const
	{
		return Namespace.IsEmpty() && Key.IsEmpty();
	}

	/** Reset this text identity to be empty */
	FORCEINLINE void Reset()
	{
		Namespace.Reset();
		Key.Reset();
	}

private:
	/** Namespace component of this text identity */
	FTextKey Namespace;

	/** Key component of this text identity */
	FTextKey Key;
};
