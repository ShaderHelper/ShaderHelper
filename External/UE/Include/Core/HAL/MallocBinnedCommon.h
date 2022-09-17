// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"

#if PLATFORM_HAS_FPlatformVirtualMemoryBlock

#define BINNEDCOMMON_MAX_LISTED_SMALL_POOL_SIZE	28672
#define BINNEDCOMMON_NUM_LISTED_SMALL_POOLS	49

#if !defined(BINNEDCOMMON_USE_SEPARATE_VM_PER_POOL)
	#if PLATFORM_WINDOWS
		#define BINNEDCOMMON_USE_SEPARATE_VM_PER_POOL (1)
	#else
		#define BINNEDCOMMON_USE_SEPARATE_VM_PER_POOL (0)
	#endif
#endif


class FBitTree
{
	uint64* Bits; // one bits in middle layers mean "all allocated"
	uint32 Capacity; // rounded up to a power of two
	uint32 DesiredCapacity;
	uint32 Rows;
	uint32 OffsetOfLastRow;
	uint32 AllocationSize;

public:
	FBitTree()
		: Bits(nullptr)
	{
	}

	static uint32 GetMemoryRequirements(uint32 DesiredCapacity);
	void FBitTreeInit(uint32 InDesiredCapacity, void * Memory, uint32 MemorySize, bool InitialValue);
	uint32 AllocBit();
	bool IsAllocated(uint32 Index) const;
	void AllocBit(uint32 Index);
	uint32 NextAllocBit() const;
	uint32 NextAllocBit(uint32 StartIndex) const;
	void FreeBit(uint32 Index);
	uint32 CountOnes(uint32 UpTo) const;
};

struct FSizeTableEntry
{
	uint32 BlockSize;
	uint16 BlocksPerBlockOfBlocks;
	uint8 PagesPlatformForBlockOfBlocks;

	FSizeTableEntry()
	{
	}

	FSizeTableEntry(uint32 InBlockSize, uint64 PlatformPageSize, uint8 Pages4k, uint32 BasePageSize, uint32 MinimumAlignment);

	bool operator<(const FSizeTableEntry& Other) const
	{
		return BlockSize < Other.BlockSize;
	}
	static uint8 FillSizeTable(uint64 PlatformPageSize, FSizeTableEntry* SizeTable, uint32 BasePageSize, uint32 MinimumAlignment, uint32 MaxSize, uint32 SizeIncrement);
};

struct FArenaParams
{
	// these are parameters you set
	uint64 AddressLimit = 1024 * 1024 * 1024; // this controls the size of the root hash table
	uint32 BasePageSize = 4096; // this is used to make sensible calls to malloc and figures into the standard pool sizes if bUseStandardSmallPoolSizes is true
	uint32 AllocationGranularity = 4096; // this is the granularity of the commit and decommit calls used on the VM slabs
	uint32 MaxSizePerBundle = 8192;
	uint32 MaxStandardPoolSize = 128 * 1024; // these are added to the standard pool sizes, mainly to use the TLS caches, they are typically one block per slab
	uint16 MaxBlocksPerBundle = 64;
	uint8 MaxMemoryPerBlockSizeShift = 29;
	uint8 EmptyCacheAllocExtra = 32;
	uint8 MaxGlobalBundles = 32;
	uint8 MinimumAlignmentShift = 4;
	uint8 PoolCount;
	bool bUseSeparateVMPerPool = !!(BINNEDCOMMON_USE_SEPARATE_VM_PER_POOL);
	bool bPerThreadCaches = true;
	bool bUseStandardSmallPoolSizes = true;
	bool bAttemptToAlignSmallBocks = true;
	TArray<uint32> AdditionalBlockSizes;

	// This lambdas is similar to the platform virtual memory HAL and by default just call that. 
	TFunction<FPlatformMemory::FPlatformVirtualMemoryBlock(SIZE_T)> ReserveVM;

	// These allow you to override the large block allocator. The value add here is that MBA tracks the metadata for you and call tell the difference between a large block pointer and a small block pointer.
	// By defaults these just use the platform VM interface to allocate some committed memory
	TFunction<void*(SIZE_T, SIZE_T, SIZE_T&, uint32&)> LargeBlockAlloc;
	TFunction<void(void*, uint32)> LargeBlockFree;


	// these are parameters are derived from other parameters
	uint64 MaxMemoryPerBlockSize;
	uint32 MaxPoolSize;
	uint32 MinimumAlignment;
	uint32 MaximumAlignmentForSmallBlock;

};


#endif