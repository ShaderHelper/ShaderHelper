// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PreHoloLensApi.h: The pre-include part of UE4's Win32 API wrapper.
=============================================================================*/

// Disable the warning that the pack size is changed in this header.
#pragma warning(disable:4103)

// Save these macros for later; Windows redefines them
#pragma push_macro("MAX_uint8")
#pragma push_macro("MAX_uint16")
#pragma push_macro("MAX_uint32")
#pragma push_macro("MAX_int32")
#pragma push_macro("TEXT")

// Undefine the TEXT macro for winnt.h to redefine it, unless it's already been included
#ifndef _WINNT_
#undef TEXT
#endif

// Disable all normal third party headers
THIRD_PARTY_INCLUDES_START
THIRD_PARTY_INCLUDES_END