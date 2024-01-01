// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Async/TaskGraphInterfaces.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/StringView.h"
#include "Containers/UnrealString.h"
#include "IO/IoHash.h"
#include "Misc/Guid.h"
#include "Templates/Tuple.h"
#include "Templates/TypeCompatibleBytes.h"

struct FTargetReceipt;
class FTargetReceiptBuildWorkerFactory;

/**
 * Globally registers a UE::DerivedData::IBuildWorkerFactory instance that runs an executable built by UnrealBuildTool.
 * UnrealBuildTool provides the executable information through a TargetReceipt file.
*/
class DESKTOPPLATFORM_API FTargetReceiptBuildWorker
{
public:
	FTargetReceiptBuildWorker(const TCHAR* TargetReceiptFilePath);
	virtual ~FTargetReceiptBuildWorker();

private:
	/**
	 * Encapsulates a path used by the worker that has both a local and remote representation.
	*/
	class FWorkerPath
	{
	public:
		FWorkerPath(FStringView InLocalPathRoot, FStringView InLocalPathSuffix, FStringView InRemotePathRoot = FString(), FStringView InRemotePathSuffix = FString());

		FString GetLocalPath() const { return LocalPath; }
		FString GetRemotePath() const { return RemotePath; }
	private:
		FString LocalPath;
		FString RemotePath;
	};

	friend class FPopulateWorkerFromTargetReceiptTask;
	friend class FTargetReceiptBuildWorkerFactory;

	FTargetReceiptBuildWorkerFactory* GetWorkerFactory();
	bool TryAddExecutablePath(FStringView Path);
	void PopulateWorkerFromReceipt(const FString& TargetReceiptFilePath);

	/**
	 * Dummy type used to indicate the size of an empty class (no members) with virtual functions (contains a virtual function table).
	 * The FTargetReceiptBuildWorkerFactory type will be created and stored in the space of one of these.
	 * Size matching is enforced with a static_assert.  The purpose of this is to avoid exposing DerivedDataCache types to
	 * modules that specify a target receipt worker.
	*/
	class IPureVirtual
	{
		virtual void Dummy();
	};

	static const FGuid WorkerReceiptVersion;
	TTypeCompatibleBytes<IPureVirtual> InternalFactory;
	FGraphEventRef PopulateTaskRef;
	FString Name;
	FString Platform;
	TMap<FString, FString> EnvironmentVariables;
	TArray<FWorkerPath> ExecutablePaths;
	using FWorkerPathMeta = TTuple<FIoHash, uint64>;
	TArray<FWorkerPathMeta> ExecutableMeta;
	FGuid BuildSystemVersion;
	TArray<TPair<FString, FGuid>> FunctionVersions;
	std::atomic<bool> bAbortRequested;
	bool bEnabled;
};
