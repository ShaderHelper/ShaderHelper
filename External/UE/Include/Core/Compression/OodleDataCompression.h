// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Misc/CompressionFlags.h"

// OodleDataCompression :
//	Unreal API for direct access to Oodle Data lossless data compression
//	for manual encoding (not in Pak/iostore)
//	NOTE : for any data that will be stored to disk, you should not be compressing it yourself!
//		allow the pak/iostore system to choose the compressor!
//
// Let me emphasize again : for data to be stored in packages for shipping games, do NOT use this
//	let the pak/iostore system choose the compression from the config options and platform settings
//
// OodleDataCompression.h is for utility compression in non-shipping game package scenarios
//	eg. uassets, storage caches and back-ends, large network transfers

DECLARE_LOG_CATEGORY_EXTERN(OodleDataCompression, Log, All);

namespace FOodleDataCompression
{

/**
 * ECompressor : Choose the Oodle Compressor
 *  this mostly trades decompression speed vs compression ratio
 * encode speed is determined by ECompressionLevel , not the compressor choice.
 * 
 * From fastest to slowest (to decode) : Selkie, Mermaid, Kraken, Leviathan
 *  
 * When in doubt, start with Kraken
 * Representative compression ratios and decode speeds :
 
Selkie4       :  1.86:1 ,   4232.6 dec MB/s
Mermaid4      :  2.21:1 ,   2648.9 dec MB/s
Kraken4       :  2.44:1 ,   1467.1 dec MB/s
Leviathan4    :  2.62:1 ,    961.8 dec MB/s
 * 
 */
//UENUM() // @todo Oodle might be nice if these were UENUM but can't pull UObject from inside Core?
// enum values should not change, they may be persisted
enum class ECompressor : uint8
{
	NotSet = 0,
	Selkie = 1,
	Mermaid = 2,
	Kraken  = 3,
	Leviathan = 4 // if another added update CompressorNameMap
};

// If the input is invalid, returns false, and doesn't touch output.
bool CORE_API ECompressorToString(ECompressor InCompressor, const TCHAR** OutName);
bool CORE_API ECompressorFromString(const class FString& InName, ECompressor& OutCompressor);

/**
 * ECompressionLevel : Choose the Oodle Compression Level
 *  this mostly trades encode speed vs compression ratio
 *  decode speed is determined by choice of compressor (ECompressor)
 * 
 * If in doubt start with "Normal" (level 4) then move up or down from there
 * 
 * The standard range is in "SuperFast" - "Normal" (levels 1-4)
 * 
 * "HyperFast" levels are for real-time encoding with minimal compression
 * 
 * The "Optimal" levels are much slower to encode, but provide more compression
 * they are intended for offline cooks
 * 
 * representative encode speeds with compression ratio trade off :
 
Kraken-4      :  1.55:1 ,  718.3 enc MB/s
Kraken-3      :  1.71:1 ,  541.8 enc MB/s
Kraken-2      :  1.88:1 ,  434.0 enc MB/s
Kraken-1      :  2.10:1 ,  369.1 enc MB/s
Kraken1       :  2.27:1 ,  242.6 enc MB/s
Kraken2       :  2.32:1 ,  157.4 enc MB/s
Kraken3       :  2.39:1 ,   34.8 enc MB/s
Kraken4       :  2.44:1 ,   22.4 enc MB/s
Kraken5       :  2.55:1 ,   10.1 enc MB/s
Kraken6       :  2.56:1 ,    5.4 enc MB/s
Kraken7       :  2.64:1 ,    3.7 enc MB/s

 */
//UENUM() // @todo Oodle might be nice if these were UENUM but can't pull UObject from inside Core?
// ECompressionLevel must numerically match the Oodle internal enum values
enum class ECompressionLevel : int8
{
	HyperFast4 = -4,
	HyperFast3 = -3,
	HyperFast2 = -2,
	HyperFast1 = -1,
	None = 0,
	SuperFast = 1,
	VeryFast = 2,
	Fast = 3,
	Normal = 4,
	Optimal1 = 5,
	Optimal2 = 6,
	Optimal3 = 7,
	Optimal4 = 8, // if another added update CompressionLevelNameMap
};

// If the input is invalid, returns false, and doesn't touch output.
bool CORE_API ECompressionLevelFromValue(int8 InValue, ECompressionLevel& OutLevel);
bool CORE_API ECompressionLevelToString(ECompressionLevel InLevel, const TCHAR** OutName);

enum class ECompressionCommonUsage : uint8
{
	Default = 0,
	FastRealtimeEncode = 1,
	SlowerSmallerEncode = 2,
	SlowestOfflineDistributionEncode = 3
};

/**
* Translate legacy CompressionFlags to an ECompressionCommonUsage
*   CompressionFlags is not encouraged for new code; prefer directly calling to Oodle
*
* @param	Flags				ECompressionFlags (BiasSpeed,BiasSize,ForPackaging)
* @return						ECompressionCommonUsage
*/
ECompressionCommonUsage CORE_API GetCommonUsageFromLegacyCompressionFlags(ECompressionFlags Flags);

/**
* Translate CompressionCommonUsage to a Compressor & Level selection
*   usually prefer the more expressive choice of {Compressor,Level}
*
* @param	Usage				Your intended compression use case
* @param	OutCompressor		Output reference
* @param	OutLevel			Output reference
* @return						
*/
void CORE_API GetCompressorAndLevelForCommonUsage(ECompressionCommonUsage Usage,ECompressor & OutCompressor,ECompressionLevel & OutLevel);


/**
* What size of compressed output buffer is needed to encode into for Compress()
*
* @param	UncompressedSize			Length of uncompressed data that will fit in this output buffer
* @return								Minimum size to allocate compressed output buffer
*/
int64 CORE_API CompressedBufferSizeNeeded(int64 UncompressedSize);

/**
* What is the maximum size of compressed data made after encoding
*  For pre-allocating buffers at decode time or to check the length of compressed data at load time
*  NOTE : CompressedBufferSizeNeeded() >= GetMaximumCompressedSize()
* this is not the size to allocate compressed buffers at encode time; see CompressedBufferSizeNeeded
*
* @param	UncompressedSize			Length of uncompressed data that could have been encoded
* @return								Maximum size of compressed data that Compress() could have made
*/
int64 CORE_API GetMaximumCompressedSize(int64 UncompressedSize);

/**
* Encode provided data with chosen Compressor and Level
* CompressedBufferSize must be >= CompressedBufferSizeNeeded(UncompressedSize)
*
* @param	OutCompressedData			output buffer where compressed data is written
* @param	CompressedBufferSize		bytes available to write in OutCompressedData
* @param	InUncompressedData			input buffer containing data to compress
* @param	UncompressedSize			number of bytes in InUncompressedData to read
* @param	Compressor					ECompressor to encode with (this is saved in the stream)
* @param	Level						ECompressionLevel to encode with (this is not saved in the stream)
* @return								Compressed size written or zero for failure
*/
int64 CORE_API Compress(
							void * OutCompressedData, int64 CompressedBufferSize,
							const void * InUncompressedData, int64 UncompressedSize,
							ECompressor Compressor,
							ECompressionLevel Level);

/**
* Decode compressed data that was made by Compress
*
* UncompressedSize must match exactly the uncompressed size that was used at encode time.  No partial decodes.
*
* @param	OutUncompressedData		output buffer where uncompressed data is written
* @param	UncompressedSize		number of bytes to decompress
* @param	InCompressedData		input buffer containing compressed data
* @param	CompressedSize			size of the input buffer, must be greater or equal to the number of compressed bytes needed
* @return							boolean success
*/
bool CORE_API Decompress(
						void * OutUncompressedData, int64 UncompressedSize,
						const void * InCompressedData, int64 CompressedSize
						);


// For Compression to/from TArray and such higher level actions use OodleDataCompressionUtil.h

// from Compression.cpp :
void CORE_API CompressionFormatInitOnFirstUseFromLock();

// from LaunchEngineLoop :
void CORE_API StartupPreInit();

};
