// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "Misc/Timespan.h"

/**
 * Class for encoding/decoding Base64 data (RFC 4648)
 */
struct CORE_API FBase64
{
	/**
	 * Encodes a FString into a Base64 string
	 *
	 * @param Source The string data to convert
	 *
	 * @return A string that encodes the binary data in a way that can be safely transmitted via various Internet protocols
	 */
	static FString Encode(const FString& Source);

	/**
	 * Encodes a binary uint8 array into a Base64 string
	 *
	 * @param Source The binary data to convert
	 *
	 * @return A string that encodes the binary data in a way that can be safely transmitted via various Internet protocols
	 */
	static FString Encode(const TArray<uint8>& Source);

	/**
	 * Encodes the source into a Base64 string
	 *
	 * @param Source The binary data to encode
	 * @param Length Length of the binary data to be encoded
	 *
	 * @return Base64 encoded string containing the binary data.
	 */
	static FString Encode(const uint8* Source, uint32 Length);

	/**
	 * Encodes the source into a Base64 string, storing it in a preallocated buffer.
	 *
	 * @param Source The binary data to encode
	 * @param Length Length of the binary data to be encoded
	 * @param Dest Buffer to receive the encoded data. Must be large enough to contain the entire output data (see GetEncodedDataSize()).
	 *
	 * @return The length of the encoded data
	 */
	template<typename CharType> static uint32 Encode(const uint8* Source, uint32 Length, CharType* Dest);

	/**
	* Get the encoded data size for the given number of bytes.
	*
	* @param NumBytes The number of bytes of input
	*
	* @return The number of characters in the encoded data.
	*/
	static inline constexpr uint32 GetEncodedDataSize(uint32 NumBytes)
	{
		return ((NumBytes + 2) / 3) * 4;
	}

	/**
	 * Decodes a Base64 string into a FString
	 *
	 * @param Source The Base64 encoded string
	 * @param OutDest Receives the decoded string data
	 */
	static bool Decode(const FString& Source, FString& OutDest);

	/**
	 * Decodes a Base64 string into an array of bytes
	 *
	 * @param Source The Base64 encoded string
	 * @param Dest Array to receive the decoded data
	 */
	static bool Decode(const FString& Source, TArray<uint8>& Dest);

	/**
	 * Decodes a Base64 string into a preallocated buffer
	 *
	 * @param Source The Base64 encoded string
	 * @param Length Length of the Base64 encoded string
	 * @param Dest Buffer to receive the decoded data
	 *
	 * @return true if the buffer was decoded, false if it was invalid.
	 */
	template<typename CharType> static bool Decode(const CharType* Source, uint32 Length, uint8* Dest);

	/**
	* Determine the decoded data size for the incoming base64 encoded string
	*
	* @param Source The Base64 encoded string
	*
	* @return The size in bytes of the decoded data
	*/
	static uint32 GetDecodedDataSize(const FString& Source);

	/**
	* Determine the decoded data size for the incoming base64 encoded string
	*
	* @param Source The Base64 encoded string
	* @param Length Length of the Base64 encoded string
	*
	* @return The size in bytes of the decoded data
	*/
	template<typename CharType> static uint32 GetDecodedDataSize(const CharType* Source, uint32 Length);

	/**
	* Get the maximum decoded data size for the given number of input characters.
	*
	* @param Length The number of input characters.
	*
	* @return The maximum number of bytes that can be decoded from this input stream. The actual number of characters decoded may be less if the data contains padding characters. Call GetDecodedDataSize() with the input data to find the exact length.
	*/
	static inline constexpr uint32 GetMaxDecodedDataSize(uint32 NumChars)
	{
		return ((NumChars * 3) + 3) / 4;
	}
};
