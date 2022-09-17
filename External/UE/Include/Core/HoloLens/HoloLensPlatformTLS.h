// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
HoloLensTLS.h: HoloLens platform TLS (Thread local storage and thread ID) functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformTLS.h"
#include "HoloLens/HoloLensSystemIncludes.h"

/**
* HoloLens implementation of the TLS OS functions
*/
struct CORE_API FHoloLensTLS : public FGenericPlatformTLS
{
	/**
	* Returns the currently executing thread's id
	*/
	static FORCEINLINE uint32 GetCurrentThreadId(void)
	{
		return Windows::GetCurrentThreadId();
	}

	/**
	* Allocates a thread local store slot
	*/
	static FORCEINLINE uint32 AllocTlsSlot(void)
	{
		return Windows::TlsAlloc();
	}

	/**
	* Sets a value in the specified TLS slot
	*
	* @param SlotIndex the TLS index to store it in
	* @param Value the value to store in the slot
	*/
	static FORCEINLINE void SetTlsValue(uint32 SlotIndex, void* Value)
	{
		Windows::TlsSetValue(SlotIndex, Value);
	}

	/**
	* Reads the value stored at the specified TLS slot
	*
	* @return the value stored in the slot
	*/
	static FORCEINLINE void* GetTlsValue(uint32 SlotIndex)
	{
		return Windows::TlsGetValue(SlotIndex);
	}

	/**
	* Frees a previously allocated TLS slot
	*
	* @param SlotIndex the TLS index to store it in
	*/
	static FORCEINLINE void FreeTlsSlot(uint32 SlotIndex)
	{
		Windows::TlsFree(SlotIndex);
	}
};

typedef FHoloLensTLS FPlatformTLS;
