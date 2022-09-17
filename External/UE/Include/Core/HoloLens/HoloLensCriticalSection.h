// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/Timespan.h"
#include "HAL/PlatformMemory.h"
#include "GenericPlatform/GenericPlatformCriticalSection.h"

/**
 * This is the HoloLens version of a critical section. It uses an aggregate
 * CRITICAL_SECTION to implement its locking.
 */
class FHoloLensCriticalSection
{
	/**
	 * The HoloLens specific critical section
	 */
	Windows::CRITICAL_SECTION CriticalSection;

public:

	/**
	 * Constructor that initializes the aggregated critical section
	 */
	FORCEINLINE FHoloLensCriticalSection()
	{
		CA_SUPPRESS(28125);
		Windows::InitializeCriticalSection(&CriticalSection);
		Windows::SetCriticalSectionSpinCount(&CriticalSection, 4000);
	}

	/**
	 * Destructor cleaning up the critical section
	 */
	FORCEINLINE ~FHoloLensCriticalSection()
	{
		Windows::DeleteCriticalSection(&CriticalSection);
	}

	/**
	 * Locks the critical section
	 */
	FORCEINLINE void Lock()
	{
		// Spin first before entering critical section, causing ring-0 transition and context switch.
		if( Windows::TryEnterCriticalSection(&CriticalSection) == 0 )
		{
			Windows::EnterCriticalSection(&CriticalSection);
		}
	}

	/**
	 * Attempt to take a lock and returns whether or not a lock was taken.
	 *
	 * @return true if a lock was taken, false otherwise.
	*/
	FORCEINLINE bool TryLock()
	{
		if (Windows::TryEnterCriticalSection(&CriticalSection))
		{
			return true;
		};
		return false;
	}

	/**
	 * Releases the lock on the critical seciton
	 */
	FORCEINLINE void Unlock()
	{
		Windows::LeaveCriticalSection(&CriticalSection);
	}
};

/**
 * FHoloLensRWLock - Read/Write Mutex
 *	- Provides non-recursive Read/Write (or shared-exclusive) access.
 *	- Windows specific lock structures/calls Ref: https://msdn.microsoft.com/en-us/library/windows/desktop/aa904937(v=vs.85).aspx
 */
class FHoloLensRWLock
{
public:
	FHoloLensRWLock(const FHoloLensRWLock&) = delete;
	FHoloLensRWLock& operator=(const FHoloLensRWLock&) = delete;

	FORCEINLINE FHoloLensRWLock(uint32 Level = 0)
	{
		Windows::InitializeSRWLock(&Mutex);
	}
	
	FORCEINLINE ~FHoloLensRWLock()
	{
	}
	
	FORCEINLINE void ReadLock()
	{
		Windows::AcquireSRWLockShared(&Mutex);
	}
	
	FORCEINLINE void WriteLock()
	{
		Windows::AcquireSRWLockExclusive(&Mutex);
	}
	
	FORCEINLINE void ReadUnlock()
	{
		Windows::ReleaseSRWLockShared(&Mutex);
	}
	
	FORCEINLINE void WriteUnlock()
	{
		Windows::ReleaseSRWLockExclusive(&Mutex);
	}
	
private:
	Windows::SRWLOCK Mutex;
};


typedef FHoloLensCriticalSection FCriticalSection;
typedef FSystemWideCriticalSectionNotImplemented FSystemWideCriticalSection;
typedef FHoloLensRWLock FRWLock;
