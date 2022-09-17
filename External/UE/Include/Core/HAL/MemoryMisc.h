// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "Containers/Map.h"

/** Holds generic memory stats, internally implemented as a map. */
struct FGenericMemoryStats
{
	void Add( const TCHAR* StatDescription, const SIZE_T StatValue )
	{
		Data.Add( FString(StatDescription), StatValue );
	}

	TMap<FString, SIZE_T> Data;
};

#ifndef ENABLE_MEMORY_SCOPE_STATS
#define ENABLE_MEMORY_SCOPE_STATS 0
#endif

// This will grab the memory stats of VM and Physical before and at the end of scope
// reporting +/- difference in memory.
// WARNING This will also capture differences in Threads which have nothing to do with the scope
#if ENABLE_MEMORY_SCOPE_STATS
class CORE_API FScopedMemoryStats
{
public:
	explicit FScopedMemoryStats(const TCHAR* Name);

	~FScopedMemoryStats();

private:
	const TCHAR* Text;
	const FPlatformMemoryStats StartStats;
};
#else
class FScopedMemoryStats
{
public:
	explicit FScopedMemoryStats(const TCHAR* Name) {}
};
#endif

/**
 * The FSharedMemoryTracker is used to track how much the shared and unique memory pools changed size in-between each call
 * WARNING: Getting the shared & unique memory pool size is extremely costly (easily takes up to 60ms) so be careful
 * not to use the tracker while hosting a game.
 */
#ifndef ENABLE_SHARED_MEMORY_TRACKER
#define ENABLE_SHARED_MEMORY_TRACKER 0
#endif

#if ENABLE_SHARED_MEMORY_TRACKER && PLATFORM_UNIX

class CORE_API FSharedMemoryTracker
{
public:

	/** Print the difference in memory pool sizes since the last call to this function exclusively */
	static void PrintMemoryDiff(const TCHAR* Context);


	/** Store the memory pool size at construction and log the difference in memory that occurred during the lifetime of the tracker */
	explicit FSharedMemoryTracker(const FString& InContext);
	~FSharedMemoryTracker();

private:
	const FString PrintContext;
	const FExtendedPlatformMemoryStats StartStats;
};
#else
class CORE_API FSharedMemoryTracker
{
public:

	static void PrintMemoryDiff(const TCHAR* /*Context*/) {}

	explicit FSharedMemoryTracker(const FString& /*InContext*/) {}
};
#endif // ENABLE_SHARED_MEMORY_TRACKER  && PLATFORM_UNIX