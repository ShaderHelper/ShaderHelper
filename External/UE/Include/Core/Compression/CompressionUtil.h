// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

class FArchive;
class FCustomVersionContainer;
class FLinker;
class FName;

namespace FCompressionUtil
{


	void CORE_API SerializeCompressorName(FArchive & Archive,FName & Compressor);


};