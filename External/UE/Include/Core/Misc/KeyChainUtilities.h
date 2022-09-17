// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Misc/AES.h"
#include "Misc/IEngineCrypto.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Base64.h"
#include "Serialization/Archive.h"
#include "Serialization/JsonSerializer.h"
#include "RSA.h"

struct FNamedAESKey
{
	FString Name;
	FGuid Guid;
	FAES::FAESKey Key;

	bool IsValid() const
	{
		return Key.IsValid();
	}
};

struct FKeyChain
{
	FKeyChain() = default;

	FKeyChain(const FKeyChain& Other)
		: SigningKey(Other.SigningKey)
		, EncryptionKeys(Other.EncryptionKeys)
	{
		if (Other.MasterEncryptionKey)
		{
			MasterEncryptionKey = EncryptionKeys.Find(Other.MasterEncryptionKey->Guid);
		}
	}
	
	FKeyChain(FKeyChain&& Other)
		: SigningKey(Other.SigningKey)
		, EncryptionKeys(MoveTemp(Other.EncryptionKeys))
	{
		if (Other.MasterEncryptionKey)
		{
			MasterEncryptionKey = EncryptionKeys.Find(Other.MasterEncryptionKey->Guid);
		}
		
		Other.SigningKey = InvalidRSAKeyHandle;
		Other.MasterEncryptionKey = nullptr;
	}

	FKeyChain& operator=(const FKeyChain& Other)
	{
		SigningKey = Other.SigningKey;
		EncryptionKeys = Other.EncryptionKeys;
		
		if (Other.MasterEncryptionKey)
		{
			MasterEncryptionKey = EncryptionKeys.Find(Other.MasterEncryptionKey->Guid);
		}
		else
		{
			MasterEncryptionKey = nullptr;
		}

		return *this;
	}

	FKeyChain& operator=(FKeyChain&& Other)
	{
		SigningKey = Other.SigningKey;
		EncryptionKeys = MoveTemp(Other.EncryptionKeys);
		
		if (Other.MasterEncryptionKey)
		{
			MasterEncryptionKey = EncryptionKeys.Find(Other.MasterEncryptionKey->Guid);
		}
		else
		{
			MasterEncryptionKey = nullptr;
		}

		Other.SigningKey = InvalidRSAKeyHandle;
		Other.MasterEncryptionKey = nullptr;

		return *this;
	}

	FRSAKeyHandle SigningKey = InvalidRSAKeyHandle;
	TMap<FGuid, FNamedAESKey> EncryptionKeys;
	const FNamedAESKey* MasterEncryptionKey = nullptr;
};


namespace KeyChainUtilities
{
	static FRSAKeyHandle ParseRSAKeyFromJson(TSharedPtr<FJsonObject> InObj)
	{
		TSharedPtr<FJsonObject> PublicKey = InObj->GetObjectField(TEXT("PublicKey"));
		TSharedPtr<FJsonObject> PrivateKey = InObj->GetObjectField(TEXT("PrivateKey"));

		FString PublicExponentBase64, PrivateExponentBase64, PublicModulusBase64, PrivateModulusBase64;

		if (PublicKey->TryGetStringField("Exponent", PublicExponentBase64)
			&& PublicKey->TryGetStringField("Modulus", PublicModulusBase64)
			&& PrivateKey->TryGetStringField("Exponent", PrivateExponentBase64)
			&& PrivateKey->TryGetStringField("Modulus", PrivateModulusBase64))
		{
			check(PublicModulusBase64 == PrivateModulusBase64);

			TArray<uint8> PublicExponent, PrivateExponent, Modulus;
			FBase64::Decode(PublicExponentBase64, PublicExponent);
			FBase64::Decode(PrivateExponentBase64, PrivateExponent);
			FBase64::Decode(PublicModulusBase64, Modulus);

			return FRSA::CreateKey(PublicExponent, PrivateExponent, Modulus);
		}
		else
		{
			return nullptr;
		}
	}

	static void LoadKeyChainFromFile(const FString& InFilename, FKeyChain& OutCryptoSettings)
	{
		FArchive* File = IFileManager::Get().CreateFileReader(*InFilename);
		checkf(File != nullptr, TEXT("Specified crypto keys cache '%s' does not exist!"), *InFilename);
		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<char>> Reader = TJsonReaderFactory<char>::Create(File);
		if (FJsonSerializer::Deserialize(Reader, RootObject))
		{
			const TSharedPtr<FJsonObject>* EncryptionKeyObject;
			if (RootObject->TryGetObjectField(TEXT("EncryptionKey"), EncryptionKeyObject))
			{
				FString EncryptionKeyBase64;
				if ((*EncryptionKeyObject)->TryGetStringField(TEXT("Key"), EncryptionKeyBase64))
				{
					if (EncryptionKeyBase64.Len() > 0)
					{
						TArray<uint8> Key;
						FBase64::Decode(EncryptionKeyBase64, Key);
						check(Key.Num() == sizeof(FAES::FAESKey::Key));
						FNamedAESKey NewKey;
						NewKey.Name = TEXT("Default");
						NewKey.Guid = FGuid();
						FMemory::Memcpy(NewKey.Key.Key, &Key[0], sizeof(FAES::FAESKey::Key));
						OutCryptoSettings.EncryptionKeys.Add(NewKey.Guid, NewKey);
					}
				}
			}

			const TSharedPtr<FJsonObject>* SigningKey = nullptr;
			if (RootObject->TryGetObjectField(TEXT("SigningKey"), SigningKey))
			{
				OutCryptoSettings.SigningKey = ParseRSAKeyFromJson(*SigningKey);
			}

			const TArray<TSharedPtr<FJsonValue>>* SecondaryEncryptionKeyArray = nullptr;
			if (RootObject->TryGetArrayField(TEXT("SecondaryEncryptionKeys"), SecondaryEncryptionKeyArray))
			{
				for (TSharedPtr<FJsonValue> EncryptionKeyValue : *SecondaryEncryptionKeyArray)
				{
					FNamedAESKey NewKey;
					TSharedPtr<FJsonObject> SecondaryEncryptionKeyObject = EncryptionKeyValue->AsObject();
					FGuid::Parse(SecondaryEncryptionKeyObject->GetStringField(TEXT("Guid")), NewKey.Guid);
					NewKey.Name = SecondaryEncryptionKeyObject->GetStringField(TEXT("Name"));
					FString KeyBase64 = SecondaryEncryptionKeyObject->GetStringField(TEXT("Key"));

					TArray<uint8> Key;
					FBase64::Decode(KeyBase64, Key);
					check(Key.Num() == sizeof(FAES::FAESKey::Key));
					FMemory::Memcpy(NewKey.Key.Key, &Key[0], sizeof(FAES::FAESKey::Key));

					check(!OutCryptoSettings.EncryptionKeys.Contains(NewKey.Guid) || OutCryptoSettings.EncryptionKeys[NewKey.Guid].Key == NewKey.Key);
					OutCryptoSettings.EncryptionKeys.Add(NewKey.Guid, NewKey);
				}
			}
		}
		delete File;
		FGuid EncryptionKeyOverrideGuid;
		OutCryptoSettings.MasterEncryptionKey = OutCryptoSettings.EncryptionKeys.Find(EncryptionKeyOverrideGuid);
	}

	static void ApplyEncryptionKeys(const FKeyChain& KeyChain)
	{
		if (KeyChain.EncryptionKeys.Contains(FGuid()))
		{
			FAES::FAESKey DefaultKey = KeyChain.EncryptionKeys[FGuid()].Key;
			FCoreDelegates::GetPakEncryptionKeyDelegate().BindLambda([DefaultKey](uint8 OutKey[32]) { FMemory::Memcpy(OutKey, DefaultKey.Key, sizeof(DefaultKey.Key)); });
		}

		for (const TMap<FGuid, FNamedAESKey>::ElementType& Key : KeyChain.EncryptionKeys)
		{
			if (Key.Key.IsValid())
			{
				// Deprecated version
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				FCoreDelegates::GetRegisterEncryptionKeyDelegate().ExecuteIfBound(Key.Key, Key.Value.Key);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS

				// New version
				FCoreDelegates::GetRegisterEncryptionKeyMulticastDelegate().Broadcast(Key.Key, Key.Value.Key);
			}
		}
	}
}
