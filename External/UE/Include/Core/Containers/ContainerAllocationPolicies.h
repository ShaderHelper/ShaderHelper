// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Templates/TypeCompatibleBytes.h"
#include "HAL/PlatformMath.h"
#include "Templates/MemoryOps.h"
#include "Math/NumericLimits.h"
#include "Templates/IsPolymorphic.h"

// This option disables array slack for initial allocations, e.g where TArray::SetNum 
// is called. This tends to save a lot of memory with almost no measured performance cost.
// NOTE: This can cause latent memory corruption issues to become more prominent
#ifndef CONTAINER_INITIAL_ALLOC_ZERO_SLACK
#define CONTAINER_INITIAL_ALLOC_ZERO_SLACK 1 // ON
#endif

class FDefaultBitArrayAllocator;

template<int IndexSize> class TSizedDefaultAllocator;
using FDefaultAllocator = TSizedDefaultAllocator<32>;

/** branchless pointer selection
* return A ? A : B;
**/
template<typename ReferencedType>
ReferencedType* IfAThenAElseB(ReferencedType* A,ReferencedType* B);

/** branchless pointer selection based on predicate
* return PTRINT(Predicate) ? A : B;
**/
template<typename PredicateType,typename ReferencedType>
ReferencedType* IfPThenAElseB(PredicateType Predicate,ReferencedType* A,ReferencedType* B);

template <typename SizeType>
FORCEINLINE SizeType DefaultCalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T BytesPerElement, bool bAllowQuantize, uint32 Alignment = DEFAULT_ALIGNMENT)
{
	SizeType Retval;
	checkSlow(NumElements < NumAllocatedElements);

	// If the container has too much slack, shrink it to exactly fit the number of elements.
	const SizeType CurrentSlackElements = NumAllocatedElements - NumElements;
	const SIZE_T CurrentSlackBytes = (NumAllocatedElements - NumElements)*BytesPerElement;
	const bool bTooManySlackBytes = CurrentSlackBytes >= 16384;
	const bool bTooManySlackElements = 3 * NumElements < 2 * NumAllocatedElements;
	if ((bTooManySlackBytes || bTooManySlackElements) && (CurrentSlackElements > 64 || !NumElements)) //  hard coded 64 :-(
	{
		Retval = NumElements;
		if (Retval > 0)
		{
			if (bAllowQuantize)
			{
				Retval = (SizeType)(FMemory::QuantizeSize(Retval * BytesPerElement, Alignment) / BytesPerElement);
			}
		}
	}
	else
	{
		Retval = NumAllocatedElements;
	}

	return Retval;
}

template <typename SizeType>
FORCEINLINE SizeType DefaultCalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T BytesPerElement, bool bAllowQuantize, uint32 Alignment = DEFAULT_ALIGNMENT)
{
#if !defined(AGGRESSIVE_MEMORY_SAVING)
	#error "AGGRESSIVE_MEMORY_SAVING must be defined"
#endif
#if AGGRESSIVE_MEMORY_SAVING
	const SIZE_T FirstGrow = 1;
	const SIZE_T ConstantGrow = 0;
#else
	const SIZE_T FirstGrow = 4;
	const SIZE_T ConstantGrow = 16;
#endif

	SizeType Retval;
	checkSlow(NumElements > NumAllocatedElements && NumElements > 0);

	SIZE_T Grow = FirstGrow; // this is the amount for the first alloc

#if CONTAINER_INITIAL_ALLOC_ZERO_SLACK
	if (NumAllocatedElements)
	{
		// Allocate slack for the array proportional to its size.
		Grow = SIZE_T(NumElements) + 3 * SIZE_T(NumElements) / 8 + ConstantGrow;
	}
	else if (SIZE_T(NumElements) > Grow)
	{
		Grow = SIZE_T(NumElements);
	}
#else
	if (NumAllocatedElements || SIZE_T(NumElements) > Grow)
	{
		// Allocate slack for the array proportional to its size.
		Grow = SIZE_T(NumElements) + 3 * SIZE_T(NumElements) / 8 + ConstantGrow;
	}
#endif
	if (bAllowQuantize)
	{
		Retval = (SizeType)(FMemory::QuantizeSize(Grow * BytesPerElement, Alignment) / BytesPerElement);
	}
	else
	{
		Retval = (SizeType)Grow;
	}
	// NumElements and MaxElements are stored in 32 bit signed integers so we must be careful not to overflow here.
	if (NumElements > Retval)
	{
		Retval = TNumericLimits<SizeType>::Max();
	}

	return Retval;
}

template <typename SizeType>
FORCEINLINE SizeType DefaultCalculateSlackReserve(SizeType NumElements, SIZE_T BytesPerElement, bool bAllowQuantize, uint32 Alignment = DEFAULT_ALIGNMENT)
{
	SizeType Retval = NumElements;
	checkSlow(NumElements > 0);
	if (bAllowQuantize)
	{
		Retval = (SizeType)(FMemory::QuantizeSize(SIZE_T(Retval) * SIZE_T(BytesPerElement), Alignment) / BytesPerElement);
		// NumElements and MaxElements are stored in 32 bit signed integers so we must be careful not to overflow here.
		if (NumElements > Retval)
		{
			Retval = TNumericLimits<SizeType>::Max();
		}
	}

	return Retval;
}

/** A type which is used to represent a script type that is unknown at compile time. */
struct FScriptContainerElement
{
};

template <typename AllocatorType>
struct TAllocatorTraitsBase
{
	enum { SupportsMove              = false };
	enum { IsZeroConstruct           = false };
	enum { SupportsFreezeMemoryImage = false };
	enum { SupportsElementAlignment  = false };
};

template <typename AllocatorType>
struct TAllocatorTraits : TAllocatorTraitsBase<AllocatorType>
{
};

template <typename FromAllocatorType, typename ToAllocatorType>
struct TCanMoveBetweenAllocators
{
	enum { Value = false };
};

/** This is the allocation policy interface; it exists purely to document the policy's interface, and should not be used. */
class FContainerAllocatorInterface
{
public:
	/** The integral type to be used for element counts and indices used by the allocator and container - must be signed */
	using SizeType = int32;

	/** Determines whether the user of the allocator may use the ForAnyElementType inner class. */
	enum { NeedsElementType = true };

	/** Determines whether the user of the allocator should do range checks */
	enum { RequireRangeCheck = true };

	/**
	 * A class that receives both the explicit allocation policy template parameters specified by the user of the container,
	 * but also the implicit ElementType template parameter from the container type.
	 */
	template<typename ElementType>
	class ForElementType
	{
		/**
		 * Moves the state of another allocator into this one.
		 *
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		void MoveToEmpty(ForElementType& Other);

		/**
		 * Moves the state of another allocator into this one.  The allocator can be different, and the type must be specified.
		 * This function should only be called if TAllocatorTraits<AllocatorType>::SupportsMoveFromOtherAllocator is true.
		 *
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		template <typename OtherAllocatorType>
		void MoveToEmptyFromOtherAllocator(typename OtherAllocatorType::template ForElementType<ElementType>& Other);

		/** Accesses the container's current data. */
		ElementType* GetAllocation() const;

		/**
		 * Resizes the container's allocation.
		 * @param PreviousNumElements - The number of elements that were stored in the previous allocation.
		 * @param NumElements - The number of elements to allocate space for.
		 * @param NumBytesPerElement - The number of bytes/element.
		 */
		void ResizeAllocation(
			SizeType PreviousNumElements,
			SizeType NumElements,
			SIZE_T NumBytesPerElement
		);

		/**
		 * Resizes the container's allocation.
		 * @param PreviousNumElements - The number of elements that were stored in the previous allocation.
		 * @param NumElements - The number of elements to allocate space for.
		 * @param NumBytesPerElement - The number of bytes/element.
		 * @param AlignmentOfElement - The alignment of the element type.
		 *
		 * @note  This overload only exists if TAllocatorTraits<Allocator>::SupportsElementAlignment == true.
		 */
		void ResizeAllocation(
			SizeType PreviousNumElements,
			SizeType NumElements,
			SIZE_T NumBytesPerElement,
			uint32 AlignmentOfElement
		);

		/**
		 * Calculates the amount of slack to allocate for an array that has just grown or shrunk to a given number of elements.
		 * @param NumElements - The number of elements to allocate space for.
		 * @param NumBytesPerElement - The number of bytes/element.
		 */
		SizeType CalculateSlackReserve(
			SizeType NumElements,
			SIZE_T NumBytesPerElement
		) const;

		/**
		 * Calculates the amount of slack to allocate for an array that has just grown or shrunk to a given number of elements.
		 * @param NumElements - The number of elements to allocate space for.
		 * @param NumBytesPerElement - The number of bytes/element.
		 * @param AlignmentOfElement - The alignment of the element type.
		 *
		 * @note  This overload only exists if TAllocatorTraits<Allocator>::SupportsElementAlignment == true.
		 */
		SizeType CalculateSlackReserve(
			SizeType NumElements,
			SIZE_T NumBytesPerElement,
			uint32 AlignmentOfElement
		) const;

		/**
		 * Calculates the amount of slack to allocate for an array that has just shrunk to a given number of elements.
		 * @param NumElements - The number of elements to allocate space for.
		 * @param CurrentNumSlackElements - The current number of elements allocated.
		 * @param NumBytesPerElement - The number of bytes/element.
		 */
		SizeType CalculateSlackShrink(
			SizeType NumElements,
			SizeType CurrentNumSlackElements,
			SIZE_T NumBytesPerElement
			) const;

		/**
		 * Calculates the amount of slack to allocate for an array that has just shrunk to a given number of elements.
		 * @param NumElements - The number of elements to allocate space for.
		 * @param CurrentNumSlackElements - The current number of elements allocated.
		 * @param NumBytesPerElement - The number of bytes/element.
		 * @param AlignmentOfElement - The alignment of the element type.
		 *
		 * @note  This overload only exists if TAllocatorTraits<Allocator>::SupportsElementAlignment == true.
		 */
		SizeType CalculateSlackShrink(
			SizeType NumElements,
			SizeType CurrentNumSlackElements,
			SIZE_T NumBytesPerElement,
			uint32 AlignmentOfElement
		) const;

		/**
		 * Calculates the amount of slack to allocate for an array that has just grown to a given number of elements.
		 * @param NumElements - The number of elements to allocate space for.
		 * @param CurrentNumSlackElements - The current number of elements allocated.
		 * @param NumBytesPerElement - The number of bytes/element.
		 */
		SizeType CalculateSlackGrow(
			SizeType NumElements,
			SizeType CurrentNumSlackElements,
			SIZE_T NumBytesPerElement
		) const;

		/**
		 * Calculates the amount of slack to allocate for an array that has just grown to a given number of elements.
		 * @param NumElements - The number of elements to allocate space for.
		 * @param CurrentNumSlackElements - The current number of elements allocated.
		 * @param NumBytesPerElement - The number of bytes/element.
		 * @param AlignmentOfElement - The alignment of the element type.
		 *
		 * @note  This overload only exists if TAllocatorTraits<Allocator>::SupportsElementAlignment == true.
		 */
		SizeType CalculateSlackGrow(
			SizeType NumElements,
			SizeType CurrentNumSlackElements,
			SIZE_T NumBytesPerElement,
			uint32 AlignmentOfElement
		) const;

		/**
		 * Returns the size of any requested heap allocation currently owned by the allocator.
		 * @param NumAllocatedElements - The number of elements allocated by the container.
		 * @param NumBytesPerElement - The number of bytes/element.
		 */
		SIZE_T GetAllocatedSize(SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const;

		/** Returns true if the allocator has made any heap allocations */
		bool HasAllocation() const;

		/** Returns number of pre-allocated elements the container can use before allocating more space */
		SizeType GetInitialCapacity() const;
	};

	/**
	 * A class that may be used when NeedsElementType=false is specified.
	 * If NeedsElementType=true, then this must be present but will not be used, and so can simply be a typedef to void
	 */
	typedef ForElementType<FScriptContainerElement> ForAnyElementType;
};

/** The indirect allocation policy always allocates the elements indirectly. */
template<uint32 Alignment = DEFAULT_ALIGNMENT>
class TAlignedHeapAllocator
{
public:
	using SizeType = int32;

	enum { NeedsElementType = false };
	enum { RequireRangeCheck = true };

	class ForAnyElementType
	{
	public:

		/** Default constructor. */
		ForAnyElementType()
			: Data(nullptr)
		{}

		/**
		 * Moves the state of another allocator into this one.
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		FORCEINLINE void MoveToEmpty(ForAnyElementType& Other)
		{
			checkSlow(this != &Other);

			if (Data)
			{
				FMemory::Free(Data);
			}

			Data       = Other.Data;
			Other.Data = nullptr;
		}

		/** Destructor. */
		FORCEINLINE ~ForAnyElementType()
		{
			if(Data)
			{
				FMemory::Free(Data);
			}
		}

		// FContainerAllocatorInterface
		FORCEINLINE FScriptContainerElement* GetAllocation() const
		{
			return Data;
		}
		void ResizeAllocation(
			SizeType PreviousNumElements,
			SizeType NumElements,
			SIZE_T NumBytesPerElement
			)
		{
			// Avoid calling FMemory::Realloc( nullptr, 0 ) as ANSI C mandates returning a valid pointer which is not what we want.
			if (Data || NumElements)
			{
				//checkSlow(((uint64)NumElements*(uint64)ElementTypeInfo.GetSize() < (uint64)INT_MAX));
				Data = (FScriptContainerElement*)FMemory::Realloc( Data, NumElements*NumBytesPerElement, Alignment );
			}
		}
		FORCEINLINE SizeType CalculateSlackReserve(SizeType NumElements, SIZE_T NumBytesPerElement) const
		{
			return DefaultCalculateSlackReserve(NumElements, NumBytesPerElement, true, Alignment);
		}
		FORCEINLINE SizeType CalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return DefaultCalculateSlackShrink(NumElements, NumAllocatedElements, NumBytesPerElement, true, Alignment);
		}
		FORCEINLINE SizeType CalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return DefaultCalculateSlackGrow(NumElements, NumAllocatedElements, NumBytesPerElement, true, Alignment);
		}

		SIZE_T GetAllocatedSize(SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return NumAllocatedElements * NumBytesPerElement;
		}

		bool HasAllocation() const
		{
			return !!Data;
		}

		SizeType GetInitialCapacity() const
		{
			return 0;
		}

	private:
		ForAnyElementType(const ForAnyElementType&);
		ForAnyElementType& operator=(const ForAnyElementType&);

		/** A pointer to the container's elements. */
		FScriptContainerElement* Data;
	};

	template<typename ElementType>
	class ForElementType : public ForAnyElementType
	{
		static constexpr SIZE_T MinimumAlignment = (Alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__) ? __STDCPP_DEFAULT_NEW_ALIGNMENT__ : Alignment;

	public:
		/** Default constructor. */
		ForElementType()
		{
			UE_STATIC_DEPRECATE(5.0, alignof(ElementType) > MinimumAlignment, "Using TAlignedHeapAllocator with an alignment lower than the element type's alignment - please update the alignment parameter");
		}

		FORCEINLINE ElementType* GetAllocation() const
		{
			return (ElementType*)ForAnyElementType::GetAllocation();
		}
	};
};

template <uint32 Alignment>
struct TAllocatorTraits<TAlignedHeapAllocator<Alignment>> : TAllocatorTraitsBase<TAlignedHeapAllocator<Alignment>>
{
	enum { SupportsMove    = true };
	enum { IsZeroConstruct = true };
};

template <int IndexSize>
struct TBitsToSizeType
{
	static_assert(IndexSize, "Unsupported allocator index size.");
};

template <> struct TBitsToSizeType<8>  { using Type = int8; };
template <> struct TBitsToSizeType<16> { using Type = int16; };
template <> struct TBitsToSizeType<32> { using Type = int32; };
template <> struct TBitsToSizeType<64> { using Type = int64; };

/** The indirect allocation policy always allocates the elements indirectly. */
template <int IndexSize>
class TSizedHeapAllocator
{
public:
	using SizeType = typename TBitsToSizeType<IndexSize>::Type;

	enum { NeedsElementType = true };
	enum { RequireRangeCheck = true };

	class ForAnyElementType
	{
		template <int>
		friend class TSizedHeapAllocator;

	public:
		/** Default constructor. */
		ForAnyElementType()
			: Data(nullptr)
		{}

		/**
		 * Moves the state of another allocator into this one.  The allocator can be different.
		 *
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		template <typename OtherAllocator>
		FORCEINLINE void MoveToEmptyFromOtherAllocator(typename OtherAllocator::ForAnyElementType& Other)
		{
			checkSlow((void*)this != (void*)&Other);

			if (Data)
			{
				FMemory::Free(Data);
			}

			Data = Other.Data;
			Other.Data = nullptr;
		}

		/**
		 * Moves the state of another allocator into this one.
		 * Moves the state of another allocator into this one.  The allocator can be different.
		 *
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		FORCEINLINE void MoveToEmpty(ForAnyElementType& Other)
		{
			this->MoveToEmptyFromOtherAllocator<TSizedHeapAllocator>(Other);
		}

		/** Destructor. */
		FORCEINLINE ~ForAnyElementType()
		{
			if(Data)
			{
				FMemory::Free(Data);
			}
		}

		// FContainerAllocatorInterface
		FORCEINLINE FScriptContainerElement* GetAllocation() const
		{
			return Data;
		}
		FORCEINLINE void ResizeAllocation(SizeType PreviousNumElements, SizeType NumElements, SIZE_T NumBytesPerElement)
		{
			// Avoid calling FMemory::Realloc( nullptr, 0 ) as ANSI C mandates returning a valid pointer which is not what we want.
			if (Data || NumElements)
			{
				//checkSlow(((uint64)NumElements*(uint64)ElementTypeInfo.GetSize() < (uint64)INT_MAX));
				Data = (FScriptContainerElement*)FMemory::Realloc( Data, NumElements*NumBytesPerElement );
			}
		}
		FORCEINLINE void ResizeAllocation(SizeType PreviousNumElements, SizeType NumElements, SIZE_T NumBytesPerElement, uint32 AlignmentOfElement)
		{
			// Avoid calling FMemory::Realloc( nullptr, 0 ) as ANSI C mandates returning a valid pointer which is not what we want.
			if (Data || NumElements)
			{
				//checkSlow(((uint64)NumElements*(uint64)ElementTypeInfo.GetSize() < (uint64)INT_MAX));
				Data = (FScriptContainerElement*)FMemory::Realloc( Data, NumElements*NumBytesPerElement, AlignmentOfElement );
			}
		}
		FORCEINLINE SizeType CalculateSlackReserve(SizeType NumElements, SIZE_T NumBytesPerElement) const
		{
			return DefaultCalculateSlackReserve(NumElements, NumBytesPerElement, true);
		}
		FORCEINLINE SizeType CalculateSlackReserve(SizeType NumElements, SIZE_T NumBytesPerElement, uint32 AlignmentOfElement) const
		{
			return DefaultCalculateSlackReserve(NumElements, NumBytesPerElement, true, (uint32)AlignmentOfElement);
		}
		FORCEINLINE SizeType CalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return DefaultCalculateSlackShrink(NumElements, NumAllocatedElements, NumBytesPerElement, true);
		}
		FORCEINLINE SizeType CalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement, uint32 AlignmentOfElement) const
		{
			return DefaultCalculateSlackShrink(NumElements, NumAllocatedElements, NumBytesPerElement, true, (uint32)AlignmentOfElement);
		}
		FORCEINLINE SizeType CalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return DefaultCalculateSlackGrow(NumElements, NumAllocatedElements, NumBytesPerElement, true);
		}
		FORCEINLINE SizeType CalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement, uint32 AlignmentOfElement) const
		{
			return DefaultCalculateSlackGrow(NumElements, NumAllocatedElements, NumBytesPerElement, true, (uint32)AlignmentOfElement);
		}

		SIZE_T GetAllocatedSize(SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return NumAllocatedElements * NumBytesPerElement;
		}

		bool HasAllocation() const
		{
			return !!Data;
		}

		SizeType GetInitialCapacity() const
		{
			return 0;
		}

	private:
		ForAnyElementType(const ForAnyElementType&);
		ForAnyElementType& operator=(const ForAnyElementType&);

		/** A pointer to the container's elements. */
		FScriptContainerElement* Data;
	};
	
	template<typename ElementType>
	class ForElementType : public ForAnyElementType
	{
	public:
		/** Default constructor. */
		ForElementType()
		{
		}

		FORCEINLINE ElementType* GetAllocation() const
		{
			return (ElementType*)ForAnyElementType::GetAllocation();
		}
	};
};

template <uint8 IndexSize>
struct TAllocatorTraits<TSizedHeapAllocator<IndexSize>> : TAllocatorTraitsBase<TSizedHeapAllocator<IndexSize>>
{
	enum { SupportsMove             = true };
	enum { IsZeroConstruct          = true };
	enum { SupportsElementAlignment = true };
};

using FHeapAllocator = TSizedHeapAllocator<32>;

template <uint8 FromIndexSize, uint8 ToIndexSize>
struct TCanMoveBetweenAllocators<TSizedHeapAllocator<FromIndexSize>, TSizedHeapAllocator<ToIndexSize>>
{
	// Allow conversions between different int width versions of the allocator
	enum { Value = true };
};

/**
 * The inline allocation policy allocates up to a specified number of elements in the same allocation as the container.
 * Any allocation needed beyond that causes all data to be moved into an indirect allocation.
 * It always uses DEFAULT_ALIGNMENT.
 */
template <uint32 NumInlineElements, int IndexSize, typename SecondaryAllocator = FDefaultAllocator>
class TSizedInlineAllocator
{
public:
	using SizeType = typename TBitsToSizeType<IndexSize>::Type;

	static_assert(std::is_same_v<SizeType, typename SecondaryAllocator::SizeType>, "Secondary allocator SizeType mismatch");

	enum { NeedsElementType = true };
	enum { RequireRangeCheck = true };

	template<typename ElementType>
	class ForElementType
	{
	public:

		/** Default constructor. */
		ForElementType()
		{
		}

		/**
		 * Moves the state of another allocator into this one.
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		FORCEINLINE void MoveToEmpty(ForElementType& Other)
		{
			checkSlow(this != &Other);

			if (!Other.SecondaryData.GetAllocation())
			{
				// Relocate objects from other inline storage only if it was stored inline in Other
				RelocateConstructItems<ElementType>((void*)InlineData, Other.GetInlineElements(), NumInlineElements);
			}

			// Move secondary storage in any case.
			// This will move secondary storage if it exists but will also handle the case where secondary storage is used in Other but not in *this.
			SecondaryData.MoveToEmpty(Other.SecondaryData);
		}

		// FContainerAllocatorInterface
		FORCEINLINE ElementType* GetAllocation() const
		{
			return IfAThenAElseB<ElementType>(SecondaryData.GetAllocation(),GetInlineElements());
		}

		void ResizeAllocation(SizeType PreviousNumElements, SizeType NumElements,SIZE_T NumBytesPerElement)
		{
			// Check if the new allocation will fit in the inline data area.
			if(NumElements <= NumInlineElements)
			{
				// If the old allocation wasn't in the inline data area, relocate it into the inline data area.
				if(SecondaryData.GetAllocation())
				{
					RelocateConstructItems<ElementType>((void*)InlineData, (ElementType*)SecondaryData.GetAllocation(), PreviousNumElements);

					// Free the old indirect allocation.
					SecondaryData.ResizeAllocation(0,0,NumBytesPerElement);
				}
			}
			else
			{
				if(!SecondaryData.GetAllocation())
				{
					// Allocate new indirect memory for the data.
					SecondaryData.ResizeAllocation(0,NumElements,NumBytesPerElement);

					// Move the data out of the inline data area into the new allocation.
					RelocateConstructItems<ElementType>((void*)SecondaryData.GetAllocation(), GetInlineElements(), PreviousNumElements);
				}
				else
				{
					// Reallocate the indirect data for the new size.
					SecondaryData.ResizeAllocation(PreviousNumElements, NumElements, NumBytesPerElement);
				}
			}
		}

		FORCEINLINE SizeType CalculateSlackReserve(SizeType NumElements, SIZE_T NumBytesPerElement) const
		{
			// If the elements use less space than the inline allocation, only use the inline allocation as slack.
			return NumElements <= NumInlineElements ?
				NumInlineElements :
				SecondaryData.CalculateSlackReserve(NumElements, NumBytesPerElement);
		}
		FORCEINLINE SizeType CalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			// If the elements use less space than the inline allocation, only use the inline allocation as slack.
			return NumElements <= NumInlineElements ?
				NumInlineElements :
				SecondaryData.CalculateSlackShrink(NumElements, NumAllocatedElements, NumBytesPerElement);
		}
		FORCEINLINE SizeType CalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			// If the elements use less space than the inline allocation, only use the inline allocation as slack.
			// Also, when computing slack growth, don't count inline elements -- the slack algorithm has a special
			// case to save memory on the initial heap allocation, versus subsequent reallocations, and we don't
			// want the inline elements to be treated as if they were the first heap allocation.
			return NumElements <= NumInlineElements ?
				NumInlineElements :
				SecondaryData.CalculateSlackGrow(NumElements, NumAllocatedElements <= NumInlineElements ? 0 : NumAllocatedElements, NumBytesPerElement);
		}

		SIZE_T GetAllocatedSize(SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			if (NumAllocatedElements > NumInlineElements)
			{
				return SecondaryData.GetAllocatedSize(NumAllocatedElements, NumBytesPerElement);
			}
			return 0;
		}

		bool HasAllocation() const
		{
			return SecondaryData.HasAllocation();
		}

		SizeType GetInitialCapacity() const
		{
			return NumInlineElements;
		}

	private:
		ForElementType(const ForElementType&);
		ForElementType& operator=(const ForElementType&);

		/** The data is stored in this array if less than NumInlineElements is needed. */
		TTypeCompatibleBytes<ElementType> InlineData[NumInlineElements];

		/** The data is allocated through the indirect allocation policy if more than NumInlineElements is needed. */
		typename SecondaryAllocator::template ForElementType<ElementType> SecondaryData;

		/** @return the base of the aligned inline element data */
		ElementType* GetInlineElements() const
		{
			return (ElementType*)InlineData;
		}
	};

	typedef void ForAnyElementType;
};

template <uint32 NumInlineElements, int IndexSize, typename SecondaryAllocator>
struct TAllocatorTraits<TSizedInlineAllocator<NumInlineElements, IndexSize, SecondaryAllocator>> : TAllocatorTraitsBase<TSizedInlineAllocator<NumInlineElements, IndexSize, SecondaryAllocator>>
{
	enum { SupportsMove = TAllocatorTraits<SecondaryAllocator>::SupportsMove };
};

template <uint32 NumInlineElements, typename SecondaryAllocator = FDefaultAllocator>
using TInlineAllocator = TSizedInlineAllocator<NumInlineElements, 32, SecondaryAllocator>;

template <uint32 NumInlineElements, typename SecondaryAllocator = FDefaultAllocator64>
using TInlineAllocator64 = TSizedInlineAllocator<NumInlineElements, 64, SecondaryAllocator>;

/**
 * Implements a variant of TInlineAllocator with a secondary heap allocator that is allowed to store a pointer to its inline elements.
 * This allows caching a pointer to the elements which avoids any conditional logic in GetAllocation(), but prevents the allocator being trivially relocatable.
 * All UE4 allocators typically rely on elements being trivially relocatable, so instances of this allocator cannot be used in other containers.
 */
template <uint32 NumInlineElements>
class TNonRelocatableInlineAllocator
{
public:
	using SizeType = int32;

	enum { NeedsElementType = true };
	enum { RequireRangeCheck = true };

	template<typename ElementType>
	class ForElementType
	{
	public:
		/** Default constructor. */
		ForElementType()
			: Data(GetInlineElements())
		{
			UE_STATIC_DEPRECATE(5.0, alignof(ElementType) > __STDCPP_DEFAULT_NEW_ALIGNMENT__, "TNonRelocatableInlineAllocator uses GMalloc's default alignment, which is lower than the element type's alignment - please consider a different approach");
		}

		/**
		 * Moves the state of another allocator into this one.
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		FORCEINLINE void MoveToEmpty(ForElementType& Other)
		{
			checkSlow(this != &Other);

			if (HasAllocation())
			{
				FMemory::Free(Data);
			}

			if (Other.HasAllocation())
			{
				Data = Other.Data;
				Other.Data = nullptr;
			}
			else
			{
				Data = GetInlineElements();
				RelocateConstructItems<ElementType>(GetInlineElements(), Other.GetInlineElements(), NumInlineElements);
			}
		}

		// FContainerAllocatorInterface
		FORCEINLINE ElementType* GetAllocation() const
		{
			return Data;
		}

		void ResizeAllocation(SizeType PreviousNumElements, SizeType NumElements,SIZE_T NumBytesPerElement)
		{
			// Check if the new allocation will fit in the inline data area.
			if(NumElements <= NumInlineElements)
			{
				// If the old allocation wasn't in the inline data area, relocate it into the inline data area.
				if(HasAllocation())
				{
					RelocateConstructItems<ElementType>(GetInlineElements(), Data, PreviousNumElements);
					FMemory::Free(Data);
					Data = GetInlineElements();
				}
			}
			else
			{
				if (HasAllocation())
				{
					// Reallocate the indirect data for the new size.
					Data = (ElementType*)FMemory::Realloc(Data, NumElements*NumBytesPerElement);
				}
				else
				{
					// Allocate new indirect memory for the data.
					Data = (ElementType*)FMemory::Realloc(nullptr, NumElements*NumBytesPerElement);

					// Move the data out of the inline data area into the new allocation.
					RelocateConstructItems<ElementType>(Data, GetInlineElements(), PreviousNumElements);
				}
			}
		}

		FORCEINLINE SizeType CalculateSlackReserve(SizeType NumElements, SIZE_T NumBytesPerElement) const
		{
			// If the elements use less space than the inline allocation, only use the inline allocation as slack.
			return (NumElements <= NumInlineElements) ? NumInlineElements : DefaultCalculateSlackReserve(NumElements, NumBytesPerElement, true);
		}

		FORCEINLINE SizeType CalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			// If the elements use less space than the inline allocation, only use the inline allocation as slack.
			return (NumElements <= NumInlineElements) ? NumInlineElements : DefaultCalculateSlackShrink(NumElements, NumAllocatedElements, NumBytesPerElement, true);
		}

		FORCEINLINE SizeType CalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			// If the elements use less space than the inline allocation, only use the inline allocation as slack.
			return (NumElements <= NumInlineElements) ? NumInlineElements : DefaultCalculateSlackGrow(NumElements, NumAllocatedElements, NumBytesPerElement, true);
		}

		SIZE_T GetAllocatedSize(SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return HasAllocation()? (NumAllocatedElements * NumBytesPerElement) : 0;
		}

		FORCEINLINE bool HasAllocation() const
		{
			return Data != GetInlineElements();
		}

		SizeType GetInitialCapacity() const
		{
			return NumInlineElements;
		}

	private:
		ForElementType(const ForElementType&) = delete;
		ForElementType& operator=(const ForElementType&) = delete;

		/** The data is allocated through the indirect allocation policy if more than NumInlineElements is needed. */
		ElementType* Data;

		/** The data is stored in this array if less than NumInlineElements is needed. */
		TTypeCompatibleBytes<ElementType> InlineData[NumInlineElements];

		/** @return the base of the aligned inline element data */
		FORCEINLINE ElementType* GetInlineElements() const
		{
			return (ElementType*)InlineData;
		}
	};

	typedef void ForAnyElementType;
};

template <uint32 NumInlineElements>
struct TAllocatorTraits<TNonRelocatableInlineAllocator<NumInlineElements>> : TAllocatorTraitsBase<TNonRelocatableInlineAllocator<NumInlineElements>>
{
	enum { SupportsMove = true };
};

/**
 * The fixed allocation policy allocates up to a specified number of elements in the same allocation as the container.
 * It's like the inline allocator, except it doesn't provide secondary storage when the inline storage has been filled.
 */
template <uint32 NumInlineElements>
class TFixedAllocator
{
public:
	using SizeType = int32;

	enum { NeedsElementType = true };
	enum { RequireRangeCheck = true };

	template<typename ElementType>
	class ForElementType
	{
	public:

		/** Default constructor. */
		ForElementType()
		{
		}

		/**
		 * Moves the state of another allocator into this one.
		 * Assumes that the allocator is currently empty, i.e. memory may be allocated but any existing elements have already been destructed (if necessary).
		 * @param Other - The allocator to move the state from.  This allocator should be left in a valid empty state.
		 */
		FORCEINLINE void MoveToEmpty(ForElementType& Other)
		{
			checkSlow(this != &Other);

			// Relocate objects from other inline storage
			RelocateConstructItems<ElementType>((void*)InlineData, Other.GetInlineElements(), NumInlineElements);
		}

		// FContainerAllocatorInterface
		FORCEINLINE ElementType* GetAllocation() const
		{
			return GetInlineElements();
		}

		void ResizeAllocation(SizeType PreviousNumElements, SizeType NumElements,SIZE_T NumBytesPerElement)
		{
			// Ensure the requested allocation will fit in the inline data area.
			check(NumElements <= NumInlineElements);
		}

		FORCEINLINE SizeType CalculateSlackReserve(SizeType NumElements, SIZE_T NumBytesPerElement) const
		{
			// Ensure the requested allocation will fit in the inline data area.
			check(NumElements <= NumInlineElements);
			return NumInlineElements;
		}
		FORCEINLINE SizeType CalculateSlackShrink(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			// Ensure the requested allocation will fit in the inline data area.
			check(NumAllocatedElements <= NumInlineElements);
			return NumInlineElements;
		}
		FORCEINLINE SizeType CalculateSlackGrow(SizeType NumElements, SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			// Ensure the requested allocation will fit in the inline data area.
			check(NumElements <= NumInlineElements);
			return NumInlineElements;
		}

		SIZE_T GetAllocatedSize(SizeType NumAllocatedElements, SIZE_T NumBytesPerElement) const
		{
			return 0;
		}

		bool HasAllocation() const
		{
			return false;
		}

		SizeType GetInitialCapacity() const
		{
			return NumInlineElements;
		}

	private:
		ForElementType(const ForElementType&);
		ForElementType& operator=(const ForElementType&);

		/** The data is stored in this array if less than NumInlineElements is needed. */
		TTypeCompatibleBytes<ElementType> InlineData[NumInlineElements];

		/** @return the base of the aligned inline element data */
		ElementType* GetInlineElements() const
		{
			return (ElementType*)InlineData;
		}
	};

	typedef void ForAnyElementType;
};

template <uint32 NumInlineElements>
struct TAllocatorTraits<TFixedAllocator<NumInlineElements>> : TAllocatorTraitsBase<TFixedAllocator<NumInlineElements>>
{
	enum { SupportsMove = true };
};

// We want these to be correctly typed as int32, but we don't want them to have linkage, so we make them macros
#define NumBitsPerDWORD ((int32)32)
#define NumBitsPerDWORDLogTwo ((int32)5)

//
// Sparse array allocation definitions
//

/** Encapsulates the allocators used by a sparse array in a single type. */
template<typename InElementAllocator = FDefaultAllocator,typename InBitArrayAllocator = FDefaultBitArrayAllocator>
class TSparseArrayAllocator
{
public:

	typedef InElementAllocator ElementAllocator;
	typedef InBitArrayAllocator BitArrayAllocator;
};

template <uint32 Alignment = DEFAULT_ALIGNMENT, typename InElementAllocator = TAlignedHeapAllocator<Alignment>,typename InBitArrayAllocator = FDefaultBitArrayAllocator>
class TAlignedSparseArrayAllocator
{
public:
	typedef InElementAllocator ElementAllocator;
	typedef InBitArrayAllocator BitArrayAllocator;
};

/** An inline sparse array allocator that allows sizing of the inline allocations for a set number of elements. */
template<
	uint32 NumInlineElements,
	typename SecondaryAllocator = TSparseArrayAllocator<FDefaultAllocator,FDefaultAllocator>
	>
class TInlineSparseArrayAllocator
{
private:

	/** The size to allocate inline for the bit array. */
	enum { InlineBitArrayDWORDs = (NumInlineElements + NumBitsPerDWORD - 1) / NumBitsPerDWORD};

public:

	typedef TInlineAllocator<NumInlineElements,typename SecondaryAllocator::ElementAllocator>		ElementAllocator;
	typedef TInlineAllocator<InlineBitArrayDWORDs,typename SecondaryAllocator::BitArrayAllocator>	BitArrayAllocator;
};

/** An inline sparse array allocator that doesn't have any secondary storage. */
template <uint32 NumInlineElements>
class TFixedSparseArrayAllocator
{
private:

	/** The size to allocate inline for the bit array. */
	enum { InlineBitArrayDWORDs = (NumInlineElements + NumBitsPerDWORD - 1) / NumBitsPerDWORD};

public:

	typedef TFixedAllocator<NumInlineElements>    ElementAllocator;
	typedef TFixedAllocator<InlineBitArrayDWORDs> BitArrayAllocator;
};



//
// Set allocation definitions.
//

#define DEFAULT_NUMBER_OF_ELEMENTS_PER_HASH_BUCKET	2
#define DEFAULT_BASE_NUMBER_OF_HASH_BUCKETS			8
#define DEFAULT_MIN_NUMBER_OF_HASHED_ELEMENTS		4

/** Encapsulates the allocators used by a set in a single type. */
template<
	typename InSparseArrayAllocator               = TSparseArrayAllocator<>,
	typename InHashAllocator                      = TInlineAllocator<1,FDefaultAllocator>,
	uint32   AverageNumberOfElementsPerHashBucket = DEFAULT_NUMBER_OF_ELEMENTS_PER_HASH_BUCKET,
	uint32   BaseNumberOfHashBuckets              = DEFAULT_BASE_NUMBER_OF_HASH_BUCKETS,
	uint32   MinNumberOfHashedElements            = DEFAULT_MIN_NUMBER_OF_HASHED_ELEMENTS
	>
class TSetAllocator
{
public:

	/** Computes the number of hash buckets to use for a given number of elements. */
	static FORCEINLINE uint32 GetNumberOfHashBuckets(uint32 NumHashedElements)
	{
		if(NumHashedElements >= MinNumberOfHashedElements)
		{
			return FPlatformMath::RoundUpToPowerOfTwo(NumHashedElements / AverageNumberOfElementsPerHashBucket + BaseNumberOfHashBuckets);
		}

		return 1;
	}

	typedef InSparseArrayAllocator SparseArrayAllocator;
	typedef InHashAllocator        HashAllocator;
};

template<
	typename InSparseArrayAllocator,
	typename InHashAllocator,
	uint32   AverageNumberOfElementsPerHashBucket,
	uint32   BaseNumberOfHashBuckets,
	uint32   MinNumberOfHashedElements
>
struct TAllocatorTraits<TSetAllocator<InSparseArrayAllocator, InHashAllocator, AverageNumberOfElementsPerHashBucket, BaseNumberOfHashBuckets, MinNumberOfHashedElements>> :
	TAllocatorTraitsBase<TSetAllocator<InSparseArrayAllocator, InHashAllocator, AverageNumberOfElementsPerHashBucket, BaseNumberOfHashBuckets, MinNumberOfHashedElements>>
{
	enum
	{
		SupportsFreezeMemoryImage = TAllocatorTraits<InSparseArrayAllocator>::SupportsFreezeMemoryImage && TAllocatorTraits<InHashAllocator>::SupportsFreezeMemoryImage,
	};
};

/** An inline set allocator that allows sizing of the inline allocations for a set number of elements. */
template<
	uint32   NumInlineElements,
	typename SecondaryAllocator                   = TSetAllocator<TSparseArrayAllocator<FDefaultAllocator,FDefaultAllocator>,FDefaultAllocator>,
	uint32   AverageNumberOfElementsPerHashBucket = DEFAULT_NUMBER_OF_ELEMENTS_PER_HASH_BUCKET,
	uint32   MinNumberOfHashedElements            = DEFAULT_MIN_NUMBER_OF_HASHED_ELEMENTS
	>
class TInlineSetAllocator
{
private:

	enum { NumInlineHashBuckets = (NumInlineElements + AverageNumberOfElementsPerHashBucket - 1) / AverageNumberOfElementsPerHashBucket };

	static_assert(NumInlineHashBuckets > 0 && !(NumInlineHashBuckets & (NumInlineHashBuckets - 1)), "Number of inline buckets must be a power of two");

public:

	/** Computes the number of hash buckets to use for a given number of elements. */
	static FORCEINLINE uint32 GetNumberOfHashBuckets(uint32 NumHashedElements)
	{
		const uint32 NumDesiredHashBuckets = FPlatformMath::RoundUpToPowerOfTwo(NumHashedElements / AverageNumberOfElementsPerHashBucket);
		if (NumDesiredHashBuckets < NumInlineHashBuckets)
		{
			return NumInlineHashBuckets;
		}

		if (NumHashedElements < MinNumberOfHashedElements)
		{
			return NumInlineHashBuckets;
		}

		return NumDesiredHashBuckets;
	}

	typedef TInlineSparseArrayAllocator<NumInlineElements,typename SecondaryAllocator::SparseArrayAllocator> SparseArrayAllocator;
	typedef TInlineAllocator<NumInlineHashBuckets,typename SecondaryAllocator::HashAllocator>                HashAllocator;
};

/** An inline set allocator that doesn't have any secondary storage. */
template<
	uint32 NumInlineElements,
	uint32 AverageNumberOfElementsPerHashBucket = DEFAULT_NUMBER_OF_ELEMENTS_PER_HASH_BUCKET,
	uint32 MinNumberOfHashedElements            = DEFAULT_MIN_NUMBER_OF_HASHED_ELEMENTS
	>
class TFixedSetAllocator
{
private:

	enum { NumInlineHashBuckets = (NumInlineElements + AverageNumberOfElementsPerHashBucket - 1) / AverageNumberOfElementsPerHashBucket };

	static_assert(NumInlineHashBuckets > 0 && !(NumInlineHashBuckets & (NumInlineHashBuckets - 1)), "Number of inline buckets must be a power of two");

public:

	/** Computes the number of hash buckets to use for a given number of elements. */
	static FORCEINLINE uint32 GetNumberOfHashBuckets(uint32 NumHashedElements)
	{
		const uint32 NumDesiredHashBuckets = FPlatformMath::RoundUpToPowerOfTwo(NumHashedElements / AverageNumberOfElementsPerHashBucket);
		if (NumDesiredHashBuckets < NumInlineHashBuckets)
		{
			return NumInlineHashBuckets;
		}

		if (NumHashedElements < MinNumberOfHashedElements)
		{
			return NumInlineHashBuckets;
		}

		return NumDesiredHashBuckets;
	}

	typedef TFixedSparseArrayAllocator<NumInlineElements> SparseArrayAllocator;
	typedef TFixedAllocator<NumInlineHashBuckets>         HashAllocator;
};


/**
 * 'typedefs' for various allocator defaults.
 *
 * These should be replaced with actual typedefs when Core.h include order is sorted out, as then we won't need to
 * 'forward' these TAllocatorTraits specializations below.
 */

template <int IndexSize> class TSizedDefaultAllocator : public TSizedHeapAllocator<IndexSize> { public: typedef TSizedHeapAllocator<IndexSize> Typedef; };

class FDefaultSetAllocator         : public TSetAllocator<>         { public: typedef TSetAllocator<>         Typedef; };
class FDefaultBitArrayAllocator    : public TInlineAllocator<4>     { public: typedef TInlineAllocator<4>     Typedef; };
class FDefaultSparseArrayAllocator : public TSparseArrayAllocator<> { public: typedef TSparseArrayAllocator<> Typedef; };

template <int IndexSize> struct TAllocatorTraits<TSizedDefaultAllocator<IndexSize>> : TAllocatorTraits<typename TSizedDefaultAllocator<IndexSize>::Typedef> {};

template <> struct TAllocatorTraits<FDefaultAllocator>            : TAllocatorTraits<typename FDefaultAllocator           ::Typedef> {};
template <> struct TAllocatorTraits<FDefaultSetAllocator>         : TAllocatorTraits<typename FDefaultSetAllocator        ::Typedef> {};
template <> struct TAllocatorTraits<FDefaultBitArrayAllocator>    : TAllocatorTraits<typename FDefaultBitArrayAllocator   ::Typedef> {};
template <> struct TAllocatorTraits<FDefaultSparseArrayAllocator> : TAllocatorTraits<typename FDefaultSparseArrayAllocator::Typedef> {};

template <typename InElementAllocator, typename InBitArrayAllocator>
struct TAllocatorTraits<TSparseArrayAllocator<InElementAllocator, InBitArrayAllocator>> : TAllocatorTraitsBase<TSparseArrayAllocator<InElementAllocator, InBitArrayAllocator>>
{
	enum
	{
		SupportsFreezeMemoryImage = TAllocatorTraits<InElementAllocator>::SupportsFreezeMemoryImage && TAllocatorTraits<InBitArrayAllocator>::SupportsFreezeMemoryImage,
	};
};

template <uint8 FromIndexSize, uint8 ToIndexSize> struct TCanMoveBetweenAllocators<TSizedDefaultAllocator<FromIndexSize>, TSizedDefaultAllocator<ToIndexSize>> : TCanMoveBetweenAllocators<typename TSizedDefaultAllocator<FromIndexSize>::Typedef, typename TSizedDefaultAllocator<ToIndexSize>::Typedef> {};
