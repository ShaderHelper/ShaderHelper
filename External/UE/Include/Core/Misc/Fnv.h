// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Char.h"
#include "Misc/CString.h"


/** 
 * FNV hash generation for different types of input data
 **/
struct CORE_API FFnv
{
	/** generates FNV hash of the memory area */
	static uint32 MemFnv32( const void* Data, int32 Length, uint32 FNV=0 );
    static uint64 MemFnv64( const void* Data, int32 Length, uint64 FNV=0 );
};
