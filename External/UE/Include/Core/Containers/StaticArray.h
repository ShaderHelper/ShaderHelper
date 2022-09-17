// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Templates/UnrealTemplate.h"
#include "Templates/UnrealTypeTraits.h"
#include "Delegates/IntegerSequence.h"

/** An array with a static number of elements. */
template <typename InElementType, uint32 NumElements, uint32 Alignment = alignof(InElementType)>
class alignas(Alignment) TStaticArray
{
public:
	using ElementType = InElementType;

	TStaticArray() 
		: Storage()
	{
	}

	UE_DEPRECATED(5.0, "Please call TStaticArray(InPlace, DefaultElement) instead.")
	explicit TStaticArray(const InElementType& DefaultElement)
		: Storage(InPlace, TMakeIntegerSequence<uint32, NumElements>(), DefaultElement)
	{
	}

	template <typename... ArgTypes>
	explicit TStaticArray(EInPlace, ArgTypes&&... Args)
		: Storage(InPlace, TMakeIntegerSequence<uint32, NumElements>(), Forward<ArgTypes>(Args)...)
	{
	}

	TStaticArray(TStaticArray&& Other) = default;
	TStaticArray(const TStaticArray& Other) = default;
	TStaticArray& operator=(TStaticArray&& Other) = default;
	TStaticArray& operator=(const TStaticArray& Other) = default;

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,TStaticArray& StaticArray)
	{
		for(uint32 Index = 0;Index < NumElements;++Index)
		{
			Ar << StaticArray[Index];
		}
		return Ar;
	}

	// Accessors.
	FORCEINLINE_DEBUGGABLE InElementType& operator[](uint32 Index)
	{
		checkSlow(Index < NumElements);
		return Storage.Elements[Index].Element;
	}

	FORCEINLINE_DEBUGGABLE const InElementType& operator[](uint32 Index) const
	{
		checkSlow(Index < NumElements);
		return Storage.Elements[Index].Element;
	}

	// Comparisons.
	friend bool operator==(const TStaticArray& A,const TStaticArray& B)
	{
		for(uint32 ElementIndex = 0;ElementIndex < NumElements;++ElementIndex)
		{
			if(!(A[ElementIndex] == B[ElementIndex]))
			{
				return false;
			}
		}
		return true;
	}

	friend bool operator!=(const TStaticArray& A,const TStaticArray& B)
	{
		for(uint32 ElementIndex = 0;ElementIndex < NumElements;++ElementIndex)
		{
			if(!(A[ElementIndex] == B[ElementIndex]))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * Returns true if the array is empty and contains no elements. 
	 *
	 * @returns True if the array is empty.
	 * @see Num
	 */
	bool IsEmpty() const
	{
		return NumElements == 0;
	}

	/** The number of elements in the array. */
	FORCEINLINE_DEBUGGABLE int32 Num() const { return NumElements; }

	/** A pointer to the first element of the array */
	FORCEINLINE_DEBUGGABLE       InElementType* GetData()       { static_assert((alignof(ElementType) % Alignment) == 0, "GetData() cannot be called on a TStaticArray with non-standard alignment"); return &Storage.Elements[0].Element; }
	FORCEINLINE_DEBUGGABLE const InElementType* GetData() const { static_assert((alignof(ElementType) % Alignment) == 0, "GetData() cannot be called on a TStaticArray with non-standard alignment"); return &Storage.Elements[0].Element; }

	/** Hash function. */
	friend uint32 GetTypeHash(const TStaticArray& Array)
	{
		uint32 Result = 0;
		for(uint32 ElementIndex = 0;ElementIndex < NumElements;++ElementIndex)
		{
			Result ^= GetTypeHash(Array[ElementIndex]);
		}
		return Result;
	}


private:

	struct alignas(Alignment) TArrayStorageElementAligned
	{
		TArrayStorageElementAligned() {}

		// Index is used to achieve pack expansion in TArrayStorage, but is unused here
		template <typename... ArgTypes>
		explicit TArrayStorageElementAligned(EInPlace, uint32 /*Index*/, ArgTypes&&... Args)
			: Element(Forward<ArgTypes>(Args)...)
		{
		}

		InElementType Element;
	};

	struct TArrayStorage
	{
		TArrayStorage()
			: Elements()
		{
		}

		template<uint32... Indices, typename... ArgTypes>
		explicit TArrayStorage(EInPlace, TIntegerSequence<uint32, Indices...>, ArgTypes&&... Args)
			: Elements{ TArrayStorageElementAligned(InPlace, Indices, Args...)... }
		{
			// The arguments are deliberately not forwarded arguments here, because we're initializing multiple elements
			// and don't want an argument to be mutated by the first element's constructor and then that moved-from state
			// be used to construct the remaining elements.
			//
			// This'll mean that it'll be a compile error to use move-only types like TUniquePtr when in-place constructing
			// TStaticArray elements, which is a natural expectation because that TUniquePtr can only transfer ownership to
			// a single element.
		}

		TArrayStorageElementAligned Elements[NumElements];
	};

	TArrayStorage Storage;


public:

	template <typename StorageElementType>
	struct FRangedForIterator
	{
		explicit FRangedForIterator(StorageElementType* InPtr)
			: Ptr(InPtr)
		{}

		auto& operator*() const
		{
			return Ptr->Element;
		}

		FRangedForIterator& operator++()
		{
			++Ptr;
			return *this;
		}

		friend bool operator!=(const FRangedForIterator& A, const FRangedForIterator& B)
		{
			return A.Ptr != B.Ptr;
		}

	private:
		StorageElementType* Ptr;
	};

	using RangedForIteratorType = FRangedForIterator<TArrayStorageElementAligned>;
	using RangedForConstIteratorType = FRangedForIterator<const TArrayStorageElementAligned>;

	/** STL-like iterators to enable range-based for loop support. */
	FORCEINLINE RangedForIteratorType		begin()			{ return RangedForIteratorType(Storage.Elements); }
	FORCEINLINE RangedForConstIteratorType	begin() const	{ return RangedForConstIteratorType(Storage.Elements); }
	FORCEINLINE RangedForIteratorType		end()			{ return RangedForIteratorType(Storage.Elements + NumElements); }
	FORCEINLINE RangedForConstIteratorType	end() const		{ return RangedForConstIteratorType(Storage.Elements + NumElements); }

};

/** Creates a static array filled with the specified value. */
template <typename InElementType, uint32 NumElements>
TStaticArray<InElementType,NumElements> MakeUniformStaticArray(typename TCallTraits<InElementType>::ParamType InValue)
{
	TStaticArray<InElementType,NumElements> Result;
	for(uint32 ElementIndex = 0;ElementIndex < NumElements;++ElementIndex)
	{
		Result[ElementIndex] = InValue;
	}
	return Result;
}

template <typename ElementType, uint32 NumElements, uint32 Alignment>
struct TIsContiguousContainer<TStaticArray<ElementType, NumElements, Alignment>>
{
	enum { Value = (alignof(ElementType) % Alignment) == 0 };
};
