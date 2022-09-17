// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Features/IModularFeatures.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Misc/CompressionFlags.h"


#define COMPRESSION_FORMAT_FEATURE_NAME "CompressionFormat"

struct ICompressionFormat : public IModularFeature, public IModuleInterface
{
	virtual FName GetCompressionFormatName() = 0;
	virtual bool Compress(void* CompressedBuffer, int32& CompressedSize, const void* UncompressedBuffer, int32 UncompressedSize, int32 CompressionData, ECompressionFlags Flags) = 0;
	virtual bool Uncompress(void* UncompressedBuffer, int32& UncompressedSize, const void* CompressedBuffer, int32 CompressedSize, int32 CompressionData) = 0;
	virtual int32 GetCompressedBufferSize(int32 UncompressedSize, int32 CompressionData) = 0;
	virtual uint32 GetVersion() = 0;
	virtual FString GetDDCKeySuffix() = 0;
	virtual bool DoesOwnWorthDecompressingCheck() = 0;
};
