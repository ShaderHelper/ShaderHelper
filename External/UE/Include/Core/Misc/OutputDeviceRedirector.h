// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/OutputDevice.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"

/*-----------------------------------------------------------------------------
FOutputDeviceRedirector.
-----------------------------------------------------------------------------*/

class FLogAllocator;

/** The type of lines buffered by secondary threads. */
struct CORE_API FBufferedLine
{
	enum EBufferedLineInit
	{
		EMoveCtor = 0
	};

	const TCHAR* Data;
	const FLazyName Category;
	const double Time;
	const ELogVerbosity::Type Verbosity;
	bool bExternalAllocation;

	FBufferedLine(const TCHAR* InData, const FName& InCategory, ELogVerbosity::Type InVerbosity, const double InTime = -1, FLogAllocator* ExternalAllocator = nullptr);
	FBufferedLine(const TCHAR* InData, const FLazyName& InCategory, ELogVerbosity::Type InVerbosity, const double InTime = -1, FLogAllocator* ExternalAllocator = nullptr);

	FBufferedLine(FBufferedLine& InBufferedLine, EBufferedLineInit Unused)
		: Data(InBufferedLine.Data)
		, Category(InBufferedLine.Category)
		, Time(InBufferedLine.Time)
		, Verbosity(InBufferedLine.Verbosity)
		, bExternalAllocation(InBufferedLine.bExternalAllocation)
	{
		InBufferedLine.Data = nullptr;
		InBufferedLine.bExternalAllocation = false;
	}

	/** Noncopyable for now, could be made movable */
	FBufferedLine(const FBufferedLine&) = delete;
	FBufferedLine& operator=(const FBufferedLine&) = delete;
	~FBufferedLine();
};

/**
* Class used for output redirection to allow logs to show
*/
class CORE_API FOutputDeviceRedirector : public FOutputDevice
{
public:

	typedef TArray<FOutputDevice*, TInlineAllocator<16> > TLocalOutputDevicesArray;

private:
	enum { InlineLogEntries = 16 };

	/** A FIFO of lines logged by non-master threads. */
	TArray<FBufferedLine, TInlineAllocator<InlineLogEntries>> BufferedLines;

	/** A FIFO backlog of messages logged before the editor had a chance to intercept them. */
	TArray<FBufferedLine> BacklogLines;

	/** Array of output devices to redirect to using buffering mechanism */
	TArray<FOutputDevice*> BufferedOutputDevices;

	/** Array of output devices that can redirected to without bufffering */
	TArray<FOutputDevice*> UnbufferedOutputDevices;

	/** The master thread ID.  Logging from other threads will be buffered for processing by the master thread. */
	uint32 MasterThreadID;

	/** Whether backlogging is enabled. */
	bool bEnableBacklog;

	FLogAllocator* BufferedLinesAllocator;

	/** Objects used for synchronization via a scoped lock */
	FCriticalSection	SynchronizationObject;
	FCriticalSection	BufferSynchronizationObject;
	FCriticalSection	OutputDevicesMutex;
	FThreadSafeCounter	OutputDevicesLockCounter;

	/**
	* The unsynchronized version of FlushThreadedLogs.
	* Assumes that the caller holds a lock on SynchronizationObject.
	* @param bUseAllDevices - if true this method will use all output devices
	*/
	void InternalFlushThreadedLogs(TLocalOutputDevicesArray& InBufferedDevices, TLocalOutputDevicesArray& InUnbufferedDevices, bool bUseAllDevices);
	void InternalFlushThreadedLogs(bool bUseAllDevices);

	/** Locks OutputDevices arrays so that nothing can be added or removed from them */
	void LockOutputDevices(TLocalOutputDevicesArray& OutBufferedDevices, TLocalOutputDevicesArray& OutUnbufferedDevices);

	/** Unlocks OutputDevices arrays */
	void UnlockOutputDevices();

	friend struct FOutputDevicesLock;

	/** Helper struct for scope locking OutputDevices arrays */
	struct FOutputDevicesLock
	{
		FOutputDeviceRedirector* Redirector;
		FOutputDevicesLock(FOutputDeviceRedirector* InRedirector, TLocalOutputDevicesArray& OutBufferedDevices, TLocalOutputDevicesArray& OutUnbufferedDevices)
			: Redirector(InRedirector)
		{
			Redirector->LockOutputDevices(OutBufferedDevices, OutUnbufferedDevices);
		}
		~FOutputDevicesLock()
		{
			Redirector->UnlockOutputDevices();
		}
	};

	void EmptyBufferedLines();

	template<class T>
	void SerializeImpl(const TCHAR* Data, ELogVerbosity::Type Verbosity, T& CategoryName, double Time);

public:

	/** Initialization constructor. */
	explicit FOutputDeviceRedirector(FLogAllocator* Allocator = nullptr);

	/**
	* Get the GLog singleton
	*/
	static FOutputDeviceRedirector* Get();

	/**
	* Adds an output device to the chain of redirections.
	*
	* @param OutputDevice	output device to add
	*/
	void AddOutputDevice(FOutputDevice* OutputDevice);

	/**
	* Removes an output device from the chain of redirections.
	*
	* @param OutputDevice	output device to remove
	*/
	void RemoveOutputDevice(FOutputDevice* OutputDevice);

	/**
	* Returns whether an output device is currently in the list of redirectors.
	*
	* @param	OutputDevice	output device to check the list against
	* @return	true if messages are currently redirected to the the passed in output device, false otherwise
	*/
	bool IsRedirectingTo(FOutputDevice* OutputDevice);

	/** Flushes lines buffered by secondary threads. */
	virtual void FlushThreadedLogs();

	/**
	*	Flushes lines buffered by secondary threads.
	*	Only used if a background thread crashed and we needed to push the callstack into the log.
	*/
	virtual void PanicFlushThreadedLogs();

	/**
	* Serializes the current backlog to the specified output device.
	* @param OutputDevice	- Output device that will receive the current backlog
	*/
	virtual void SerializeBacklog(FOutputDevice* OutputDevice);

	/**
	* Enables or disables the backlog.
	* @param bEnable	- Starts saving a backlog if true, disables and discards any backlog if false
	*/
	virtual void EnableBacklog(bool bEnable);

	/**
	* Sets the current thread to be the master thread that prints directly
	* (isn't queued up)
	*/
	virtual void SetCurrentThreadAsMasterThread();

	/**
	* Serializes the passed in data via all current output devices.
	*
	* @param	Data	Text to log
	* @param	Event	Event name used for suppression purposes
	*/
	virtual void Serialize(const TCHAR* Data, ELogVerbosity::Type Verbosity, const class FName& Category, const double Time) override;

	/**
	* Serializes the passed in data via all current output devices.
	*
	* @param	Data	Text to log
	* @param	Event	Event name used for suppression purposes
	*/
	virtual void Serialize(const TCHAR* Data, ELogVerbosity::Type Verbosity, const class FName& Category) override;
	
	/** Same as Serialize() but FName creation. Only needed to support 
	*
	*/
	void RedirectLog(const FLazyName& Category, ELogVerbosity::Type Verbosity, const TCHAR* Data);

	void RedirectLog(const FName& Category, ELogVerbosity::Type Verbosity, const TCHAR* Data);

	/**
	* Passes on the flush request to all current output devices.
	*/
	void Flush() override;

	/**
	* Closes output device and cleans up. This can't happen in the destructor
	* as we might have to call "delete" which cannot be done for static/ global
	* objects.
	*/
	void TearDown() override;

	/**
	* Determine if backlog is enabled
	*/
	bool IsBacklogEnabled() const
	{
		return bEnableBacklog;
	}
};
