// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Compression/CompressedBuffer.h"
#include "Containers/StringFwd.h"
#include "IO/IoHash.h"
#include "Features/IModularFeature.h"
#include "Features/IModularFeatures.h"

class FPackagePath;

namespace UE::Virtualization
{

/** Profiling data containing all activity relating to payloads. */
struct FPayloadActivityInfo
{
	struct FActivity
	{
		/** The number of payloads that have been involved by the activity. */
		int64 PayloadCount = 0;
		/** The total size of all payloads involved in the activity, in bytes. */
		int64 TotalBytes = 0;
		/** The total number of cycles spent on the activity across all threads. */
		int64 CyclesSpent = 0;
	};

	FActivity Pull;
	FActivity Push;
	FActivity Cache;
};

/** Describes the type of storage to use for a given action */
enum class EStorageType : int8
{
	/** Store in the local cache backends, this can be called from any thread */
	Local = 0,
	/** Store in the persistent backends, this can only be called from the game thread due to limitations with ISourceControlModule. */
	Persistent
};

/** Describes the status of a payload in regards to a backend storage system */
enum class FPayloadStatus : int8
{
	/** The payload id was not value */
	Invalid = -1,
	/** The payload was not found in any backend for the given storage type */
	NotFound = 0,
	/** The payload was found in at least one backend but was not found in all backends available for the given storage type */
	Partial,
	/** The payload was found in all of the backends available for the given storage type */
	FoundAll
};

/** Data structure representing a request to push a payload to a backend storage system */
struct FPushRequest
{
	enum class EStatus
	{
		/** The request failed, or was not reached because of the failure of an earlier request */
		Failed,
		/** The payload does not have a valid identifier or is empty */
		Invalid,
		/** The payload is below the minimum length required for virtualization */
		BelowMinSize,
		/** The payload is owned by a package that is excluded from virtualization by path filtering */
		ExcludedByPackagPath,
		/** The payload in the request is now present in all backends */
		Success
	};

	FPushRequest(const FIoHash& InIdentifier, const FCompressedBuffer& InPayload, const FString& InContext)
		: Identifier(InIdentifier)
		, Payload(InPayload)
		, Context(InContext)
	{

	}

	FPushRequest(const FIoHash& InIdentifier, FCompressedBuffer&& InPayload, FString&& InContext)
		: Identifier(InIdentifier)
		, Payload(InPayload)
		, Context(InContext)
	{

	}

	/** The identifier of the payload */
	FIoHash Identifier;
	/** The payload data */
	FCompressedBuffer Payload;
	/** A string containing context for the payload, typically a package name */
	FString Context;

	/** Once the request has been processed this value will contains the results */
	EStatus Status = EStatus::Failed;
};

/**
 * Creates the global IVirtualizationSystem if it has not already been set up. This can be called explicitly
 * during process start up but it will also be called by IVirtualizationSystem::Get if it detects that the
 * IVirtualizationSystem has not yet been set up.
 */
CORE_API void Initialize();

/** 
 * The base interface for the virtualization system. An Epic maintained version can be found in the Virtualization module.
 To implement your own, simply derived from this interface and then use the
 * UE_REGISTER_VIRTUALIZATION_SYSTEM macro in the cpp to register it as an option. 
 * You can then set the config file option [Core.ContentVirtualization]SystemName=FooBar, where FooBar should be the 
 * SystemName parameter you used when registering with the macro.
 * 
 * Special Cases:
 * SystemName=Off		-	This is the default set up and means a project will not use content virtualization
 *							Note that calling IVirtualizationSystem::Get() will still return a valid 
 *							IVirtualizationSystem implementation, but all push and pull operations will result 
 *							in failure and IsEnabled will always return false.
 * SystemName=Default	-	This will cause the default Epic implementation to be used @see VirtualizationManager
 */
class CORE_API IVirtualizationSystem
{
public:
	IVirtualizationSystem() = default;
	virtual ~IVirtualizationSystem() = default;

	/** Gain access to the current virtualization system active for the project */
	static IVirtualizationSystem& Get();

	/** Poll to see if content virtualization is enabled or not. */
	virtual bool IsEnabled() const = 0;

	/** Poll to see if pushing virtualized content to the given backend storage type is enabled or not. */
	virtual bool IsPushingEnabled(EStorageType StorageType) const = 0;

	/**
	 * Push a payload to the virtualization backends.
	 *
	 * @param	Id				The identifier of the payload being pushed.
	 * @param	Payload			The payload itself in FCompressedBuffer form, it is assumed that if the buffer is to 
	 *							be compressed that it will have been done by the caller.
	 * @param	StorageType		The type of storage to push the payload to, @See EStorageType for details.
	 * @param	PackageContext	Context for the payload being submitted, typically the name from the UPackage that owns it.
	 * 
	 * @return	True if at least one backend now contains the payload, otherwise false.
	 */
	virtual bool PushData(const FIoHash& Id, const FCompressedBuffer& Payload, EStorageType StorageType, const FString& Context) = 0;
	
	/**
	 * Push one or more payloads to a backend storage system. @See FPushRequest.
	 * 
	 * @param	Requests	A list of one or more payloads
	 * @param	StorageType	The type of storage to push the payload to, @See EStorageType for details.
	 * 
	 * @return	When StorageType is Local, this method will return true assuming at least one backend
	 *			managed to push all of the payloads. 
	 *			When StorageType is Persistent, this method will only return true if ALL backends
	 *			manage to push all of the payloads.
	 * 			If this returns true then you can check the Status member of each request for more info 
	 *			about each payloads push operation.
	 *			If this returns false then you can assume that the payloads are not safely virtualized.
	 */
	virtual bool PushData(TArrayView<FPushRequest> Requests, EStorageType StorageType) = 0;

	/**
	 * Pull a payload from the virtualization backends.
	 *
	 * @param	Id The identifier of the payload being pulled.
	 * @return	The payload in the form of a FCompressedBuffer. No decompression will be applied to the payload, it is 
	 *			up to the caller if they want to retain the payload in compressed or uncompressed format.  If no
	 *			backend contained the payload then an empty invalid FCompressedBuffer will be returned.
	 */
	virtual FCompressedBuffer PullData(const FIoHash& Id) = 0;

	

	/**
	 * Query if a number of payloads exist or not in the given storage type. 
	 * 
	 * @param	Ids					One or more payload identifiers to test
	 * @param	StorageType			The type of storage to push the payload to, @See EStorageType for details.
	 * @param	OutStatuses [out]	An array containing the results for each payload. @See FPayloadStatus
	 * 								If the operation succeeds the array will be resized to match the size of Ids. 
	 * 
	 * @return	True if the operation succeeded and the contents of OutStatuses is valid. False if errors were 
	 * 			encountered in which case the contents of OutStatuses should be ignored.
	 */
	virtual bool DoPayloadsExist(TArrayView<const FIoHash> Ids, EStorageType StorageType, TArray<FPayloadStatus>& OutStatuses) = 0;

	using GetPayloadActivityInfoFuncRef = TFunctionRef<void(const FString& DebugName, const FString& ConfigName, const FPayloadActivityInfo& PayloadInfo)>;

	/** Access profiling info relating to payload activity per backend. Stats will only be collected if ENABLE_COOK_STATS is enabled.*/
	virtual void GetPayloadActivityInfo( GetPayloadActivityInfoFuncRef ) const = 0;

	/** Access profiling info relating to accumulated payload activity. Stats will only be collected if ENABLE_COOK_STATS is enabled.*/
	virtual FPayloadActivityInfo GetAccumualtedPayloadActivityInfo() const = 0;

	//* Notification messages
	enum ENotification
	{
		PushBegunNotification,
		PushEndedNotification,
		PushFailedNotification,

		PullBegunNotification,
		PullEndedNotification,
		PullFailedNotification,
	};

	/** Declare delegate for notifications*/
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnNotification, ENotification, const FIoHash&);

	virtual FOnNotification& GetNotificationEvent() = 0;
};

namespace Private
{

/** 
 * Factory interface for creating virtualization systems. This is not intended to be derived from 
 * directly. Use the provided UE_REGISTER_VIRTUALIZATION_SYSTEM macro instead 
 */
class IVirtualizationSystemFactory : public IModularFeature
{
public:
	/** Creates and returns a new virtualization system instance */
	virtual TUniquePtr<IVirtualizationSystem> Create() = 0;

	/** Returns the name of the system that this factory created */
	virtual FName GetName() = 0;
};

} // namespace Private

/**
 * Registers a class derived from IVirtualizationSystem so that it can be set as the virtualization system for
 * the process to use.
 * 
 * @param SystemClass	The class derived from IVirtualizationSystem
 * @param SystemName	The name of the system that will be used to potentially select the system for use
 */
#define UE_REGISTER_VIRTUALIZATION_SYSTEM(SystemClass, SystemName) \
	class FVirtualizationSystem##Factory : public Private::IVirtualizationSystemFactory \
	{ \
	public: \
		FVirtualizationSystem##Factory() { IModularFeatures::Get().RegisterModularFeature(FName("VirtualizationSystem"), this); }\
		virtual ~FVirtualizationSystem##Factory() { IModularFeatures::Get().UnregisterModularFeature(FName("VirtualizationSystem"), this); } \
	private: \
		virtual TUniquePtr<IVirtualizationSystem> Create() override { return MakeUnique<SystemClass>(); } \
		virtual FName GetName() override { return FName(#SystemName); } \
	}; \
	static FVirtualizationSystem##Factory FVirtualizationSystem##Factory##Instance;

namespace Experimental
{

class IVirtualizationSourceControlUtilities : public IModularFeature
{
public:
	/**
	 * Given a package path this method will attempt to sync th e.upayload file that is compatible with
	 * the .uasset file of the package.
	 *
	 * We can make the following assumptions about the relationship between .uasset and .upayload files:
	 * 1) The .uasset may be submitted to perforce without the .upayload (if the payload is unmodified)
	 * 2) If the payload is modified then the .uasset and .upayload file must be submitted at the same time.
	 * 3) The caller has already checked the existing .upayload file (if any) to see if it contains the payload
	 * that they are looking for.
	 *
	 * If the above is true then we can sync the .upayload file to the same perforce changelist as the
	 * * .uasset and be sure that we have the correct version.
	 *
	 * Note that this has only been tested with perforce and so other source control solutions are currently
	 * unsupported.
	 */
	virtual bool SyncPayloadSidecarFile(const FPackagePath& PackagePath) = 0;
};

} // namespace Experimental

} // namespace UE::Virtualization
