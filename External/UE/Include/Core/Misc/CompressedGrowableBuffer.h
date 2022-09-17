// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"

/**
 * Growable compressed buffer. Usage is to append frequently but only request and therefore decompress
 * very infrequently. The prime usage case is the memory profiler keeping track of full call stacks.
 */
struct CORE_API FCompressedGrowableBuffer
{
private:
	/**
	 * This enum and the following constructor is a workaround for VC compiler bug that prevents using attributes
	 * on constructors without inline implementation. This should be removed when the deprecated ctor is removed.
	 */
	enum EVS2015Redirector
	{
		Redirect
	};
	FCompressedGrowableBuffer(EVS2015Redirector, int32 MaxPendingBufferSize, ECompressionFlags CompressionFlags);
public:
	/**
	 * Constructor
	 *
	 * @param	MaxPendingBufferSize	Max chunk size to compress in uncompressed bytes
	 * @param	CompressionFlags		Compression flags to compress memory with
	 */
	UE_DEPRECATED(4.21, "Use FName version of FCompressedGrowableBuffer constructor")
	FCompressedGrowableBuffer(int32 MaxPendingBufferSize, ECompressionFlags CompressionFlags)
		// Make sure to remove the EVS2015Redirector constructor when this constructor is removed
		: FCompressedGrowableBuffer(EVS2015Redirector::Redirect, MaxPendingBufferSize, CompressionFlags)
	{}
	FCompressedGrowableBuffer(int32 MaxPendingBufferSize, FName COmpressionFormat, ECompressionFlags CompressionFlags=COMPRESS_None);

	/**
	 * Locks the buffer for reading. Needs to be called before calls to Access and needs
	 * to be matched up with Unlock call.
	 */
	void Lock();
	/**
	 * Unlocks the buffer and frees temporary resources used for accessing.
	 */
	void Unlock();

	/**
	 * Appends passed in data to the buffer. The data needs to be less than the max
	 * pending buffer size. The code will assert on this assumption.	 
	 *
	 * @param	Data	Data to append
	 * @param	Size	Size of data in bytes.
	 * @return	Offset of data, used for retrieval later on
	 */
	int32 Append( void* Data, int32 Size );

	/**
	 * Accesses the data at passed in offset and returns it. The memory is read-only and
	 * memory will be freed in call to unlock. The lifetime of the data is till the next
	 * call to Unlock, Append or Access
	 *
	 * @param	Offset	Offset to return corresponding data for
	 */
	void* Access( int32 Offset );

	/**
	 * @return	Number of entries appended.
	 */
	int32 Num() const
	{
		return NumEntries;
	}

	/** 
	 * Helper function to return the amount of memory allocated by this buffer 
	 *
	 * @return number of bytes allocated by this buffer
	 */
	SIZE_T GetAllocatedSize() const
	{
		return CompressedBuffer.GetAllocatedSize()
			+ PendingCompressionBuffer.GetAllocatedSize()
			+ DecompressedBuffer.GetAllocatedSize()
			+ BookKeepingInfo.GetAllocatedSize();
	}

private:

	/** Helper structure for book keeping. */
	struct FBufferBookKeeping
	{
		/** Offset into compressed data.				*/
		int32 CompressedOffset;
		/** Size of compressed data in this chunk.		*/
		int32 CompressedSize;
		/** Offset into uncompressed data.				*/
		int32 UncompressedOffset;
		/** Size of uncompressed data in this chunk.	*/
		int32 UncompressedSize;
	};

	/** Maximum chunk size to compress in uncompressed bytes.				*/
	int32					MaxPendingBufferSize;
	/** Compression format used to compress the data.						*/
	FName				CompressionFormat;
	/** Compression flags used to compress the data.						*/
	ECompressionFlags	CompressionFlags;
	/** Current offset in uncompressed data.								*/
	int32					CurrentOffset;
	/** Number of entries in buffer.										*/
	int32					NumEntries;
	/** Compressed data.													*/
	TArray<uint8>		CompressedBuffer;
	/** Data pending compression once size limit is reached.				*/
	TArray<uint8>		PendingCompressionBuffer;
	/** Temporary decompression buffer used between Lock/ Unlock.			*/
	TArray<uint8>		DecompressedBuffer;
	/** Index into book keeping info associated with decompressed buffer.	*/
	int32					DecompressedBufferBookKeepingInfoIndex;
	/** Book keeping information for decompression/ access.					*/
	TArray<FBufferBookKeeping>	BookKeepingInfo;
};
