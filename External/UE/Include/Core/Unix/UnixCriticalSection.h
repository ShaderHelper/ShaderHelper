// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/Timespan.h"
#include "HAL/PThreadCriticalSection.h"
#include "HAL/PThreadRWLock.h"

/**
	* Unix implementation of the FSystemWideCriticalSection. Uses exclusive file locking.
	**/
class FUnixSystemWideCriticalSection
{
public:
	/** Construct a named, system-wide critical section and attempt to get access/ownership of it */
	explicit CORE_API FUnixSystemWideCriticalSection(const FString& InName, FTimespan InTimeout = FTimespan::Zero());

	/** Destructor releases system-wide critical section if it is currently owned */
	CORE_API ~FUnixSystemWideCriticalSection();

	/**
	 * Does the calling thread have ownership of the system-wide critical section?
	 *
	 * @return True if the system-wide lock is obtained. WARNING: Returns true for abandoned locks so shared resources can be in undetermined states.
	 */
	CORE_API bool IsValid() const;

	/** Releases system-wide critical section if it is currently owned */
	CORE_API void Release();

private:
	FUnixSystemWideCriticalSection(const FUnixSystemWideCriticalSection&);
	FUnixSystemWideCriticalSection& operator=(const FUnixSystemWideCriticalSection&);

private:
	int32 FileHandle;
};

typedef FPThreadsCriticalSection FCriticalSection;
typedef FUnixSystemWideCriticalSection FSystemWideCriticalSection;
typedef FPThreadsRWLock FRWLock;
