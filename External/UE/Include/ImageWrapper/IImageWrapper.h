// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "CoreTypes.h"
#include "Templates/SharedPointer.h"


/**
 * Enumerates the types of image formats this class can handle.
 */
enum class EImageFormat : int8
{
	/** Invalid or unrecognized format. */
	Invalid = -1,

	/** Portable Network Graphics. */
	PNG = 0,

	/** Joint Photographic Experts Group. */
	JPEG,

	/** Single channel JPEG. */
	GrayscaleJPEG,	

	/** Windows Bitmap. */
	BMP,

	/** Windows Icon resource. */
	ICO,

	/** OpenEXR (HDR) image file format. */
	EXR,

	/** Mac icon. */
	ICNS,
	
	/** Truevision TGA / TARGA */
	TGA,

	/** Hdr file from radiance using RGBE */
	HDR,

	/** Tag Image File Format files */
	TIFF,
};


/**
 * Enumerates the types of RGB formats this class can handle.
 */
enum class ERGBFormat : int8
{
	Invalid = -1,

	// Red, Green, Blue and Alpha
	RGBA =  0,

	// Blue, Green, Red and Alpha
	BGRA =  1,

	// Gray scale
	Gray =  2,

	// Red, Green, Blue and Alpha using IEEE Floating-Point Arithmetic (see IEEE754). The format is always binary.
	RGBAF = 3,

	// Blue, Green, Red and Exponent (Similar to the RGBE format from radiance but with the blue and red channel inversed)
	BGRE =  4,

	// Gray scale using IEEE Floating-Point Arithmetic (see IEEE754). The format is always binary.
	GrayF = 5,
};


/**
 * Enumerates available image compression qualities.
 * 
 * JPEG interprets Quality as 1-100
 */
enum class EImageCompressionQuality : uint8
{
	Default = 0,
	Uncompressed = 1,
};


/**
 * Interface for image wrappers.
 */
class IImageWrapper
{
public:

	/**  
	 * Sets the compressed data.
	 *
	 * @param InCompressedData The memory address of the start of the compressed data.
	 * @param InCompressedSize The size of the compressed data parsed.
	 * @return true if data was the expected format.
	 */
	virtual bool SetCompressed(const void* InCompressedData, int64 InCompressedSize) = 0;

	/**  
	 * Sets the compressed data.
	 *
	 * @param InRawData The memory address of the start of the raw data.
	 * @param InRawSize The size of the compressed data parsed.
	 * @param InWidth The width of the image data.
	 * @param InHeight the height of the image data.
	 * @param InFormat the format the raw data is in, normally RGBA.
	 * @param InBitDepth the bit-depth per channel, normally 8.
	 * @param InBytesPerRow the number of bytes between rows, 0 = tightly packed rows with no padding.
	 * @return true if data was the expected format.
	 */
	virtual bool SetRaw(const void* InRawData, int64 InRawSize, const int32 InWidth, const int32 InHeight, const ERGBFormat InFormat, const int32 InBitDepth, const int32 InBytesPerRow = 0) = 0;

	/**
	 * Set information for animated formats
	 * @param InNumFrames The number of frames in the animation (the RawData from SetRaw will need to be a multiple of NumFrames)
	 * @param InFramerate The playback rate of the animation
	 * @return true if successful
	 */
	virtual bool SetAnimationInfo(int32 InNumFrames, int32 InFramerate) = 0;

	/**
	 * Gets the compressed data.
	 * (Note: It may consume the data set in the SetCompressed function if it was set before)
	 * 
	 * @return Array of the compressed data.
	 */
	virtual TArray64<uint8> GetCompressed(int32 Quality = 0) = 0;

	/**
	 * Get the raw version of the image and write to the array view
	 * (Note: It may consume the data set in the SetRaw function if it was set before)
	 *
	 * @param InFormat How we want to manipulate the RGB data.
	 * @param InBitDepth The output bit-depth per channel, normally 8.
	 * @param OutRawData Will contain the uncompressed raw data.
	 * @return true on success, false otherwise.
	 */
	virtual bool GetRaw(const ERGBFormat InFormat, int32 InBitDepth, TArrayView64<uint8> OutRawData)
	{
		// Todo We should make this function virtual pure and implement this properly in each ImageWrapper
		TArray64<uint8> TmpRawData;
		if (GetRaw(InFormat, InBitDepth, TmpRawData))
		{
			if (ensureMsgf(TmpRawData.Num() == OutRawData.Num(), TEXT("The view doesn't have the proper size to receive the texture.")))
			{
				FPlatformMemory::Memcpy(OutRawData.GetData(), TmpRawData.GetData(), OutRawData.Num());
				return true;
			}
		}

		return false;
	}

	/**  
	 * Gets the raw data.
	 * (Note: It may consume the data set in the SetRaw function if it was set before)
	 *
	 * @param InFormat How we want to manipulate the RGB data.
	 * @param InBitDepth The output bit-depth per channel, normally 8.
	 * @param OutRawData Will contain the uncompressed raw data.
	 * @return true on success, false otherwise.
	 */
	virtual bool GetRaw(const ERGBFormat InFormat, int32 InBitDepth, TArray64<uint8>& OutRawData) = 0;

	/**
	 * Gets the raw data in a TArray. Only use this if you're certain that the image is less than 2 GB in size.
	 * Prefer using the overload which takes a TArray64 in general.
	 * (Note: It may consume the data set in the SetRaw function if it was set before)
	 *
	 * @param InFormat How we want to manipulate the RGB data.
	 * @param InBitDepth The output bit-depth per channel, normally 8.
	 * @param OutRawData Will contain the uncompressed raw data.
	 * @return true on success, false otherwise.
	 */
	bool GetRaw(const ERGBFormat InFormat, int32 InBitDepth, TArray<uint8>& OutRawData)
	{
		TArray64<uint8> TmpRawData;
		if (GetRaw(InFormat, InBitDepth, TmpRawData) && ensureMsgf(TmpRawData.Num() == (int32)TmpRawData.Num(), TEXT("Tried to get %dx%d %dbpp image with format %d into 32-bit TArray (%" INT64_FMT " bytes)"), GetWidth(), GetHeight(), InBitDepth, InFormat, (long long int)TmpRawData.Num()))
		{
			OutRawData = MoveTemp(TmpRawData);
			return true;
		}
		else
		{
			return false;
		}
	}

	/**
	 * Gets the width of the image.
	 *
	 * @return Image width.
	 * @see GetHeight
	 */
	virtual int32 GetWidth() const = 0;

	/** 
	 * Gets the height of the image.
	 *
	 * @return Image height.
	 * @see GetWidth
	 */
	virtual int32 GetHeight() const = 0;

	/** 
	 * Gets the bit depth of the image.
	 *
	 * @return The bit depth per-channel of the image.
	 */
	virtual int32 GetBitDepth() const = 0;

	/** 
	 * Gets the format of the image.
	 * Theoretically, this is the format it would be best to call GetRaw() with, if you support it.
	 *
	 * @return The format the image data is in
	 */
	virtual ERGBFormat GetFormat() const = 0;

	/**
	 * @return The number of frames in an animated image
	 */
	virtual int32 GetNumFrames() const = 0;

	/**
	 * @return The playback framerate of animated images (or 0 for non-animated)
	 */
	virtual int32 GetFramerate() const = 0;

public:

	/** Virtual destructor. */
	virtual ~IImageWrapper() { }
};
