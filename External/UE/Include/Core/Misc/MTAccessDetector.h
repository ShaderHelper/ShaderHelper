// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "CoreGlobals.h"
#include "Misc/AssertionMacros.h"
#include "HAL/PlatformTLS.h"

#define ENABLE_MT_DETECTOR DO_CHECK

#if ENABLE_MT_DETECTOR

/**
 * Read write multithread access detector, will check on concurrent write/write and read/write access, but will not on concurrent read access.
 * Note this detector is not re-entrant, see FRWRecursiveAccessDetector and FRWFullyRecursiveAccessDetector. 
 * But FRWAccessDetector should be the default one to start with.
 */
struct FRWAccessDetector
{
public:

	FRWAccessDetector()
		: AtomicValue(0)
		{}

	~FRWAccessDetector()
	{
		checkf(AtomicValue == 0, TEXT("Detector cannot be destroyed while other threads has not release all access"))
	}

	/**
	 * Acquires read access, will check if there are any writers
	 * @return true if no errors were detected
	 */
	FORCEINLINE bool AcquireReadAccess() const
	{
		const bool ErrorDetected = (AtomicValue.fetch_add(1, std::memory_order_relaxed) & WriterBits) != 0;
		checkf(!ErrorDetected || GIsAutomationTesting, TEXT("Aquiring a read access while there is already a write access"));
		return !ErrorDetected;
	}

	/**
	 * Releases read access, will check if there are any writers
	 * @return true if no errors were detected
	 */
	FORCEINLINE bool ReleaseReadAccess() const
	{
		const bool ErrorDetected = (AtomicValue.fetch_sub(1, std::memory_order_relaxed) & WriterBits) != 0;
		checkf(!ErrorDetected || GIsAutomationTesting, TEXT("Another thread asked to have a write access during this read access"));
		return !ErrorDetected;
	}

	/** 
	 * Acquires write access, will check if there are readers or other writers
	 * @return true if no errors were detected
	 */
	FORCEINLINE bool AcquireWriteAccess() const
	{
		const bool ErrorDetected = AtomicValue.fetch_add(WriterIncrementValue, std::memory_order_relaxed) != 0;
		checkf(!ErrorDetected || GIsAutomationTesting, TEXT("Acquiring a write access while there are ongoing read or write access"));
		return !ErrorDetected;
	}

	/** 
	 * Releases write access, will check if there are readers or other writers
	 * @return true if no errors were detected
	 */
	FORCEINLINE bool ReleaseWriteAccess() const
	{
		const bool ErrorDetected = AtomicValue.fetch_sub(WriterIncrementValue, std::memory_order_relaxed) != WriterIncrementValue;
		checkf(!ErrorDetected || GIsAutomationTesting, TEXT("Another thread asked to have a read or write access during this write access"));
		return !ErrorDetected;
	}

protected:

	// We need to do an atomic operation to know there are multiple writers, this is why we reserve more than one bit for them.
	// While firing the check upon acquire write access, the other writer thread could continue and hopefully fire a check upon releasing access so we get both faulty callstacks.
	static constexpr uint32 WriterBits = 0xfff00000;
	static constexpr uint32 WriterIncrementValue = 0x100000;

	mutable std::atomic<uint32> AtomicValue;
};

/**
 * Same as FRWAccessDetector, but support re-entrance on the write access
 * See FRWFullyRecursiveAccessDetector for read access re-entrance when holding a write access
 */
struct FRWRecursiveAccessDetector : public FRWAccessDetector
{
public:
	/**
	 * Acquires write access, will check if there are readers or other writers
	 * @return true if no errors were detected
	 */
	FORCEINLINE bool AcquireWriteAccess() const
	{
		uint32 CurThreadID = FPlatformTLS::GetCurrentThreadId();

		if (WriterThreadID == CurThreadID)
		{
			RecursiveDepth++;
			return true;
		}
		else if (FRWAccessDetector::AcquireWriteAccess())
		{
			check(RecursiveDepth == 0);
			WriterThreadID = CurThreadID;
			RecursiveDepth++;
			return true;
		}
		return false;
	}

	/**
	 * Releases write access, will check if there are readers or other writers
	 * @return true if no errors were detected
	 */
	FORCEINLINE bool ReleaseWriteAccess() const
	{
		uint32 CurThreadID = FPlatformTLS::GetCurrentThreadId();
		if (WriterThreadID == CurThreadID)
		{
			check(RecursiveDepth > 0);
			RecursiveDepth--;

			if (RecursiveDepth == 0)
			{
				WriterThreadID = (uint32)-1;
				return FRWAccessDetector::ReleaseWriteAccess();
			}
			return true;
		}
		else
		{
			// This can happen when a user continues pass a reported error, 
			// just trying to keep things going as best as possible.
			return FRWAccessDetector::ReleaseWriteAccess();
		}
	}

protected:
	mutable uint32 WriterThreadID = (uint32)-1;
	mutable int32 RecursiveDepth = 0;
};

/**
 * Same as FRWRecursiveAccessDetector, but support re-entrance on read access when holding a write access.
 */
struct FRWFullyRecursiveAccessDetector : public FRWRecursiveAccessDetector
{
public:
	/**
	 * Acquires read access, will check if there are any writers
	 * @return true if no errors were detected
	 */
	FORCEINLINE bool AcquireReadAccess() const
	{
		uint32 CurThreadID = FPlatformTLS::GetCurrentThreadId();
		if (WriterThreadID == CurThreadID)
		{
			return true;
		}
		return FRWAccessDetector::AcquireReadAccess();
	}

	/**
	 * Releases read access, will check if there are any writers
	 * @return true if no errors were detected
	 */
	FORCEINLINE bool ReleaseReadAccess() const
	{
		uint32 CurThreadID = FPlatformTLS::GetCurrentThreadId();
		if (WriterThreadID == CurThreadID)
		{
			return true;
		}
		return FRWAccessDetector::ReleaseReadAccess();
	}
};

struct FBaseScopedAccessDetector
{
};

template<typename RWAccessDetector>
struct TScopedReaderAccessDetector : public FBaseScopedAccessDetector
{
public:

	FORCEINLINE TScopedReaderAccessDetector(const RWAccessDetector& InAccessDetector)
	: AccessDetector(InAccessDetector)
	{
		AccessDetector.AcquireReadAccess();
	}

	FORCEINLINE ~TScopedReaderAccessDetector()
	{
		AccessDetector.ReleaseReadAccess();
	}
private:
	const RWAccessDetector& AccessDetector;
};

template<typename RWAccessDetector>
FORCEINLINE TScopedReaderAccessDetector<RWAccessDetector> MakeScopedReaderAccessDetector(RWAccessDetector& InAccessDetector)
{
	return TScopedReaderAccessDetector<RWAccessDetector>(InAccessDetector);
}

template<typename RWAccessDetector>
struct TScopedWriterDetector : public FBaseScopedAccessDetector
{
public:

	FORCEINLINE TScopedWriterDetector(const RWAccessDetector& InAccessDetector)
		: AccessDetector(InAccessDetector)
	{
		AccessDetector.AcquireWriteAccess();
	}

	FORCEINLINE ~TScopedWriterDetector()
	{
		AccessDetector.ReleaseWriteAccess();
	}
private:
	const RWAccessDetector& AccessDetector;
};

template<typename RWAccessDetector>
FORCEINLINE TScopedWriterDetector<RWAccessDetector> MakeScopedWriterAccessDetector(RWAccessDetector& InAccessDetector)
{
	return TScopedWriterDetector<RWAccessDetector>(InAccessDetector);
}

#define UE_MT_DECLARE_RW_ACCESS_DETECTOR(AccessDetector) FRWAccessDetector AccessDetector;
#define UE_MT_DECLARE_RW_RECURSIVE_ACCESS_DETECTOR(AccessDetector) FRWRecursiveAccessDetector AccessDetector;
#define UE_MT_DECLARE_RW_FULLY_RECURSIVE_ACCESS_DETECTOR(AccessDetector) FRWFullyRecursiveAccessDetector AccessDetector;

#define UE_MT_SCOPED_READ_ACCESS(AccessDetector) const FBaseScopedAccessDetector& PREPROCESSOR_JOIN(ScopedMTAccessDetector_,__LINE__) = MakeScopedReaderAccessDetector(AccessDetector);
#define UE_MT_SCOPED_WRITE_ACCESS(AccessDetector) const FBaseScopedAccessDetector& PREPROCESSOR_JOIN(ScopedMTAccessDetector_,__LINE__) = MakeScopedWriterAccessDetector(AccessDetector);

#define UE_MT_ACQUIRE_READ_ACCESS(AccessDetector) (AccessDetector).AcquireReadAccess();
#define UE_MT_RELEASE_READ_ACCESS(AccessDetector) (AccessDetector).ReleaseReadAccess();
#define UE_MT_ACQUIRE_WRITE_ACCESS(AccessDetector) (AccessDetector).AcquireWriteAccess();
#define UE_MT_RELEASE_WRITE_ACCESS(AccessDetector) (AccessDetector).ReleaseWriteAccess();

#else // ENABLE_MT_DETECTOR

#define UE_MT_DECLARE_RW_ACCESS_DETECTOR(AccessDetector)
#define UE_MT_DECLARE_RW_RECURSIVE_ACCESS_DETECTOR(AccessDetector)
#define UE_MT_DECLARE_RW_FULLY_RECURSIVE_ACCESS_DETECTOR(AccessDetector)

#define UE_MT_SCOPED_READ_ACCESS(AccessDetector) 
#define UE_MT_SCOPED_WRITE_ACCESS(AccessDetector)

#define UE_MT_ACQUIRE_READ_ACCESS(AccessDetector)
#define UE_MT_RELEASE_READ_ACCESS(AccessDetector)
#define UE_MT_ACQUIRE_WRITE_ACCESS(AccessDetector)
#define UE_MT_RELEASE_WRITE_ACCESS(AccessDetector)

#endif // ENABLE_MT_DETECTOR
