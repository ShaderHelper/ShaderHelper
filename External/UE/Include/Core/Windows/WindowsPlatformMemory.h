// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformMemory.h"
#include "Windows/WindowsSystemIncludes.h"

class FString;
class FMalloc;
struct FGenericMemoryStats;

/**
 *	Windows implementation of the FGenericPlatformMemoryStats.
 *	At this moment it's just the same as the FGenericPlatformMemoryStats.
 *	Can be extended as shown in the following example.
 */
struct FPlatformMemoryStats
	: public FGenericPlatformMemoryStats
{
	/** Default constructor, clears all variables. */
	FPlatformMemoryStats()
		: FGenericPlatformMemoryStats()
		, WindowsSpecificMemoryStat(0)
	{ }

	/** Memory stat specific only for Windows. */
	SIZE_T WindowsSpecificMemoryStat;
};


/**
* Windows implementation of the memory OS functions
**/
struct CORE_API FWindowsPlatformMemory
	: public FGenericPlatformMemory
{
	enum EMemoryCounterRegion
	{
		MCR_Invalid, // not memory
		MCR_Physical, // main system memory
		MCR_GPU, // memory directly a GPU (graphics card, etc)
		MCR_GPUSystem, // system memory directly accessible by a GPU
		MCR_TexturePool, // presized texture pools
		MCR_StreamingPool, // amount of texture pool available for streaming.
		MCR_UsedStreamingPool, // amount of texture pool used for streaming.
		MCR_GPUDefragPool, // presized pool of memory that can be defragmented.
		MCR_SamplePlatformSpecifcMemoryRegion, 
		MCR_PhysicalLLM, // total physical memory displayed in the LLM stats (on consoles CPU + GPU)
		MCR_MAX
	};

	/**
	 * Windows representation of a shared memory region
	 */
	struct FWindowsSharedMemoryRegion : public FSharedMemoryRegion
	{
		/** Returns the handle to file mapping object. */
		Windows::HANDLE GetMapping() const { return Mapping; }

		FWindowsSharedMemoryRegion(const FString& InName, uint32 InAccessMode, void* InAddress, SIZE_T InSize, Windows::HANDLE InMapping)
			:	FSharedMemoryRegion(InName, InAccessMode, InAddress, InSize)
			,	Mapping(InMapping)
		{}

	protected:

		/** Handle of a file mapping object */
		Windows::HANDLE				Mapping;
	};

	//~ Begin FGenericPlatformMemory Interface
	static void Init();
	static uint32 GetBackMemoryPoolSize()
	{
		/**
		* Value determined by series of tests on Fortnite with limited process memory.
		* 26MB sufficed to report all test crashes, using 32MB to have some slack.
		* If this pool is too large, use the following values to determine proper size:
		* 2MB pool allowed to report 78% of crashes.
		* 6MB pool allowed to report 90% of crashes.
		*/
		return 32 * 1024 * 1024;
	}

	static class FMalloc* BaseAllocator();
	static FPlatformMemoryStats GetStats();
	static void GetStatsForMallocProfiler( FGenericMemoryStats& out_Stats );
	static const FPlatformMemoryConstants& GetConstants();
	static bool PageProtect(void* const Ptr, const SIZE_T Size, const bool bCanRead, const bool bCanWrite);
	static void* BinnedAllocFromOS( SIZE_T Size );
	static void BinnedFreeToOS( void* Ptr, SIZE_T Size );

	class FPlatformVirtualMemoryBlock : public FBasicVirtualMemoryBlock
	{
	public:

		FPlatformVirtualMemoryBlock()
		{
		}

		FPlatformVirtualMemoryBlock(void *InPtr, uint32 InVMSizeDivVirtualSizeAlignment)
			: FBasicVirtualMemoryBlock(InPtr, InVMSizeDivVirtualSizeAlignment)
		{
		}
		FPlatformVirtualMemoryBlock(const FPlatformVirtualMemoryBlock& Other) = default;
		FPlatformVirtualMemoryBlock& operator=(const FPlatformVirtualMemoryBlock& Other) = default;

		void Commit(size_t InOffset, size_t InSize);
		void Decommit(size_t InOffset, size_t InSize);
		void FreeVirtual();

		FORCEINLINE void CommitByPtr(void *InPtr, size_t InSize)
		{
			Commit(size_t(((uint8*)InPtr) - ((uint8*)Ptr)), InSize);
		}

		FORCEINLINE void DecommitByPtr(void *InPtr, size_t InSize)
		{
			Decommit(size_t(((uint8*)InPtr) - ((uint8*)Ptr)), InSize);
		}

		FORCEINLINE void Commit()
		{
			Commit(0, GetActualSize());
		}

		FORCEINLINE void Decommit()
		{
			Decommit(0, GetActualSize());
		}

		FORCEINLINE size_t GetActualSize() const
		{
			return VMSizeDivVirtualSizeAlignment * GetVirtualSizeAlignment();
		}

		static FPlatformVirtualMemoryBlock AllocateVirtual(size_t Size, size_t InAlignment = FPlatformVirtualMemoryBlock::GetVirtualSizeAlignment());
		static size_t GetCommitAlignment();
		static size_t GetVirtualSizeAlignment();
	};

	static FSharedMemoryRegion* MapNamedSharedMemoryRegion(const FString& InName, bool bCreate, uint32 AccessMode, SIZE_T Size, const void* pSecurityAttributes = nullptr);
	static bool UnmapNamedSharedMemoryRegion(FSharedMemoryRegion * MemoryRegion);
	static bool GetLLMAllocFunctions(void*(*&OutAllocFunction)(size_t), void(*&OutFreeFunction)(void*, size_t), int32& OutAlignment);
protected:
	friend struct FGenericStatsUpdater;

	static void InternalUpdateStats( const FPlatformMemoryStats& MemoryStats );
	//~ End FGenericPlatformMemory Interface
};


typedef FWindowsPlatformMemory FPlatformMemory;
