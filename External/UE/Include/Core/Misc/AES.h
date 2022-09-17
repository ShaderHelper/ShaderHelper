// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "HAL/UnrealMemory.h"

#define AES_BLOCK_SIZE 16


struct CORE_API FAES
{
	static constexpr uint32 AESBlockSize = 16;

	/** 
	 * Class representing a 256 bit AES key
	 */
	struct FAESKey
	{
		static constexpr int32 KeySize = 32;

		uint8 Key[KeySize];

		FAESKey()
		{
			Reset();
		}

		bool IsValid() const
		{
			uint32* Words = (uint32*)Key;
			for (int32 Index = 0; Index < KeySize / 4; ++Index)
			{
				if (Words[Index] != 0)
				{
					return true;
				}
			}
			return false;
		}

		void Reset()
		{
			FMemory::Memset(Key, 0, KeySize);
		}

		bool operator == (const FAESKey& Other) const
		{
			return FMemory::Memcmp(Key, Other.Key, KeySize) == 0;
		}
	};

	/**
	* Encrypts a chunk of data using a specific key
	*
	* @param Contents the buffer to encrypt
	* @param NumBytes the size of the buffer
	* @param Key An FAESKey object containing the encryption key
	*/
	static void EncryptData(uint8* Contents, uint32 NumBytes, const FAESKey& Key);

	/**
	 * Encrypts a chunk of data using a specific key
	 *
	 * @param Contents the buffer to encrypt
	 * @param NumBytes the size of the buffer
	 * @param Key a null terminated string that is a 32 byte multiple length
	 */
	static void EncryptData(uint8* Contents, uint32 NumBytes, const ANSICHAR* Key);

	/**
	* Encrypts a chunk of data using a specific key
	*
	* @param Contents the buffer to encrypt
	* @param NumBytes the size of the buffer
	* @param Key a byte array that is a 32 byte multiple length
	*/
	static void EncryptData(uint8* Contents, uint32 NumBytes, const uint8* KeyBytes, uint32 NumKeyBytes);

	/**
	* Decrypts a chunk of data using a specific key
	*
	* @param Contents the buffer to encrypt
	* @param NumBytes the size of the buffer
	* @param Key a null terminated string that is a 32 byte multiple length
	*/
	static void DecryptData(uint8* Contents, uint32 NumBytes, const FAESKey& Key);

	/**
	 * Decrypts a chunk of data using a specific key
	 *
	 * @param Contents the buffer to encrypt
	 * @param NumBytes the size of the buffer
	 * @param Key a null terminated string that is a 32 byte multiple length
	 */
	static void DecryptData(uint8* Contents, uint32 NumBytes, const ANSICHAR* Key);

	/**
	* Decrypts a chunk of data using a specific key
	*
	* @param Contents the buffer to encrypt
	* @param NumBytes the size of the buffer
	* @param Key a byte array that is a 32 byte multiple length
	*/
	static void DecryptData(uint8* Contents, uint32 NumBytes, const uint8* KeyBytes, uint32 NumKeyBytes);
};
