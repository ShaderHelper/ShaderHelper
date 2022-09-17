// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "HAL/UnrealMemory.h"

CORE_API void* AnsiMalloc(SIZE_T Size, uint32 Alignment);
CORE_API void* AnsiRealloc(void* Ptr, SIZE_T NewSize, uint32 Alignment);
CORE_API void AnsiFree(void* Ptr);

//
// ANSI C memory allocator.
//
class FMallocAnsi final
	: public FMalloc
{
	
public:
	/**
	 * Constructor enabling low fragmentation heap on platforms supporting it.
	 */
	FMallocAnsi();

	// FMalloc interface.
	virtual void* Malloc( SIZE_T Size, uint32 Alignment ) override;

	virtual void* TryMalloc(SIZE_T Size, uint32 Alignment) override;

	virtual void* Realloc( void* Ptr, SIZE_T NewSize, uint32 Alignment ) override;

	virtual void* TryRealloc(void* Ptr, SIZE_T NewSize, uint32 Alignment) override;

	virtual void Free( void* Ptr ) override;

	virtual bool GetAllocationSize(void *Original, SIZE_T &SizeOut) override;

	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 *
	 * @return true as we're using system allocator
	 */
	virtual bool IsInternallyThreadSafe() const override;

	/**
	 * Validates the allocator's heap
	 */
	virtual bool ValidateHeap() override;

	virtual const TCHAR* GetDescriptiveName() override { return TEXT("ANSI"); }
};
