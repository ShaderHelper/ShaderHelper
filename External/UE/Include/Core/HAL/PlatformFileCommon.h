// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/ScopeLock.h"
#include "HAL/DiskUtilizationTracker.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/ScopeLock.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformProcess.h"

#define MANAGE_FILE_HANDLES (#) // this is not longer used, this will error on any attempt to use it

class FRegisteredFileHandle : public IFileHandle
{
public:
	FRegisteredFileHandle()
		: NextLink(nullptr)
		, PreviousLink(nullptr)
		, bIsOpenAndAvailableForClosing(false)
	{
	}

private:
	friend class FFileHandleRegistry;

	FRegisteredFileHandle* NextLink;
	FRegisteredFileHandle* PreviousLink;
	bool bIsOpenAndAvailableForClosing;
};


class FFileHandleRegistry
{
public:
	FFileHandleRegistry(int32 InMaxOpenHandles)
		: MaxOpenHandles(InMaxOpenHandles)
		, OpenAndAvailableForClosingHead(nullptr)
		, OpenAndAvailableForClosingTail(nullptr)
	{
	}

	FRegisteredFileHandle* InitialOpenFile(const TCHAR* Filename)
	{
		if (HandlesCurrentlyInUse.Increment() > MaxOpenHandles)
		{
			FreeHandles();
		}

		FRegisteredFileHandle* Handle = PlatformInitialOpenFile(Filename);
		if (Handle != nullptr)
		{
			FScopeLock Lock(&LockSection);
			LinkToTail(Handle);
		}
		else
		{
			HandlesCurrentlyInUse.Decrement();
		}

		return Handle;
	}

	void UnTrackAndCloseFile(FRegisteredFileHandle* Handle)
	{
		bool bWasOpen = false;
		{
			FScopeLock Lock(&LockSection);
			if (Handle->bIsOpenAndAvailableForClosing)
			{
				UnLink(Handle);
				bWasOpen = true;
			}
		}
		if (bWasOpen)
		{
			PlatformCloseFile(Handle);
			HandlesCurrentlyInUse.Decrement();
		}
	}

	void TrackStartRead(FRegisteredFileHandle* Handle)
	{
		{
			FScopeLock Lock(&LockSection);

			if (Handle->bIsOpenAndAvailableForClosing)
			{
				UnLink(Handle);
				return;
			}
		}

		if (HandlesCurrentlyInUse.Increment() > MaxOpenHandles)
		{
			FreeHandles();
		}
		// can do this out of the lock, in case it's slow
		PlatformReopenFile(Handle);
	}

	void TrackEndRead(FRegisteredFileHandle* Handle)
	{
		{
			FScopeLock Lock(&LockSection);
			LinkToTail(Handle);
		}
	}

protected:
	virtual FRegisteredFileHandle* PlatformInitialOpenFile(const TCHAR* Filename) = 0;
	virtual bool PlatformReopenFile(FRegisteredFileHandle*) = 0;
	virtual void PlatformCloseFile(FRegisteredFileHandle*) = 0;

private:

	void FreeHandles()
	{
		// do we need to make room for a file handle? this assumes that the critical section is already locked
		while (HandlesCurrentlyInUse.GetValue() > MaxOpenHandles)
		{
			FRegisteredFileHandle* ToBeClosed = nullptr;
			{
				FScopeLock Lock(&LockSection);
				ToBeClosed = PopFromHead();
			}
			if (ToBeClosed)
			{
				// close it, freeing up space for new file to open
				PlatformCloseFile(ToBeClosed);
				HandlesCurrentlyInUse.Decrement();
			}
			else
			{
				FPlatformMisc::LowLevelOutputDebugString(TEXT("Spinning because we are actively reading from more file handles than we have possible handles.\r\n"));
				FPlatformProcess::SleepNoStats(.1f);
			}
		}
	}


	void LinkToTail(FRegisteredFileHandle* Handle)
	{
		check(!Handle->PreviousLink && !Handle->NextLink && !Handle->bIsOpenAndAvailableForClosing);
		Handle->bIsOpenAndAvailableForClosing = true;
		if (OpenAndAvailableForClosingTail)
		{
			Handle->PreviousLink = OpenAndAvailableForClosingTail;
			check(!OpenAndAvailableForClosingTail->NextLink);
			OpenAndAvailableForClosingTail->NextLink = Handle;
		}
		else
		{
			check(!OpenAndAvailableForClosingHead);
			OpenAndAvailableForClosingHead = Handle;
		}
		OpenAndAvailableForClosingTail = Handle;
	}

	void UnLink(FRegisteredFileHandle* Handle)
	{
		if (OpenAndAvailableForClosingHead == Handle)
		{
			verify(PopFromHead() == Handle);
			return;
		}
		check(Handle->bIsOpenAndAvailableForClosing);
		Handle->bIsOpenAndAvailableForClosing = false;
		if (OpenAndAvailableForClosingTail == Handle)
		{
			check(OpenAndAvailableForClosingHead && OpenAndAvailableForClosingHead != Handle && Handle->PreviousLink);
			OpenAndAvailableForClosingTail = Handle->PreviousLink;
			OpenAndAvailableForClosingTail->NextLink = nullptr;
			Handle->NextLink = nullptr;
			Handle->PreviousLink = nullptr;
			return;
		}
		check(Handle->NextLink && Handle->PreviousLink);
		Handle->NextLink->PreviousLink = Handle->PreviousLink;
		Handle->PreviousLink->NextLink = Handle->NextLink;
		Handle->NextLink = nullptr;
		Handle->PreviousLink = nullptr;

	}

	FRegisteredFileHandle* PopFromHead()
	{
		FRegisteredFileHandle* Result = OpenAndAvailableForClosingHead;
		if (Result)
		{
			check(!Result->PreviousLink);
			check(Result->bIsOpenAndAvailableForClosing);
			Result->bIsOpenAndAvailableForClosing = false;
			OpenAndAvailableForClosingHead = Result->NextLink;
			if (!OpenAndAvailableForClosingHead)
			{
				check(OpenAndAvailableForClosingTail == Result);
				OpenAndAvailableForClosingTail = nullptr;
			}
			else
			{
				check(OpenAndAvailableForClosingHead->PreviousLink == Result);
				OpenAndAvailableForClosingHead->PreviousLink = nullptr;
			}
			Result->NextLink = nullptr;
			Result->PreviousLink = nullptr;
		}
		return Result;
	}

	// critical section to protect the below arrays
	FCriticalSection LockSection;

	int32 MaxOpenHandles;

	FRegisteredFileHandle* OpenAndAvailableForClosingHead;
	FRegisteredFileHandle* OpenAndAvailableForClosingTail;

	FThreadSafeCounter HandlesCurrentlyInUse;
};

