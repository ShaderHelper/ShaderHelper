// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IImageWrapper.h"

class IMAGEWRAPPER_API ImageWrapperHelper
{
public:
	 static FStringView GetFormatExtension(EImageFormat InImageFormat, bool bIncludeDot=false);
	 static EImageFormat GetImageFormat(FStringView StringExtention);
	 static const FStringView GetImageFilesFilterString(bool bIncludeAllFiles);
};
