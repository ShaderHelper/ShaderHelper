// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "Async/Future.h"
#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Compression/CompressedBuffer.h"
#include "Containers/ArrayView.h"
#include "Delegates/Delegate.h"
#include "HAL/CriticalSection.h"
#include "HAL/Platform.h"
#include "IO/IoHash.h"

class IBulkDataRegistry;
class UPackage;
struct FGuid;
namespace UE::Serialization { class FEditorBulkData; }

DECLARE_DELEGATE_RetVal(IBulkDataRegistry*, FSetBulkDataRegistry);

namespace UE::BulkDataRegistry
{
	/** Results of GetMeta call. */
	struct FMetaData
	{
		/** True if data was found, else false. */
		bool bValid;
		/** IoHash of the uncompressed bytes of the data that will be returned from GetData. */
		FIoHash RawHash;
		/** Size of the uncompressed bytes of the data that will be returned from GetData. */
		uint64 RawSize;
	};

	/** Results of GetData call. */
	struct FData
	{
		/** True if data was found, else false. */
		bool bValid;
		/** The discovered data. Empty if data was not found. */
		FCompressedBuffer Buffer;
	};

}

/** Registers BulkDatas so that they can be referenced by guid during builds later in the editor process. */
class IBulkDataRegistry
{
public:
	/** The BulkDataRegistry can be configured off. Return whether it is enabled. A stub is used if not enabled. */
	COREUOBJECT_API static bool IsEnabled();
	/** Get the global BulkDataRegistry; always returns a valid interface, so long as Initialize has been called. */
	COREUOBJECT_API static IBulkDataRegistry& Get();
	
	/** Register a BulkData with the registry. Its payload and metadata will be fetchable by its GetIdentifier. */
	virtual void Register(UPackage* Owner, const UE::Serialization::FEditorBulkData& BulkData) = 0;
	/** Report that a BulkData is leaving memory and its in-memory payload (if it had one) is no longer available. */
	virtual void OnExitMemory(const UE::Serialization::FEditorBulkData& BulkData) = 0;

	/** Return the metadata for the given registered BulkData; returns false if not registered. */
	virtual TFuture<UE::BulkDataRegistry::FMetaData> GetMeta(const FGuid& BulkDataId) = 0;

	/**
	 * Return the (possibly compressed) payload for the given registered BulkData.
	 * Returns an empty buffer if not registered.
	 */
	virtual TFuture<UE::BulkDataRegistry::FData> GetData(const FGuid& BulkDataId) = 0;

	/**
	 * Report whether the Package had BulkDatas during load that upgrade or otherwise exist in memoryonly and
	 * cannot save all its BulkDatas by reference when resaved. This function only returns the correct
	 * information until OnEndLoadPackage is called for the given package; after that it can return an arbitrary
	 * value.
	 */
	virtual uint64 GetBulkDataResaveSize(FName PackageName) = 0;

	/** Set and intialize global IBulkDataRegistry; Get fatally fails before. */
	COREUOBJECT_API static void Initialize();
	/** Shutdown and deallocate global IBulkDataRegistry; Get fatally fails afterwards. */
	COREUOBJECT_API static void Shutdown();
	/** Subscribe to set the class for the global IBulkDataRegistry. */
	COREUOBJECT_API static FSetBulkDataRegistry& GetSetBulkDataRegistryDelegate();

protected:
	virtual ~IBulkDataRegistry() {};
};

namespace UE::BulkDataRegistry::Private
{

/** Implements behavior needed across multiple BulkDataRegistry implementations for GetBulkDataResaveSize */
class FResaveSizeTracker
{
public:
	COREUOBJECT_API FResaveSizeTracker();
	COREUOBJECT_API ~FResaveSizeTracker();

	COREUOBJECT_API void Register(UPackage* Owner, const UE::Serialization::FEditorBulkData& BulkData);
	COREUOBJECT_API uint64 GetBulkDataResaveSize(FName PackageName);

private:
	COREUOBJECT_API void OnEndLoadPackage(TConstArrayView<UPackage*> LoadedPackages);
	COREUOBJECT_API void OnPostEngineInit();

	FRWLock Lock;
	TMap<FName, uint64> PackageBulkResaveSize;
	TArray<FName> DeferredRemove;
	bool bPostEngineInitComplete = false;
};

}


// Temporary interface for tunneling the EditorBuildInputResolver into CoreUObject.
// In the future this will be implemented as part of the BuildAPI
namespace UE::DerivedData { class IBuildInputResolver; }
COREUOBJECT_API UE::DerivedData::IBuildInputResolver* GetGlobalBuildInputResolver();
COREUOBJECT_API void SetGlobalBuildInputResolver(UE::DerivedData::IBuildInputResolver* InResolver);


#endif // WITH_EDITOR