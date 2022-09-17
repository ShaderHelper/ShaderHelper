// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Delegates/Delegate.h"
#include "Logging/LogMacros.h"
#include "Containers/Ticker.h"

DECLARE_LOG_CATEGORY_EXTERN(AsyncLoadingFlush, Log, All);

DECLARE_DELEGATE(FOnAsyncLoadingFlushComplete);

/**
 * Flush the async loader in a non-blocking manner.
 */
class COREUOBJECT_API FAsyncLoadingFlushContext final : FNoncopyable
{
public:
	FAsyncLoadingFlushContext(const FString & Context);
	FAsyncLoadingFlushContext(FString&& Context);
	~FAsyncLoadingFlushContext();

	void Flush(const FOnAsyncLoadingFlushComplete& OnFlushComplete);

	int32 GetId() const { return Id; }

private:
	FAsyncLoadingFlushContext() = delete;

	void CleanupTickers();

	bool OnAsyncLoadingCheck(float DeltaTime);
	bool OnAsyncLoadingWarn(float DeltaTime);

	/** How often to warn while waiting for async loading to complete. */
	static constexpr float LoadingFlushWarnIntervalSeconds = 10.f;

	/** Context for logging */
	FString Context;

	/** Unique id for context. */
	int32 Id;

	/** Delegate to be fired on flush. */
	FOnAsyncLoadingFlushComplete OnFlushCompleteDelegate;

	/** Record event timing for logging. */
	double StartTime = 0;

	/** Tick delegates. */
	FTSTicker::FDelegateHandle LoadingCompleteCheckDelegateHandle;
	FTSTicker::FDelegateHandle WaitingWarnDelegateHandle;

	/** Increment ids so each instance has a unique id. */
	static int32 NextId;
};
