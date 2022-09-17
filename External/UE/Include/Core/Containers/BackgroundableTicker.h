// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Ticker.h"

/**
 * This works the same as the core FTSTicker, but on supported mobile platforms
 * it continues ticking while the app is running in the background.
 */
class CORE_API FTSBackgroundableTicker
	: public FTSTicker
{
public:
	static FTSBackgroundableTicker& GetCoreTicker();

	FTSBackgroundableTicker();
	~FTSBackgroundableTicker();

private:
	FDelegateHandle CoreTickerHandle;
	::FDelegateHandle BackgroundTickerHandle;
	bool bWasBackgrounded = false;
};

PRAGMA_DISABLE_DEPRECATION_WARNINGS

/**
 * DEPRECATED not thread-safe version
 * For migration guide see `FTSTicker`
 * 
 * This works the same as the core FTicker, but on supported mobile platforms 
 * it continues ticking while the app is running in the background.
 */
class UE_DEPRECATED(5.0, "Please use thread-safe FTSTickerObjectBase, see the migration guide in the code comment above FTSTicker") FBackgroundableTicker
	: public FTicker
{
public:

	CORE_API static FBackgroundableTicker& GetCoreTicker();

	CORE_API FBackgroundableTicker();
	CORE_API ~FBackgroundableTicker();

private:
	
	FDelegateHandle CoreTickerHandle;
	FDelegateHandle BackgroundTickerHandle;
	bool bWasBackgrounded = false;
};

PRAGMA_ENABLE_DEPRECATION_WARNINGS
