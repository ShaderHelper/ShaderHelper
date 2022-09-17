// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Containers/Array.h"
#include <initializer_list>


/**
 * Base dynamic array.
 * An untyped data array; mirrors a TArray's members, but doesn't need an exact C++ type for its elements.
 **/
template <typename AllocatorType>
class TScriptArray
	: protected AllocatorType::ForAnyElementType
{
public:

	FORCEINLINE void* GetData()
	{
		return this->GetAllocation();
	}
	FORCEINLINE const void* GetData() const
	{
		return this->GetAllocation();
	}
	FORCEINLINE bool IsValidIndex(int32 i) const
	{
		return i>=0 && i<ArrayNum;
	}
	bool IsEmpty() const
	{
		return ArrayNum == 0;
	}
	FORCEINLINE int32 Num() const
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		return ArrayNum;
	}
	UE_DEPRECATED(5.0, "TScriptArray::InsertZeroed without an alignment parameter is deprecated.")
	void InsertZeroed( int32 Index, int32 Count, int32 NumBytesPerElement )
	{
		InsertZeroed( Index, Count, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__ );
	}
	void InsertZeroed( int32 Index, int32 Count, int32 NumBytesPerElement, uint32 AlignmentOfElement )
	{
		Insert( Index, Count, NumBytesPerElement, AlignmentOfElement );
		FMemory::Memzero( (uint8*)this->GetAllocation()+Index*NumBytesPerElement, Count*NumBytesPerElement );
	}
	UE_DEPRECATED(5.0, "TScriptArray::Insert without an alignment parameter is deprecated.")
	void Insert( int32 Index, int32 Count, int32 NumBytesPerElement )
	{
		Insert(Index, Count, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}
	void Insert( int32 Index, int32 Count, int32 NumBytesPerElement, uint32 AlignmentOfElement )
	{
		check(Count>=0);
		check(ArrayNum>=0);
		check(ArrayMax>=ArrayNum);
		check(Index>=0);
		check(Index<=ArrayNum);

		const int32 OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ResizeGrow(OldNum, NumBytesPerElement, AlignmentOfElement);
		}
		FMemory::Memmove
		(
			(uint8*)this->GetAllocation() + (Index+Count )*NumBytesPerElement,
			(uint8*)this->GetAllocation() + (Index       )*NumBytesPerElement,
			                                               (OldNum-Index)*NumBytesPerElement
		);
	}
	UE_DEPRECATED(5.0, "TScriptArray::Add without an alignment parameter is deprecated.")
	int32 Add(int32 Count, int32 NumBytesPerElement)
	{
		return Add(Count, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}
	int32 Add( int32 Count, int32 NumBytesPerElement, uint32 AlignmentOfElement )
	{
		check(Count>=0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);

		const int32 OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ResizeGrow(OldNum, NumBytesPerElement, AlignmentOfElement);
		}

		return OldNum;
	}
	UE_DEPRECATED(5.0, "TScriptArray::AddZeroed without an alignment parameter is deprecated.")
	int32 AddZeroed( int32 Count, int32 NumBytesPerElement )
	{
		return AddZeroed(Count, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}
	int32 AddZeroed( int32 Count, int32 NumBytesPerElement, uint32 AlignmentOfElement )
	{
		const int32 Index = Add( Count, NumBytesPerElement, AlignmentOfElement );
		FMemory::Memzero( (uint8*)this->GetAllocation()+Index*NumBytesPerElement, Count*NumBytesPerElement );
		return Index;
	}
	UE_DEPRECATED(5.0, "TScriptArray::Shrink without an alignment parameter is deprecated.")
	void Shrink( int32 NumBytesPerElement )
	{
		Shrink(NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}
	void Shrink( int32 NumBytesPerElement, uint32 AlignmentOfElement )
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		if (ArrayNum != ArrayMax)
		{
			ResizeTo(ArrayNum, NumBytesPerElement, AlignmentOfElement);
		}
	}
	UE_DEPRECATED(5.0, "TScriptArray::MoveAssign without an alignment parameter is deprecated.")
	void MoveAssign(TScriptArray& Other, int32 NumBytesPerElement)
	{
		MoveAssign(Other, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}
	void MoveAssign(TScriptArray& Other, int32 NumBytesPerElement, uint32 AlignmentOfElement)
	{
		checkSlow(this != &Other);
		Empty(0, NumBytesPerElement, AlignmentOfElement);
		this->MoveToEmpty(Other);
		ArrayNum = Other.ArrayNum; Other.ArrayNum = 0;
		ArrayMax = Other.ArrayMax; Other.ArrayMax = 0;
	}
	UE_DEPRECATED(5.0, "TScriptArray::Empty without an alignment parameter is deprecated.")
	void Empty( int32 Slack, int32 NumBytesPerElement )
	{
		Empty(Slack, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}
	void Empty( int32 Slack, int32 NumBytesPerElement, uint32 AlignmentOfElement )
	{
		checkSlow(Slack>=0);
		ArrayNum = 0;
		if (Slack != ArrayMax)
		{
			ResizeTo(Slack, NumBytesPerElement, AlignmentOfElement);
		}
	}
	void SwapMemory(int32 A, int32 B, int32 NumBytesPerElement )
	{
		FMemory::Memswap(
			(uint8*)this->GetAllocation()+(NumBytesPerElement*A),
			(uint8*)this->GetAllocation()+(NumBytesPerElement*B),
			NumBytesPerElement
			);
	}
	TScriptArray()
	:   ArrayNum( 0 )
	,	ArrayMax( 0 )
	{
	}
	void CountBytes( FArchive& Ar, int32 NumBytesPerElement  )
	{
		Ar.CountBytes( ArrayNum*NumBytesPerElement, ArrayMax*NumBytesPerElement );
	}
	/**
	 * Returns the amount of slack in this array in elements.
	 */
	FORCEINLINE int32 GetSlack() const
	{
		return ArrayMax - ArrayNum;
	}

	UE_DEPRECATED(5.0, "TScriptArray::Remove without an alignment parameter is deprecated.")
	void Remove( int32 Index, int32 Count, int32 NumBytesPerElement  )
	{
		Remove(Index, Count, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}
	void Remove( int32 Index, int32 Count, int32 NumBytesPerElement, uint32 AlignmentOfElement )
	{
		if (Count)
		{
			checkSlow(Count >= 0);
			checkSlow(Index >= 0);
			checkSlow(Index <= ArrayNum);
			checkSlow(Index + Count <= ArrayNum);

			// Skip memmove in the common case that there is nothing to move.
			int32 NumToMove = ArrayNum - Index - Count;
			if (NumToMove)
			{
				FMemory::Memmove
					(
					(uint8*)this->GetAllocation() + (Index)* NumBytesPerElement,
					(uint8*)this->GetAllocation() + (Index + Count) * NumBytesPerElement,
					NumToMove * NumBytesPerElement
					);
			}
			ArrayNum -= Count;

			ResizeShrink(NumBytesPerElement, AlignmentOfElement);
			checkSlow(ArrayNum >= 0);
			checkSlow(ArrayMax >= ArrayNum);
		}
	}

protected:

	UE_DEPRECATED(5.0, "FScriptArray::TScriptArray without an alignment parameter is deprecated.")
	TScriptArray(int32 InNum, int32 NumBytesPerElement)
		: TScriptArray(InNum, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__)
	{
	}
	TScriptArray( int32 InNum, int32 NumBytesPerElement, uint32 AlignmentOfElement )
	:   ArrayNum( 0 )
	,	ArrayMax( InNum )

	{
		if (ArrayMax)
		{
			ResizeInit(NumBytesPerElement, AlignmentOfElement);
		}
		ArrayNum = InNum;
	}
	int32	  ArrayNum;
	int32	  ArrayMax;

	FORCENOINLINE void ResizeInit(int32 NumBytesPerElement, uint32 AlignmentOfElement)
	{
		ArrayMax = this->CalculateSlackReserve(ArrayMax, NumBytesPerElement, AlignmentOfElement);
		this->ResizeAllocation(ArrayNum, ArrayMax, NumBytesPerElement, AlignmentOfElement);
	}
	FORCENOINLINE void ResizeGrow(int32 OldNum, int32 NumBytesPerElement, uint32 AlignmentOfElement)
	{
		ArrayMax = this->CalculateSlackGrow(ArrayNum, ArrayMax, NumBytesPerElement, AlignmentOfElement);
		this->ResizeAllocation(OldNum, ArrayMax, NumBytesPerElement, AlignmentOfElement);
	}
	FORCENOINLINE void ResizeShrink(int32 NumBytesPerElement, uint32 AlignmentOfElement)
	{
		const int32 NewArrayMax = this->CalculateSlackShrink(ArrayNum, ArrayMax, NumBytesPerElement, AlignmentOfElement);
		if (NewArrayMax != ArrayMax)
		{
			ArrayMax = NewArrayMax;
			this->ResizeAllocation(ArrayNum, ArrayMax, NumBytesPerElement, AlignmentOfElement);
		}
	}
	FORCENOINLINE void ResizeTo(int32 NewMax, int32 NumBytesPerElement, uint32 AlignmentOfElement)
	{
		if (NewMax)
		{
			NewMax = this->CalculateSlackReserve(NewMax, NumBytesPerElement);
		}
		if (NewMax != ArrayMax)
		{
			ArrayMax = NewMax;
			this->ResizeAllocation(ArrayNum, ArrayMax, NumBytesPerElement, AlignmentOfElement);
		}
	}
public:
	// These should really be private, because they shouldn't be called, but there's a bunch of code
	// that needs to be fixed first.
	TScriptArray(const TScriptArray&) { check(false); }
	void operator=(const TScriptArray&) { check(false); }
};

template<typename AllocatorType> struct TIsZeroConstructType<TScriptArray<AllocatorType>> { enum { Value = true }; };

class FScriptArray : public TScriptArray<FHeapAllocator>
{
	using Super = TScriptArray<FHeapAllocator>;

public:
	FScriptArray() = default;

	UE_DEPRECATED(5.0, "FScriptArray::MoveAssign without an alignment parameter is deprecated.")
	void MoveAssign(FScriptArray& Other, int32 NumBytesPerElement)
	{
		MoveAssign(Other, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}
	void MoveAssign(FScriptArray& Other, int32 NumBytesPerElement, uint32 AlignmentOfElement)
	{
		Super::MoveAssign(Other, NumBytesPerElement, AlignmentOfElement);
	}

protected:
	UE_DEPRECATED(5.0, "FScriptArray::FScriptArray without an alignment parameter is deprecated.")
	FScriptArray(int32 InNum, int32 NumBytesPerElement)
		: TScriptArray<FHeapAllocator>(InNum, NumBytesPerElement, __STDCPP_DEFAULT_NEW_ALIGNMENT__)
	{
	}
	FScriptArray(int32 InNum, int32 NumBytesPerElement, uint32 AlignmentOfElement)
		: TScriptArray<FHeapAllocator>(InNum, NumBytesPerElement, AlignmentOfElement)
	{
	}

public:
	// These should really be private, because they shouldn't be called, but there's a bunch of code
	// that needs to be fixed first.
	FScriptArray(const FScriptArray&) { check(false); }
	void operator=(const FScriptArray&) { check(false); }
};

template<> struct TIsZeroConstructType<FScriptArray> { enum { Value = true }; };
