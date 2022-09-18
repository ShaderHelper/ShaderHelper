// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IImageWrapper.h"
#include "CoreMinimal.h"

// http://radsite.lbl.gov/radiance/refer/Notes/picture_format.html
// http://paulbourke.net/dataformats/pic/

/** To load the HDR file image format. Does not support all possible types HDR formats (e.g. xyze is not supported) */
class IMAGEWRAPPER_API FHdrImageWrapper : public IImageWrapper
{
public:

	// Todo we should have this for all image wrapper.
	bool SetCompressedFromView(TArrayView64<const uint8> Data);

	// IIMageWrapper Interface begin
	virtual bool SetCompressed(const void* InCompressedData, int64 InCompressedSize) override;
	virtual bool SetRaw(const void* InRawData, int64 InRawSize, const int32 InWidth, const int32 InHeight, const ERGBFormat InFormat, const int32 InBitDepth, const int32 InBytesPerRow) override;
	virtual bool SetAnimationInfo(int32 InNumFrames, int32 InFramerate) override;
	virtual TArray64<uint8> GetCompressed(int32 Quality = 0) override;
	virtual bool GetRaw(const ERGBFormat InFormat, int32 InBitDepth, TArray64<uint8>& OutRawData) override;

	virtual int32 GetWidth() const override;
	virtual int32 GetHeight() const override;
	virtual int32 GetBitDepth() const override;
	virtual ERGBFormat GetFormat() const override;

	virtual int32 GetNumFrames() const override;
	virtual int32 GetFramerate() const override;
	// IImageWrapper Interface end

	const FText& GetErrorMessage() const;

	void FreeCompressedData();

	using IImageWrapper::GetRaw;
private:
	bool GetHeaderLine(const uint8*& BufferPos, char Line[256]);

	/** @param Out order in bytes: RGBE */
	bool DecompressScanline(uint8* Out, const uint8*& In);

	bool OldDecompressScanline(uint8* Out, const uint8*& In, int32 Lenght);

	bool IsCompressedImageValid() const;

	TArrayView64<const uint8> CompressedData;
	const uint8* RGBDataStart = nullptr;

	TArray64<uint8> CompressedDataHolder;

	/** INDEX_NONE if not valid */
	int32 Width = INDEX_NONE;
	/** INDEX_NONE if not valid */
	int32 Height = INDEX_NONE;

	// Reported error
	FText ErrorMessage;
};