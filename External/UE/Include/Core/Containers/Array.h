// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Templates/AreTypesEqual.h"
#include "Templates/IsSigned.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Containers/ContainerElementTypeCompatibility.h"
#include "Serialization/Archive.h"
#include "Serialization/MemoryImageWriter.h"

#include "Algo/Heapify.h"
#include "Algo/HeapSort.h"
#include "Algo/IsHeap.h"
#include "Algo/Impl/BinaryHeap.h"
#include "Templates/AndOrNot.h"
#include "Templates/IdentityFunctor.h"
#include "Templates/Invoke.h"
#include "Templates/Less.h"
#include "Templates/ChooseClass.h"
#include "Templates/Sorting.h"
#include "Templates/AlignmentTemplates.h"
#include "Templates/IsConstructible.h"

#include <type_traits>

#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	#define TARRAY_RANGED_FOR_CHECKS 0
#else
	#define TARRAY_RANGED_FOR_CHECKS 1
#endif

template <typename T> struct TCanBulkSerialize { enum { Value = false }; };
template<> struct TCanBulkSerialize<unsigned int> { enum { Value = true }; };
template<> struct TCanBulkSerialize<unsigned short> { enum { Value = true }; };
template<> struct TCanBulkSerialize<int> { enum { Value = true }; };

// Forward declarations

template <typename T, typename AllocatorType> inline void* operator new(size_t Size, TArray<T, AllocatorType>& Array);
template <typename T, typename AllocatorType> inline void* operator new(size_t Size, TArray<T, AllocatorType>& Array, typename TArray<T, AllocatorType>::SizeType Index);

/**
 * Generic iterator which can operate on types that expose the following:
 * - A type called ElementType representing the contained type.
 * - A method SizeType Num() const that returns the number of items in the container.
 * - A method bool IsValidIndex(SizeType index) which returns whether a given index is valid in the container.
 * - A method T& operator\[\](SizeType index) which returns a reference to a contained object by index.
 * - A method void RemoveAt(SizeType index) which removes the element at index
 */
template< typename ContainerType, typename ElementType, typename SizeType>
class TIndexedContainerIterator
{
public:
	TIndexedContainerIterator(ContainerType& InContainer, SizeType StartIndex = 0)
		: Container(InContainer)
		, Index    (StartIndex)
	{
	}

	/** Advances iterator to the next element in the container. */
	TIndexedContainerIterator& operator++()
	{
		++Index;
		return *this;
	}
	TIndexedContainerIterator operator++(int)
	{
		TIndexedContainerIterator Tmp(*this);
		++Index;
		return Tmp;
	}

	/** Moves iterator to the previous element in the container. */
	TIndexedContainerIterator& operator--()
	{
		--Index;
		return *this;
	}
	TIndexedContainerIterator operator--(int)
	{
		TIndexedContainerIterator Tmp(*this);
		--Index;
		return Tmp;
	}

	/** iterator arithmetic support */
	TIndexedContainerIterator& operator+=(SizeType Offset)
	{
		Index += Offset;
		return *this;
	}

	TIndexedContainerIterator operator+(SizeType Offset) const
	{
		TIndexedContainerIterator Tmp(*this);
		return Tmp += Offset;
	}

	TIndexedContainerIterator& operator-=(SizeType Offset)
	{
		return *this += -Offset;
	}

	TIndexedContainerIterator operator-(SizeType Offset) const
	{
		TIndexedContainerIterator Tmp(*this);
		return Tmp -= Offset;
	}

	ElementType& operator* () const
	{
		return Container[ Index ];
	}

	ElementType* operator->() const
	{
		return &Container[ Index ];
	}

	/** conversion to "bool" returning true if the iterator has not reached the last element. */
	FORCEINLINE explicit operator bool() const
	{
		return Container.IsValidIndex(Index);
	}

	/** Returns an index to the current element. */
	SizeType GetIndex() const
	{
		return Index;
	}

	/** Resets the iterator to the first element. */
	void Reset()
	{
		Index = 0;
	}

	/** Sets the iterator to one past the last element. */
	void SetToEnd()
	{
		Index = Container.Num();
	}
	
	/** Removes current element in array. This invalidates the current iterator value and it must be incremented */
	void RemoveCurrent()
	{
		Container.RemoveAt(Index);
		Index--;
	}

	FORCEINLINE friend bool operator==(const TIndexedContainerIterator& Lhs, const TIndexedContainerIterator& Rhs) { return &Lhs.Container == &Rhs.Container && Lhs.Index == Rhs.Index; }
	FORCEINLINE friend bool operator!=(const TIndexedContainerIterator& Lhs, const TIndexedContainerIterator& Rhs) { return &Lhs.Container != &Rhs.Container || Lhs.Index != Rhs.Index; }

private:

	ContainerType& Container;
	SizeType      Index;
};


/** operator + */
template <typename ContainerType, typename ElementType, typename SizeType>
FORCEINLINE TIndexedContainerIterator<ContainerType, ElementType, SizeType> operator+(SizeType Offset, TIndexedContainerIterator<ContainerType, ElementType, SizeType> RHS)
{
	return RHS + Offset;
}


#if TARRAY_RANGED_FOR_CHECKS
	/**
	 * Pointer-like iterator type for ranged-for loops which checks that the
	 * container hasn't been resized during iteration.
	 */
	template <typename ElementType, typename SizeType>
	struct TCheckedPointerIterator
	{
		// This iterator type only supports the minimal functionality needed to support
		// C++ ranged-for syntax.  For example, it does not provide post-increment ++ nor ==.
		//
		// We do add an operator-- to help FString implementation

		explicit TCheckedPointerIterator(const SizeType& InNum, ElementType* InPtr)
			: Ptr       (InPtr)
			, CurrentNum(InNum)
			, InitialNum(InNum)
		{
		}

		FORCEINLINE ElementType& operator*() const
		{
			return *Ptr;
		}

		FORCEINLINE TCheckedPointerIterator& operator++()
		{
			++Ptr;
			return *this;
		}

		FORCEINLINE TCheckedPointerIterator& operator--()
		{
			--Ptr;
			return *this;
		}

	private:
		ElementType*    Ptr;
		const SizeType& CurrentNum;
		SizeType        InitialNum;

		FORCEINLINE friend bool operator!=(const TCheckedPointerIterator& Lhs, const TCheckedPointerIterator& Rhs)
		{
			// We only need to do the check in this operator, because no other operator will be
			// called until after this one returns.
			//
			// Also, we should only need to check one side of this comparison - if the other iterator isn't
			// even from the same array then the compiler has generated bad code.
			ensureMsgf(Lhs.CurrentNum == Lhs.InitialNum, TEXT("Array has changed during ranged-for iteration!"));
			return Lhs.Ptr != Rhs.Ptr;
		}
	};
#endif


template <typename ElementType, typename IteratorType>
struct TDereferencingIterator
{
	explicit TDereferencingIterator(IteratorType InIter)
		: Iter(InIter)
	{
	}

	FORCEINLINE ElementType& operator*() const
	{
		return *(ElementType*)*Iter;
	}

	FORCEINLINE TDereferencingIterator& operator++()
	{
		++Iter;
		return *this;
	}

private:
	IteratorType Iter;

	FORCEINLINE friend bool operator!=(const TDereferencingIterator& Lhs, const TDereferencingIterator& Rhs)
	{
		return Lhs.Iter != Rhs.Iter;
	}
};

namespace UE4Array_Private
{
	template <typename FromArrayType, typename ToArrayType>
	struct TCanMoveTArrayPointersBetweenArrayTypes
	{
		typedef typename FromArrayType::AllocatorType FromAllocatorType;
		typedef typename ToArrayType  ::AllocatorType ToAllocatorType;
		typedef typename FromArrayType::ElementType   FromElementType;
		typedef typename ToArrayType  ::ElementType   ToElementType;

		enum
		{
			Value =
				TOr<TAreTypesEqual<FromAllocatorType, ToAllocatorType>, TCanMoveBetweenAllocators<FromAllocatorType, ToAllocatorType>>::Value && // Allocators must be equal or move-compatible
				TContainerTraits<FromArrayType>::MoveWillEmptyContainer &&   // A move must be allowed to leave the source array empty
				(
					TAreTypesEqual         <ToElementType, FromElementType>::Value || // The element type of the container must be the same, or...
					TIsBitwiseConstructible<ToElementType, FromElementType>::Value    // ... the element type of the source container must be bitwise constructible from the element type in the destination container
				)
		};
	};

	// Assume elements are compatible with themselves - avoids problems with generated copy
	// constuctors of arrays of forwarded types, e.g.:
	//
	// struct FThing;
	//
	// struct FOuter
	// {
	//     TArray<FThing> Arr; // this will cause errors without this workaround
	// };
	//
	// This should be changed to use std::disjunction and std::is_constructible, and the usage
	// changed to use ::value instead of ::Value, when std::disjunction (C++17) is available everywhere.
	template <typename DestType, typename SourceType>
	using TArrayElementsAreCompatible = TOrValue<std::is_same<DestType, std::decay_t<DestType>>::value, TIsConstructible<DestType, SourceType>>;
}


/**
 * Templated dynamic array
 *
 * A dynamically sized array of typed elements.  Makes the assumption that your elements are relocate-able; 
 * i.e. that they can be transparently moved to new memory without a copy constructor.  The main implication 
 * is that pointers to elements in the TArray may be invalidated by adding or removing other elements to the array. 
 * Removal of elements is O(N) and invalidates the indices of subsequent elements.
 *
 * Caution: as noted below some methods are not safe for element types that require constructors.
 *
 **/
template<typename InElementType, typename InAllocatorType>
class TArray
{
	template <typename OtherInElementType, typename OtherAllocator>
	friend class TArray;

public:
	typedef typename InAllocatorType::SizeType SizeType;
	typedef InElementType ElementType;
	typedef InAllocatorType AllocatorType;

	UE_DEPRECATED(5.0, "TArray::Allocator type is deprecated, please use TArray::AllocatorType instead.")
	typedef InAllocatorType Allocator;

	typedef typename TChooseClass<
		AllocatorType::NeedsElementType,
		typename AllocatorType::template ForElementType<ElementType>,
		typename AllocatorType::ForAnyElementType
	>::Result ElementAllocatorType;

	static_assert(TIsSigned<SizeType>::Value, "TArray only supports signed index types");

	/**
	 * Constructor, initializes element number counters.
	 */
	FORCEINLINE TArray()
		: ArrayNum(0)
		, ArrayMax(AllocatorInstance.GetInitialCapacity())
	{}

	/**
	 * Constructor from a raw array of elements.
	 *
	 * @param Ptr   A pointer to an array of elements to copy.
	 * @param Count The number of elements to copy from Ptr.
	 * @see Append
	 */
	FORCEINLINE TArray(const ElementType* Ptr, SizeType Count)
	{
		check(Ptr != nullptr || Count == 0);

		CopyToEmpty(Ptr, Count, 0, 0);
	}

	template <typename OtherElementType, typename OtherSizeType>
	explicit TArray(const TArrayView<OtherElementType, OtherSizeType>& Other);

	/**
	 * Initializer list constructor
	 */
	TArray(std::initializer_list<InElementType> InitList)
	{
		// This is not strictly legal, as std::initializer_list's iterators are not guaranteed to be pointers, but
		// this appears to be the case on all of our implementations.  Also, if it's not true on a new implementation,
		// it will fail to compile rather than behave badly.
		CopyToEmpty(InitList.begin(), (SizeType)InitList.size(), 0, 0);
	}

	/**
	 * Copy constructor with changed allocator. Use the common routine to perform the copy.
	 *
	 * @param Other The source array to copy.
	 */
	template <
		typename OtherElementType,
		typename OtherAllocator,
		std::enable_if_t<UE4Array_Private::TArrayElementsAreCompatible<ElementType, const OtherElementType&>::Value>* = nullptr
	>
	FORCEINLINE explicit TArray(const TArray<OtherElementType, OtherAllocator>& Other)
	{
		CopyToEmpty(Other.GetData(), Other.Num(), 0, 0);
	}

	/**
	 * Copy constructor. Use the common routine to perform the copy.
	 *
	 * @param Other The source array to copy.
	 */
	FORCEINLINE TArray(const TArray& Other)
	{
		CopyToEmpty(Other.GetData(), Other.Num(), 0, 0);
	}

	/**
	 * Copy constructor. Use the common routine to perform the copy.
	 *
	 * @param Other The source array to copy.
	 * @param ExtraSlack Tells how much extra memory should be preallocated
	 *                   at the end of the array in the number of elements.
	 */
	FORCEINLINE TArray(const TArray& Other, SizeType ExtraSlack)
	{
		CopyToEmpty(Other.GetData(), Other.Num(), 0, ExtraSlack);
	}

	/**
	 * Initializer list assignment operator. First deletes all currently contained elements
	 * and then copies from initializer list.
	 *
	 * @param InitList The initializer_list to copy from.
	 */
	TArray& operator=(std::initializer_list<InElementType> InitList)
	{
		DestructItems(GetData(), ArrayNum);
		// This is not strictly legal, as std::initializer_list's iterators are not guaranteed to be pointers, but
		// this appears to be the case on all of our implementations.  Also, if it's not true on a new implementation,
		// it will fail to compile rather than behave badly.
		CopyToEmpty(InitList.begin(), (SizeType)InitList.size(), ArrayMax, 0);
		return *this;
	}

	/**
	 * Assignment operator. First deletes all currently contained elements
	 * and then copies from other array.
	 *
	 * AllocatorType changing version.
	 *
	 * @param Other The source array to assign from.
	 */
	template<typename OtherAllocatorType>
	TArray& operator=(const TArray<ElementType, OtherAllocatorType>& Other)
	{
		DestructItems(GetData(), ArrayNum);
		CopyToEmpty(Other.GetData(), Other.Num(), ArrayMax, 0);
		return *this;
	}

	/**
	 * Assignment operator. First deletes all currently contained elements
	 * and then copies from other array.
	 *
	 * @param Other The source array to assign from.
	 */
	TArray& operator=(const TArray& Other)
	{
		if (this != &Other)
		{
			DestructItems(GetData(), ArrayNum);
			CopyToEmpty(Other.GetData(), Other.Num(), ArrayMax, 0);
		}
		return *this;
	}

	template <typename OtherElementType, typename OtherSizeType>
	TArray& operator=(const TArrayView<OtherElementType, OtherSizeType>& Other);

private:
	/**
	 * Moves or copies array. Depends on the array type traits.
	 *
	 * This override moves.
	 *
	 * @param ToArray Array to move into.
	 * @param FromArray Array to move from.
	 */
	template <typename FromArrayType, typename ToArrayType>
	static FORCEINLINE std::enable_if_t<UE4Array_Private::TCanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>::Value> MoveOrCopy(ToArrayType& ToArray, FromArrayType& FromArray, SizeType PrevMax)
	{
		using FromAllocatorType = typename FromArrayType::AllocatorType;
		using ToAllocatorType   = typename ToArrayType::AllocatorType;

		if constexpr (TCanMoveBetweenAllocators<FromAllocatorType, ToAllocatorType>::Value)
		{
			ToArray.AllocatorInstance.template MoveToEmptyFromOtherAllocator<FromAllocatorType>(FromArray.AllocatorInstance);
		}
		else
		{
			ToArray.AllocatorInstance.MoveToEmpty(FromArray.AllocatorInstance);
		}

		ToArray  .ArrayNum = FromArray.ArrayNum;
		ToArray  .ArrayMax = FromArray.ArrayMax;

		// Ensure the destination container could hold the source range (when the allocator size types shrink)
		checkf(ToArray.ArrayNum == FromArray.ArrayNum && ToArray.ArrayMax == FromArray.ArrayMax, TEXT("Data lost when moving to a container with a more constrained size type"));

		FromArray.ArrayNum = 0;
		FromArray.ArrayMax = FromArray.AllocatorInstance.GetInitialCapacity();
	}

	/**
	 * Moves or copies array. Depends on the array type traits.
	 *
	 * This override copies.
	 *
	 * @param ToArray Array to move into.
	 * @param FromArray Array to move from.
	 * @param ExtraSlack Tells how much extra memory should be preallocated
	 *                   at the end of the array in the number of elements.
	 */
	template <typename FromArrayType, typename ToArrayType>
	static FORCEINLINE std::enable_if_t<!UE4Array_Private::TCanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>::Value> MoveOrCopy(ToArrayType& ToArray, FromArrayType& FromArray, SizeType PrevMax)
	{
		ToArray.CopyToEmpty(FromArray.GetData(), FromArray.Num(), PrevMax, 0);
	}

	/**
	 * Moves or copies array. Depends on the array type traits.
	 *
	 * This override moves.
	 *
	 * @param ToArray Array to move into.
	 * @param FromArray Array to move from.
	 * @param ExtraSlack Tells how much extra memory should be preallocated
	 *                   at the end of the array in the number of elements.
	 */
	template <typename FromArrayType, typename ToArrayType>
	static FORCEINLINE std::enable_if_t<UE4Array_Private::TCanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>::Value> MoveOrCopyWithSlack(ToArrayType& ToArray, FromArrayType& FromArray, SizeType PrevMax, SizeType ExtraSlack)
	{
		MoveOrCopy(ToArray, FromArray, PrevMax);

		ToArray.Reserve(ToArray.ArrayNum + ExtraSlack);
	}

	/**
	 * Moves or copies array. Depends on the array type traits.
	 *
	 * This override copies.
	 *
	 * @param ToArray Array to move into.
	 * @param FromArray Array to move from.
	 * @param ExtraSlack Tells how much extra memory should be preallocated
	 *                   at the end of the array in the number of elements.
	 */
	template <typename FromArrayType, typename ToArrayType>
	static FORCEINLINE std::enable_if_t<!UE4Array_Private::TCanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>::Value> MoveOrCopyWithSlack(ToArrayType& ToArray, FromArrayType& FromArray, SizeType PrevMax, SizeType ExtraSlack)
	{
		ToArray.CopyToEmpty(FromArray.GetData(), FromArray.Num(), PrevMax, ExtraSlack);
	}

public:
	/**
	 * Move constructor.
	 *
	 * @param Other Array to move from.
	 */
	FORCEINLINE TArray(TArray&& Other)
	{
		MoveOrCopy(*this, Other, 0);
	}

	/**
	 * Move constructor.
	 *
	 * @param Other Array to move from.
	 */
	template <
		typename OtherElementType,
		typename OtherAllocator,
		std::enable_if_t<UE4Array_Private::TArrayElementsAreCompatible<ElementType, OtherElementType&&>::Value>* = nullptr
	>
	FORCEINLINE explicit TArray(TArray<OtherElementType, OtherAllocator>&& Other)
	{
		MoveOrCopy(*this, Other, 0);
	}

	/**
	 * Move constructor.
	 *
	 * @param Other Array to move from.
	 * @param ExtraSlack Tells how much extra memory should be preallocated
	 *                   at the end of the array in the number of elements.
	 */
	template <
		typename OtherElementType,
		std::enable_if_t<UE4Array_Private::TArrayElementsAreCompatible<ElementType, OtherElementType&&>::Value>* = nullptr
	>
	TArray(TArray<OtherElementType, AllocatorType>&& Other, SizeType ExtraSlack)
	{
		// We don't implement move semantics for general OtherAllocators, as there's no way
		// to tell if they're compatible with the current one.  Probably going to be a pretty
		// rare requirement anyway.

		MoveOrCopyWithSlack(*this, Other, 0, ExtraSlack);
	}

	/**
	 * Move assignment operator.
	 *
	 * @param Other Array to assign and move from.
	 */
	TArray& operator=(TArray&& Other)
	{
		if (this != &Other)
		{
			DestructItems(GetData(), ArrayNum);
			MoveOrCopy(*this, Other, ArrayMax);
		}
		return *this;
	}

	/** Destructor. */
	~TArray()
	{
		#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		#if defined(_MSC_VER) && !defined(__clang__)	// Relies on MSVC-specific lazy template instantiation to support arrays of incomplete types
			// ensure that DebugGet gets instantiated.
			// this is done to ensure DebugGet is available for the debugger watch window
			//@todo it would be nice if we had a cleaner solution for DebugGet
			volatile const ElementType* Dummy = &DebugGet(0);
		#endif
		#endif
		
		DestructItems(GetData(), ArrayNum);

		// note ArrayNum, ArrayMax and data pointer are not invalidated
		// they are left unchanged and use-after-destruct will see them the same as before destruct
	}

	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @returns Pointer to first array entry or nullptr if ArrayMax == 0.
	 */
	FORCEINLINE ElementType* GetData()
	{
		return (ElementType*)AllocatorInstance.GetAllocation();
	}

	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @returns Pointer to first array entry or nullptr if ArrayMax == 0.
	 */
	FORCEINLINE const ElementType* GetData() const
	{
		return (const ElementType*)AllocatorInstance.GetAllocation();
	}

	/** 
	 * Helper function returning the size of the inner type.
	 *
	 * @returns Size in bytes of array type.
	 */
	FORCEINLINE uint32 GetTypeSize() const
	{
		return sizeof(ElementType);
	}

	/** 
	 * Helper function to return the amount of memory allocated by this
	 * container.
	 * Only returns the size of allocations made directly by the container, not the elements themselves.
	 *
	 * @returns Number of bytes allocated by this container.
	 */
	FORCEINLINE SIZE_T GetAllocatedSize(void) const
	{
		return AllocatorInstance.GetAllocatedSize(ArrayMax, sizeof(ElementType));
	}

	/**
	 * Returns the amount of slack in this array in elements.
	 *
	 * @see Num, Shrink
	 */
	FORCEINLINE SizeType GetSlack() const
	{
		return ArrayMax - ArrayNum;
	}

	/**
	 * Checks array invariants: if array size is greater than zero and less
	 * than maximum.
	 */
	FORCEINLINE void CheckInvariants() const
	{
		checkSlow((ArrayNum >= 0) & (ArrayMax >= ArrayNum)); // & for one branch
	}

	/**
	 * Checks if index is in array range.
	 *
	 * @param Index Index to check.
	 */
	FORCEINLINE void RangeCheck(SizeType Index) const
	{
		CheckInvariants();

		// Template property, branch will be optimized out
		if (AllocatorType::RequireRangeCheck)
		{
			checkf((Index >= 0) & (Index < ArrayNum),TEXT("Array index out of bounds: %i from an array of size %i"),Index,ArrayNum); // & for one branch
		}
	}

	/**
	 * Tests if index is valid, i.e. greater than or equal to zero, and less than the number of elements in the array.
	 *
	 * @param Index Index to test.
	 * @returns True if index is valid. False otherwise.
	 */
	FORCEINLINE bool IsValidIndex(SizeType Index) const
	{
		return Index >= 0 && Index < ArrayNum;
	}

	/**
	 * Returns true if the array is empty and contains no elements. 
	 *
	 * @returns True if the array is empty.
	 * @see Num
	 */
	bool IsEmpty() const
	{
		return ArrayNum == 0;
	}

	/**
	 * Returns number of elements in array.
	 *
	 * @returns Number of elements in array.
	 * @see GetSlack
	 */
	FORCEINLINE SizeType Num() const
	{
		return ArrayNum;
	}

	/**
	 * Returns maximum number of elements in array.
	 *
	 * @returns Maximum number of elements in array.
	 * @see GetSlack
	 */
	FORCEINLINE SizeType Max() const
	{
		return ArrayMax;
	}

	/**
	 * Array bracket operator. Returns reference to element at give index.
	 *
	 * @returns Reference to indexed element.
	 */
	FORCEINLINE ElementType& operator[](SizeType Index)
	{
		RangeCheck(Index);
		return GetData()[Index];
	}

	/**
	 * Array bracket operator. Returns reference to element at give index.
	 *
	 * Const version of the above.
	 *
	 * @returns Reference to indexed element.
	 */
	FORCEINLINE const ElementType& operator[](SizeType Index) const
	{
		RangeCheck(Index);
		return GetData()[Index];
	}

	/**
	 * Pops element from the array.
	 *
	 * @param bAllowShrinking If this call allows shrinking of the array during element remove.
	 * @returns Popped element.
	 */
	template<typename ET=InElementType>
	FORCEINLINE typename TEnableIf<!TIsAbstract<ET>::Value, ElementType>::Type Pop(bool bAllowShrinking = true)
	{
		RangeCheck(0);
		ElementType Result = MoveTempIfPossible(GetData()[ArrayNum - 1]);
		RemoveAt(ArrayNum - 1, 1, bAllowShrinking);
		return Result;
	}

	/**
	 * Pushes element into the array.
	 *
	 * @param Item Item to push.
	 */
	FORCEINLINE void Push(ElementType&& Item)
	{
		Add(MoveTempIfPossible(Item));
	}

	/**
	 * Pushes element into the array.
	 *
	 * Const ref version of the above.
	 *
	 * @param Item Item to push.
	 * @see Pop, Top
	 */
	FORCEINLINE void Push(const ElementType& Item)
	{
		Add(Item);
	}

	/**
	 * Returns the top element, i.e. the last one.
	 *
	 * @returns Reference to the top element.
	 * @see Pop, Push
	 */
	FORCEINLINE ElementType& Top()
	{
		return Last();
	}

	/**
	 * Returns the top element, i.e. the last one.
	 *
	 * Const version of the above.
	 *
	 * @returns Reference to the top element.
	 * @see Pop, Push
	 */
	FORCEINLINE const ElementType& Top() const
	{
		return Last();
	}

	/**
	 * Returns n-th last element from the array.
	 *
	 * @param IndexFromTheEnd (Optional) Index from the end of array (default = 0).
	 * @returns Reference to n-th last element from the array.
	 */
	FORCEINLINE ElementType& Last(SizeType IndexFromTheEnd = 0)
	{
		RangeCheck(ArrayNum - IndexFromTheEnd - 1);
		return GetData()[ArrayNum - IndexFromTheEnd - 1];
	}

	/**
	 * Returns n-th last element from the array.
	 *
	 * Const version of the above.
	 *
	 * @param IndexFromTheEnd (Optional) Index from the end of array (default = 0).
	 * @returns Reference to n-th last element from the array.
	 */
	FORCEINLINE const ElementType& Last(SizeType IndexFromTheEnd = 0) const
	{
		RangeCheck(ArrayNum - IndexFromTheEnd - 1);
		return GetData()[ArrayNum - IndexFromTheEnd - 1];
	}

	/**
	 * Shrinks the array's used memory to smallest possible to store elements currently in it.
	 *
	 * @see Slack
	 */
	FORCEINLINE void Shrink()
	{
		CheckInvariants();
		if (ArrayMax != ArrayNum)
		{
			ResizeTo(ArrayNum);
		}
	}

	/**
	 * Finds element within the array.
	 *
	 * @param Item Item to look for.
	 * @param Index Will contain the found index.
	 * @returns True if found. False otherwise.
	 * @see FindLast, FindLastByPredicate
	 */
	FORCEINLINE bool Find(const ElementType& Item, SizeType& Index) const
	{
		Index = this->Find(Item);
		return Index != INDEX_NONE;
	}

	/**
	 * Finds element within the array.
	 *
	 * @param Item Item to look for.
	 * @returns Index of the found element. INDEX_NONE otherwise.
	 * @see FindLast, FindLastByPredicate
	 */
	SizeType Find(const ElementType& Item) const
	{
		const ElementType* RESTRICT Start = GetData();
		for (const ElementType* RESTRICT Data = Start, *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (*Data == Item)
			{
				return static_cast<SizeType>(Data - Start);
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Finds element within the array starting from the end.
	 *
	 * @param Item Item to look for.
	 * @param Index Output parameter. Found index.
	 * @returns True if found. False otherwise.
	 * @see Find, FindLastByPredicate
	 */
	FORCEINLINE bool FindLast(const ElementType& Item, SizeType& Index) const
	{
		Index = this->FindLast(Item);
		return Index != INDEX_NONE;
	}

	/**
	 * Finds element within the array starting from the end.
	 *
	 * @param Item Item to look for.
	 * @returns Index of the found element. INDEX_NONE otherwise.
	 */
	SizeType FindLast(const ElementType& Item) const
	{
		for (const ElementType* RESTRICT Start = GetData(), *RESTRICT Data = Start + ArrayNum; Data != Start; )
		{
			--Data;
			if (*Data == Item)
			{
				return static_cast<SizeType>(Data - Start);
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Searches an initial subrange of the array for the last occurrence of an element which matches the specified predicate.
	 *
	 * @param Pred Predicate taking array element and returns true if element matches search criteria, false otherwise.
	 * @param Count The number of elements from the front of the array through which to search.
	 * @returns Index of the found element. INDEX_NONE otherwise.
	 */
	template <typename Predicate>
	SizeType FindLastByPredicate(Predicate Pred, SizeType Count) const
	{
		check(Count >= 0 && Count <= this->Num());
		for (const ElementType* RESTRICT Start = GetData(), *RESTRICT Data = Start + Count; Data != Start; )
		{
			--Data;
			if (::Invoke(Pred, *Data))
			{
				return static_cast<SizeType>(Data - Start);
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Searches the array for the last occurrence of an element which matches the specified predicate.
	 *
	 * @param Pred Predicate taking array element and returns true if element matches search criteria, false otherwise.
	 * @returns Index of the found element. INDEX_NONE otherwise.
	 */
	template <typename Predicate>
	FORCEINLINE SizeType FindLastByPredicate(Predicate Pred) const
	{
		return FindLastByPredicate(Pred, ArrayNum);
	}

	/**
	 * Finds an item by key (assuming the ElementType overloads operator== for
	 * the comparison).
	 *
	 * @param Key The key to search by.
	 * @returns Index to the first matching element, or INDEX_NONE if none is found.
	 */
	template <typename KeyType>
	SizeType IndexOfByKey(const KeyType& Key) const
	{
		const ElementType* RESTRICT Start = GetData();
		for (const ElementType* RESTRICT Data = Start, *RESTRICT DataEnd = Start + ArrayNum; Data != DataEnd; ++Data)
		{
			if (*Data == Key)
			{
				return static_cast<SizeType>(Data - Start);
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Finds an item by predicate.
	 *
	 * @param Pred The predicate to match.
	 * @returns Index to the first matching element, or INDEX_NONE if none is found.
	 */
	template <typename Predicate>
	SizeType IndexOfByPredicate(Predicate Pred) const
	{
		const ElementType* RESTRICT Start = GetData();
		for (const ElementType* RESTRICT Data = Start, *RESTRICT DataEnd = Start + ArrayNum; Data != DataEnd; ++Data)
		{
			if (::Invoke(Pred, *Data))
			{
				return static_cast<SizeType>(Data - Start);
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Finds an item by key (assuming the ElementType overloads operator== for
	 * the comparison).
	 *
	 * @param Key The key to search by.
	 * @returns Pointer to the first matching element, or nullptr if none is found.
	 * @see Find
	 */
	template <typename KeyType>
	FORCEINLINE const ElementType* FindByKey(const KeyType& Key) const
	{
		return const_cast<TArray*>(this)->FindByKey(Key);
	}

	/**
	 * Finds an item by key (assuming the ElementType overloads operator== for
	 * the comparison). Time Complexity: O(n), starts iteration from the beginning so better performance if Key is in the front
	 *
	 * @param Key The key to search by.
	 * @returns Pointer to the first matching element, or nullptr if none is found.
	 * @see Find
	 */
	template <typename KeyType>
	ElementType* FindByKey(const KeyType& Key)
	{
		for (ElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (*Data == Key)
			{
				return Data;
			}
		}

		return nullptr;
	}

	/**
	 * Finds an element which matches a predicate functor.
	 *
	 * @param Pred The functor to apply to each element.
	 * @returns Pointer to the first element for which the predicate returns true, or nullptr if none is found.
	 * @see FilterByPredicate, ContainsByPredicate
	 */
	template <typename Predicate>
	FORCEINLINE const ElementType* FindByPredicate(Predicate Pred) const
	{
		return const_cast<TArray*>(this)->FindByPredicate(Pred);
	}

	/**
	 * Finds an element which matches a predicate functor.
	 *
	 * @param Pred The functor to apply to each element. true, or nullptr if none is found.
	 * @see FilterByPredicate, ContainsByPredicate
	 */
	template <typename Predicate>
	ElementType* FindByPredicate(Predicate Pred)
	{
		for (ElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (::Invoke(Pred, *Data))
			{
				return Data;
			}
		}

		return nullptr;
	}

	/**
	 * Filters the elements in the array based on a predicate functor.
	 *
	 * @param Pred The functor to apply to each element.
	 * @returns TArray with the same type as this object which contains
	 *          the subset of elements for which the functor returns true.
	 * @see FindByPredicate, ContainsByPredicate
	 */
	template <typename Predicate>
	TArray<ElementType> FilterByPredicate(Predicate Pred) const
	{
		TArray<ElementType> FilterResults;
		for (const ElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (::Invoke(Pred, *Data))
			{
				FilterResults.Add(*Data);
			}
		}
		return FilterResults;
	}

	/**
	 * Checks if this array contains the element.
	 *
	 * @returns	True if found. False otherwise.
	 * @see ContainsByPredicate, FilterByPredicate, FindByPredicate
	 */
	template <typename ComparisonType>
	bool Contains(const ComparisonType& Item) const
	{
		for (const ElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (*Data == Item)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * Checks if this array contains element for which the predicate is true.
	 *
	 * @param Predicate to use
	 * @returns	True if found. False otherwise.
	 * @see Contains, Find
	 */
	template <typename Predicate>
	FORCEINLINE bool ContainsByPredicate(Predicate Pred) const
	{
		return FindByPredicate(Pred) != nullptr;
	}

	/**
	 * Equality operator.
	 *
	 * @param OtherArray Array to compare.
	 * @returns True if this array is the same as OtherArray. False otherwise.
	 */
	bool operator==(const TArray& OtherArray) const
	{
		SizeType Count = Num();

		return Count == OtherArray.Num() && CompareItems(GetData(), OtherArray.GetData(), Count);
	}

	/**
	 * Inequality operator.
	 *
	 * @param OtherArray Array to compare.
	 * @returns True if this array is NOT the same as OtherArray. False otherwise.
	 */
	FORCEINLINE bool operator!=(const TArray& OtherArray) const
	{
		return !(*this == OtherArray);
	}

	/**
	 * Serialization operator.
	 *
	 * @param Ar Archive to serialize the array with.
	 * @param A Array to serialize.
	 * @returns Passing the given archive.
	 */
	friend FArchive& operator<<(FArchive& Ar, TArray& A)
	{
		A.CountBytes(Ar);

		// For net archives, limit serialization to 16MB, to protect against excessive allocation
		constexpr SizeType MaxNetArraySerialize = (16 * 1024 * 1024) / sizeof(ElementType);
		SizeType SerializeNum = Ar.IsLoading() ? 0 : A.ArrayNum;

		Ar << SerializeNum;

		if (SerializeNum == 0)
		{
			// if we are loading, then we have to reset the size to 0, in case it isn't currently 0
			if (Ar.IsLoading())
			{
				A.Empty();
			}
			return Ar;
		}

		check(SerializeNum >= 0);

		if (!Ar.IsError() && SerializeNum > 0 && ensure(!Ar.IsNetArchive() || SerializeNum <= MaxNetArraySerialize))
		{
			// if we don't need to perform per-item serialization, just read it in bulk
			if constexpr (sizeof(ElementType) == 1 || TCanBulkSerialize<ElementType>::Value)
			{
				A.ArrayNum = SerializeNum;

				// Serialize simple bytes which require no construction or destruction.
				if ((A.ArrayNum || A.ArrayMax) && Ar.IsLoading())
				{
					A.ResizeForCopy(A.ArrayNum, A.ArrayMax);
				}

				if(TIsUECoreVariant<ElementType, double>::Value && Ar.IsLoading() && Ar.UEVer() < EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES)
				{
					// Per item serialization is required for core variant types loaded from pre LWC archives, to enable conversion from float to double.
					A.Empty(SerializeNum);
					for (SizeType i=0; i<SerializeNum; i++)
					{
						Ar << *::new(A) ElementType;
					}		
				}
				else
				{
					Ar.Serialize(A.GetData(), A.Num() * sizeof(ElementType));
				}
			}
			else if (Ar.IsLoading())
			{
				// Required for resetting ArrayNum
				A.Empty(SerializeNum);

				for (SizeType i=0; i<SerializeNum; i++)
				{
					Ar << *::new(A) ElementType;
				}
			}
			else
			{
				A.ArrayNum = SerializeNum;

				for (SizeType i=0; i<A.ArrayNum; i++)
				{
					Ar << A[i];
				}
			}
		}
		else
		{
			Ar.SetError();
		}

		return Ar;
	}

	/**
	 * Bulk serialize array as a single memory blob when loading. Uses regular serialization code for saving
	 * and doesn't serialize at all otherwise (e.g. transient, garbage collection, ...).
	 * 
	 * Requirements:
	 *   - T's << operator needs to serialize ALL member variables in the SAME order they are layed out in memory.
	 *   - T's << operator can NOT perform any fixup operations. This limitation can be lifted by manually copying
	 *     the code after the BulkSerialize call.
	 *   - T can NOT contain any member variables requiring constructor calls or pointers
	 *   - sizeof(ElementType) must be equal to the sum of sizes of it's member variables.
	 *        - e.g. use pragma pack (push,1)/ (pop) to ensure alignment
	 *        - match up uint8/ WORDs so everything always end up being properly aligned
	 *   - Code can not rely on serialization of T if neither IsLoading() nor IsSaving() is true.
	 *   - Can only be called platforms that either have the same endianness as the one the content was saved with
	 *     or had the endian conversion occur in a cooking process like e.g. for consoles.
	 *
	 * Notes:
	 *   - it is safe to call BulkSerialize on TTransArrays
	 *
	 * IMPORTANT:
	 *   - This is Overridden in XeD3dResourceArray.h Please make certain changes are propogated accordingly
	 *
	 * @param Ar	FArchive to bulk serialize this TArray to/from
	 */
	void BulkSerialize(FArchive& Ar, bool bForcePerElementSerialization = false)
	{
		int32 ElementSize = sizeof(ElementType);
		// Serialize element size to detect mismatch across platforms.
		int32 SerializedElementSize = ElementSize;
		Ar << SerializedElementSize;
	
		if (bForcePerElementSerialization
			|| (Ar.IsSaving()			// if we are saving, we always do the ordinary serialize as a way to make sure it matches up with bulk serialization
			&& !Ar.IsCooking()			// but cooking and transacting is performance critical, so we skip that
			&& !Ar.IsTransacting())		
			|| Ar.IsByteSwapping()		// if we are byteswapping, we need to do that per-element
			)
		{
			Ar << *this;
		}
		else
		{
			CountBytes(Ar);
			if (Ar.IsLoading())
			{
				// Basic sanity checking to ensure that sizes match.
				checkf(SerializedElementSize == 0 || SerializedElementSize == ElementSize, TEXT("Unexpected array element size. Expected %i, Got: %i. Package can be corrupt or the array template type changed."), ElementSize, SerializedElementSize);
				// Serialize the number of elements, block allocate the right amount of memory and deserialize
				// the data as a giant memory blob in a single call to Serialize. Please see the function header
				// for detailed documentation on limitations and implications.
				SizeType NewArrayNum = 0;
				Ar << NewArrayNum;
				Empty(NewArrayNum);
				AddUninitialized(NewArrayNum);
				Ar.Serialize(GetData(), NewArrayNum * SerializedElementSize);
			}
			else if (Ar.IsSaving())
			{
				SizeType ArrayCount = Num();
				Ar << ArrayCount;
				Ar.Serialize(GetData(), ArrayCount * SerializedElementSize);
			}
		}
	}

	/**
	 * Count bytes needed to serialize this array.
	 *
	 * @param Ar Archive to count for.
	 */
	void CountBytes(FArchive& Ar) const
	{
		Ar.CountBytes(ArrayNum*sizeof(ElementType), ArrayMax*sizeof(ElementType));
	}

	/**
	 * Adds a given number of uninitialized elements into the array.
	 *
	 * Caution, AddUninitialized() will create elements without calling
	 * the constructor and this is not appropriate for element types that
	 * require a constructor to function properly.
	 *
	 * @param Count Number of elements to add.
	 * @returns Number of elements in array before addition.
	 */
	FORCEINLINE SizeType AddUninitialized(SizeType Count = 1)
	{
		CheckInvariants();
		checkSlow(Count >= 0);

		const SizeType OldNum = ArrayNum;
		if ((ArrayNum += Count) > ArrayMax)
		{
			ResizeGrow(OldNum);
		}
		return OldNum;
	}

private:
	template <typename OtherSizeType>
	void InsertUninitializedImpl(SizeType Index, OtherSizeType Count)
	{
		CheckInvariants();
		checkSlow((Count >= 0) & (Index >= 0) & (Index <= ArrayNum));

		SizeType NewNum = Count;
		checkf((OtherSizeType)NewNum == Count, TEXT("Invalid number of elements to add to this array type: %llu"), (unsigned long long)NewNum);

		const SizeType OldNum = ArrayNum;
		if ((ArrayNum += Count) > ArrayMax)
		{
			ResizeGrow(OldNum);
		}
		ElementType* Data = GetData() + Index;
		RelocateConstructItems<ElementType>(Data + Count, Data, OldNum - Index);
	}

public:
	/**
	 * Inserts a given number of uninitialized elements into the array at given
	 * location.
	 *
	 * Caution, InsertUninitialized() will create elements without calling the
	 * constructor and this is not appropriate for element types that require
	 * a constructor to function properly.
	 *
	 * @param Index Tells where to insert the new elements.
	 * @param Count Number of elements to add.
	 * @see Insert, InsertZeroed, InsertDefaulted
	 */
	FORCEINLINE void InsertUninitialized(SizeType Index, SizeType Count = 1)
	{
		InsertUninitializedImpl(Index, Count);
	}

	/**
	 * Inserts a given number of zeroed elements into the array at given
	 * location.
	 *
	 * Caution, InsertZeroed() will create elements without calling the
	 * constructor and this is not appropriate for element types that require
	 * a constructor to function properly.
	 *
	 * @param Index Tells where to insert the new elements.
	 * @param Count Number of elements to add.
	 * @see Insert, InsertUninitialized, InsertDefaulted
	 */
	void InsertZeroed(SizeType Index, SizeType Count = 1)
	{
		InsertUninitializedImpl(Index, Count);
		FMemory::Memzero(GetData() + Index, Count * sizeof(ElementType));
	}

	/**
	 * Inserts a zeroed element into the array at given location.
	 *
	 * Caution, InsertZeroed_GetRef() will create an element without calling the
	 * constructor and this is not appropriate for element types that require
	 * a constructor to function properly.
	 *
	 * @param Index Tells where to insert the new element.
	 * @return A reference to the newly-inserted element.
	 * @see Insert_GetRef, InsertDefaulted_GetRef
	 */
	ElementType& InsertZeroed_GetRef(SizeType Index)
	{
		InsertUninitializedImpl(Index, 1);
		ElementType* Ptr = GetData() + Index;
		FMemory::Memzero(Ptr, sizeof(ElementType));
		return *Ptr;
	}

	/**
	 * Inserts a given number of default-constructed elements into the array at a given
	 * location.
	 *
	 * @param Index Tells where to insert the new elements.
	 * @param Count Number of elements to add.
	 * @see Insert, InsertUninitialized, InsertZeroed
	 */
	void InsertDefaulted(SizeType Index, SizeType Count = 1)
	{
		InsertUninitializedImpl(Index, Count);
		DefaultConstructItems<ElementType>(GetData() + Index, Count);
	}

	/**
	 * Inserts a default-constructed element into the array at a given
	 * location.
	 *
	 * @param Index Tells where to insert the new element.
	 * @return A reference to the newly-inserted element.
	 * @see Insert_GetRef, InsertZeroed_GetRef
	 */
	ElementType& InsertDefaulted_GetRef(SizeType Index)
	{
		InsertUninitializedImpl(Index, 1);
		ElementType* Ptr = GetData() + Index;
		DefaultConstructItems<ElementType>(Ptr, 1);
		return *Ptr;
	}

	/**
	 * Inserts given elements into the array at given location.
	 *
	 * @param Items Array of elements to insert.
	 * @param InIndex Tells where to insert the new elements.
	 * @returns Location at which the item was inserted.
	 */
	SizeType Insert(std::initializer_list<ElementType> InitList, const SizeType InIndex)
	{
		SizeType NumNewElements = (SizeType)InitList.size();

		InsertUninitializedImpl(InIndex, NumNewElements);
		ConstructItems<ElementType>(GetData() + InIndex, InitList.begin(), NumNewElements);

		return InIndex;
	}

	/**
	 * Inserts given elements into the array at given location.
	 *
	 * @param Items Array of elements to insert.
	 * @param InIndex Tells where to insert the new elements.
	 * @returns Location at which the item was inserted.
	 */
	template <typename OtherAllocator>
	SizeType Insert(const TArray<ElementType, OtherAllocator>& Items, const SizeType InIndex)
	{
		check((const void*)this != (const void*)&Items);

		auto NumNewElements = Items.Num();

		InsertUninitializedImpl(InIndex, NumNewElements);
		ConstructItems<ElementType>(GetData() + InIndex, Items.GetData(), NumNewElements);

		return InIndex;
	}

	/**
	 * Inserts given elements into the array at given location.
	 *
	 * @param Items Array of elements to insert.
	 * @param InIndex Tells where to insert the new elements.
	 * @returns Location at which the item was inserted.
	 */
	template <typename OtherAllocator>
	SizeType Insert(TArray<ElementType, OtherAllocator>&& Items, const SizeType InIndex)
	{
		check((const void*)this != (const void*)&Items);

		auto NumNewElements = Items.Num();

		InsertUninitializedImpl(InIndex, NumNewElements);
		RelocateConstructItems<ElementType>(GetData() + InIndex, Items.GetData(), NumNewElements);
		Items.ArrayNum = 0;

		return InIndex;
	}

	/**
	 * Inserts a raw array of elements at a particular index in the TArray.
	 *
	 * @param Ptr A pointer to an array of elements to add.
	 * @param Count The number of elements to insert from Ptr.
	 * @param Index The index to insert the elements at.
	 * @return The index of the first element inserted.
	 * @see Add, Remove
	 */
	SizeType Insert(const ElementType* Ptr, SizeType Count, SizeType Index)
	{
		check(Ptr != nullptr);

		InsertUninitializedImpl(Index, Count);
		ConstructItems<ElementType>(GetData() + Index, Ptr, Count);

		return Index;
	}

	/**
	 * Checks that the specified address is not part of an element within the
	 * container. Used for implementations to check that reference arguments
	 * aren't going to be invalidated by possible reallocation.
	 *
	 * @param Addr The address to check.
	 * @see Add, Remove
	 */
	FORCEINLINE void CheckAddress(const ElementType* Addr) const
	{
		checkf(Addr < GetData() || Addr >= (GetData() + ArrayMax), TEXT("Attempting to use a container element (%p) which already comes from the container being modified (%p, ArrayMax: %d, ArrayNum: %d, SizeofElement: %d)!"), Addr, GetData(), ArrayMax, ArrayNum, sizeof(ElementType));
	}

	/**
	 * Inserts a given element into the array at given location. Move semantics
	 * version.
	 *
	 * @param Item The element to insert.
	 * @param Index Tells where to insert the new elements.
	 * @returns Location at which the insert was done.
	 * @see Add, Remove
	 */
	SizeType Insert(ElementType&& Item, SizeType Index)
	{
		CheckAddress(&Item);

		// construct a copy in place at Index (this new operator will insert at 
		// Index, then construct that memory with Item)
		InsertUninitializedImpl(Index, 1);
		new(GetData() + Index) ElementType(MoveTempIfPossible(Item));
		return Index;
	}

	/**
	 * Inserts a given element into the array at given location.
	 *
	 * @param Item The element to insert.
	 * @param Index Tells where to insert the new elements.
	 * @returns Location at which the insert was done.
	 * @see Add, Remove
	 */
	SizeType Insert(const ElementType& Item, SizeType Index)
	{
		CheckAddress(&Item);

		// construct a copy in place at Index (this new operator will insert at 
		// Index, then construct that memory with Item)
		InsertUninitializedImpl(Index, 1);
		new(GetData() + Index) ElementType(Item);
		return Index;
	}

	/**
	 * Inserts a given element into the array at given location. Move semantics
	 * version.
	 *
	 * @param Item The element to insert.
	 * @param Index Tells where to insert the new element.
	 * @return A reference to the newly-inserted element.
	 * @see Add, Remove
	 */
	ElementType& Insert_GetRef(ElementType&& Item, SizeType Index)
	{
		CheckAddress(&Item);

		// construct a copy in place at Index (this new operator will insert at 
		// Index, then construct that memory with Item)
		InsertUninitializedImpl(Index, 1);
		ElementType* Ptr = GetData() + Index;
		new(Ptr) ElementType(MoveTempIfPossible(Item));
		return *Ptr;
	}

	/**
	 * Inserts a given element into the array at given location.
	 *
	 * @param Item The element to insert.
	 * @param Index Tells where to insert the new element.
	 * @return A reference to the newly-inserted element.
	 * @see Add, Remove
	 */
	ElementType& Insert_GetRef(const ElementType& Item, SizeType Index)
	{
		CheckAddress(&Item);

		// construct a copy in place at Index (this new operator will insert at 
		// Index, then construct that memory with Item)
		InsertUninitializedImpl(Index, 1);
		ElementType* Ptr = GetData() + Index;
		new(Ptr) ElementType(Item);
		return *Ptr;
	}

private:
	void RemoveAtImpl(SizeType Index, SizeType Count, bool bAllowShrinking)
	{
		if (Count)
		{
			CheckInvariants();
			checkSlow((Count >= 0) & (Index >= 0) & (Index + Count <= ArrayNum));

			DestructItems(GetData() + Index, Count);

			// Skip memmove in the common case that there is nothing to move.
			SizeType NumToMove = ArrayNum - Index - Count;
			if (NumToMove)
			{
				FMemory::Memmove
					(
					(uint8*)AllocatorInstance.GetAllocation() + (Index)* sizeof(ElementType),
					(uint8*)AllocatorInstance.GetAllocation() + (Index + Count) * sizeof(ElementType),
					NumToMove * sizeof(ElementType)
					);
			}
			ArrayNum -= Count;

			if (bAllowShrinking)
			{
				ResizeShrink();
			}
		}
	}

public:
	/**
	 * Removes an element (or elements) at given location optionally shrinking
	 * the array.
	 *
	 * @param Index Location in array of the element to remove.
	 * @param Count (Optional) Number of elements to remove. Default is 1.
	 * @param bAllowShrinking (Optional) Tells if this call can shrink array if suitable after remove. Default is true.
	 */
	FORCEINLINE void RemoveAt(SizeType Index)
	{
		RemoveAtImpl(Index, 1, true);
	}

	/**
	 * Removes an element (or elements) at given location optionally shrinking
	 * the array.
	 *
	 * @param Index Location in array of the element to remove.
	 * @param Count (Optional) Number of elements to remove. Default is 1.
	 * @param bAllowShrinking (Optional) Tells if this call can shrink array if suitable after remove. Default is true.
	 */
	template <typename CountType>
	FORCEINLINE void RemoveAt(SizeType Index, CountType Count, bool bAllowShrinking = true)
	{
		static_assert(!TAreTypesEqual<CountType, bool>::Value, "TArray::RemoveAt: unexpected bool passed as the Count argument");
		RemoveAtImpl(Index, (SizeType)Count, bAllowShrinking);
	}

private:
	void RemoveAtSwapImpl(SizeType Index, SizeType Count = 1, bool bAllowShrinking = true)
	{
		if (Count)
		{
			CheckInvariants();
			checkSlow((Count >= 0) & (Index >= 0) & (Index + Count <= ArrayNum));

			DestructItems(GetData() + Index, Count);

			// Replace the elements in the hole created by the removal with elements from the end of the array, so the range of indices used by the array is contiguous.
			const SizeType NumElementsInHole = Count;
			const SizeType NumElementsAfterHole = ArrayNum - (Index + Count);
			const SizeType NumElementsToMoveIntoHole = FPlatformMath::Min(NumElementsInHole, NumElementsAfterHole);
			if (NumElementsToMoveIntoHole)
			{
				FMemory::Memcpy(
					(uint8*)AllocatorInstance.GetAllocation() + (Index)* sizeof(ElementType),
					(uint8*)AllocatorInstance.GetAllocation() + (ArrayNum - NumElementsToMoveIntoHole) * sizeof(ElementType),
					NumElementsToMoveIntoHole * sizeof(ElementType)
					);
			}
			ArrayNum -= Count;

			if (bAllowShrinking)
			{
				ResizeShrink();
			}
		}
	}

public:
	/**
	 * Removes an element (or elements) at given location optionally shrinking
	 * the array.
	 *
	 * This version is much more efficient than RemoveAt (O(Count) instead of
	 * O(ArrayNum)), but does not preserve the order.
	 *
	 * @param Index Location in array of the element to remove.
	 * @param Count (Optional) Number of elements to remove. Default is 1.
	 * @param bAllowShrinking (Optional) Tells if this call can shrink array if
	 *                        suitable after remove. Default is true.
	 */
	FORCEINLINE void RemoveAtSwap(SizeType Index)
	{
		RemoveAtSwapImpl(Index, 1, true);
	}

	/**
	 * Removes an element (or elements) at given location optionally shrinking
	 * the array.
	 *
	 * This version is much more efficient than RemoveAt (O(Count) instead of
	 * O(ArrayNum)), but does not preserve the order.
	 *
	 * @param Index Location in array of the element to remove.
	 * @param Count (Optional) Number of elements to remove. Default is 1.
	 * @param bAllowShrinking (Optional) Tells if this call can shrink array if
	 *                        suitable after remove. Default is true.
	 */
	template <typename CountType>
	FORCEINLINE void RemoveAtSwap(SizeType Index, CountType Count, bool bAllowShrinking = true)
	{
		static_assert(!TAreTypesEqual<CountType, bool>::Value, "TArray::RemoveAtSwap: unexpected bool passed as the Count argument");
		RemoveAtSwapImpl(Index, Count, bAllowShrinking);
	}

	/**
	 * Same as empty, but doesn't change memory allocations, unless the new size is larger than
	 * the current array. It calls the destructors on held items if needed and then zeros the ArrayNum.
	 *
	 * @param NewSize The expected usage size after calling this function.
	 */
	void Reset(SizeType NewSize = 0)
	{
		// If we have space to hold the excepted size, then don't reallocate
		if (NewSize <= ArrayMax)
		{
			DestructItems(GetData(), ArrayNum);
			ArrayNum = 0;
		}
		else
		{
			Empty(NewSize);
		}
	}

	/**
	 * Empties the array. It calls the destructors on held items if needed.
	 *
	 * @param Slack (Optional) The expected usage size after empty operation. Default is 0.
	 */
	void Empty(SizeType Slack = 0)
	{
		DestructItems(GetData(), ArrayNum);

		checkSlow(Slack >= 0);
		ArrayNum = 0;

		if (ArrayMax != Slack)
		{
			ResizeTo(Slack);
		}
	}

	/**
	 * Resizes array to given number of elements.
	 *
	 * @param NewNum New size of the array.
	 * @param bAllowShrinking Tell if this function can shrink the memory in-use if suitable.
	 */
	void SetNum(SizeType NewNum, bool bAllowShrinking = true)
	{
		if (NewNum > Num())
		{
			const SizeType Diff = NewNum - ArrayNum;
			const SizeType Index = AddUninitialized(Diff);
			DefaultConstructItems<ElementType>((uint8*)AllocatorInstance.GetAllocation() + Index * sizeof(ElementType), Diff);
		}
		else if (NewNum < Num())
		{
			RemoveAt(NewNum, Num() - NewNum, bAllowShrinking);
		}
	}

	/**
	 * Resizes array to given number of elements. New elements will be zeroed.
	 *
	 * @param NewNum New size of the array.
	 */
	void SetNumZeroed(SizeType NewNum, bool bAllowShrinking = true)
	{
		if (NewNum > Num())
		{
			AddZeroed(NewNum - Num());
		}
		else if (NewNum < Num())
		{
			RemoveAt(NewNum, Num() - NewNum, bAllowShrinking);
		}
	}

	/**
	 * Resizes array to given number of elements. New elements will be uninitialized.
	 *
	 * @param NewNum New size of the array.
	 */
	void SetNumUninitialized(SizeType NewNum, bool bAllowShrinking = true)
	{
		if (NewNum > Num())
		{
			AddUninitialized(NewNum - Num());
		}
		else if (NewNum < Num())
		{
			RemoveAt(NewNum, Num() - NewNum, bAllowShrinking);
		}
	}

	/**
	 * Does nothing except setting the new number of elements in the array. Does not destruct items, does not de-allocate memory.
	 * @param NewNum New number of elements in the array, must be <= the current number of elements in the array.
	 */
	void SetNumUnsafeInternal(SizeType NewNum)
	{
		checkSlow(NewNum <= Num() && NewNum >= 0);
		ArrayNum = NewNum;
	}

	/**
	 * Appends the specified array to this array.
	 *
	 * AllocatorType changing version.
	 *
	 * @param Source The array to append.
	 * @see Add, Insert
	 */
	template <typename OtherElementType, typename OtherAllocatorType>
	void Append(const TArray<OtherElementType, OtherAllocatorType>& Source)
	{
		check((void*)this != (void*)&Source);

		SizeType SourceCount = Source.Num();

		// Do nothing if the source is empty.
		if (!SourceCount)
		{
			return;
		}

		// Allocate memory for the new elements.
		Reserve(ArrayNum + SourceCount);
		ConstructItems<ElementType>(GetData() + ArrayNum, Source.GetData(), SourceCount);

		ArrayNum += SourceCount;
	}

	/**
	 * Appends the specified array to this array.
	 *
	 * @param Source The array to append.
	 * @see Add, Insert
	 */
	template <typename OtherElementType, typename OtherAllocator>
	void Append(TArray<OtherElementType, OtherAllocator>&& Source)
	{
		check((void*)this != (void*)&Source);

		SizeType SourceCount = Source.Num();

		// Do nothing if the source is empty.
		if (!SourceCount)
		{
			return;
		}

		// Allocate memory for the new elements.
		Reserve(ArrayNum + SourceCount);
		RelocateConstructItems<ElementType>(GetData() + ArrayNum, Source.GetData(), SourceCount);
		Source.ArrayNum = 0;

		ArrayNum += SourceCount;
	}

	/**
	 * Adds a raw array of elements to the end of the TArray.
	 *
	 * @param Ptr   A pointer to an array of elements to add.
	 * @param Count The number of elements to insert from Ptr.
	 * @see Add, Insert
	 */
	void Append(const ElementType* Ptr, SizeType Count)
	{
		check(Ptr != nullptr || Count == 0);

		SizeType Pos = AddUninitialized(Count);
		ConstructItems<ElementType>(GetData() + Pos, Ptr, Count);
	}

	/**
	 * Adds an initializer list of elements to the end of the TArray.
	 *
	 * @param InitList The initializer list of elements to add.
	 * @see Add, Insert
	 */
	FORCEINLINE void Append(std::initializer_list<ElementType> InitList)
	{
		SizeType Count = (SizeType)InitList.size();

		SizeType Pos = AddUninitialized(Count);
		ConstructItems<ElementType>(GetData() + Pos, InitList.begin(), Count);
	}

	/**
	 * Appends the specified array to this array.
	 * Cannot append to self.
	 *
	 * Move semantics version.
	 *
	 * @param Other The array to append.
	 */
	TArray& operator+=(TArray&& Other)
	{
		Append(MoveTemp(Other));
		return *this;
	}

	/**
	 * Appends the specified array to this array.
	 * Cannot append to self.
	 *
	 * @param Other The array to append.
	 */
	TArray& operator+=(const TArray& Other)
	{
		Append(Other);
		return *this;
	}

	/**
	 * Appends the specified initializer list to this array.
	 *
	 * @param InitList The initializer list to append.
	 */
	TArray& operator+=(std::initializer_list<ElementType> InitList)
	{
		Append(InitList);
		return *this;
	}

	/**
	 * Constructs a new item at the end of the array, possibly reallocating the whole array to fit.
	 *
	 * @param Args	The arguments to forward to the constructor of the new item.
	 * @return		Index to the new item
	 */
	template <typename... ArgsType>
	FORCEINLINE SizeType Emplace(ArgsType&&... Args)
	{
		const SizeType Index = AddUninitialized(1);
		new(GetData() + Index) ElementType(Forward<ArgsType>(Args)...);
		return Index;
	}

	/**
	 * Constructs a new item at the end of the array, possibly reallocating the whole array to fit.
	 *
	 * @param Args	The arguments to forward to the constructor of the new item.
	 * @return A reference to the newly-inserted element.
	 */
	template <typename... ArgsType>
	FORCEINLINE ElementType& Emplace_GetRef(ArgsType&&... Args)
	{
		const SizeType Index = AddUninitialized(1);
		ElementType* Ptr = GetData() + Index;
		new(Ptr) ElementType(Forward<ArgsType>(Args)...);
		return *Ptr;
	}

	/**
	 * Constructs a new item at a specified index, possibly reallocating the whole array to fit.
	 *
	 * @param Index	The index to add the item at.
	 * @param Args	The arguments to forward to the constructor of the new item.
	 */
	template <typename... ArgsType>
	FORCEINLINE void EmplaceAt(SizeType Index, ArgsType&&... Args)
	{
		InsertUninitializedImpl(Index, 1);
		new(GetData() + Index) ElementType(Forward<ArgsType>(Args)...);
	}

	/**
	 * Constructs a new item at a specified index, possibly reallocating the whole array to fit.
	 *
	 * @param Index	The index to add the item at.
	 * @param Args	The arguments to forward to the constructor of the new item.
	 * @return A reference to the newly-inserted element.
	 */
	template <typename... ArgsType>
	FORCEINLINE ElementType& EmplaceAt_GetRef(SizeType Index, ArgsType&&... Args)
	{
		InsertUninitializedImpl(Index, 1);
		ElementType* Ptr = GetData() + Index;
		new(Ptr) ElementType(Forward<ArgsType>(Args)...);
		return *Ptr;
	}

	/**
	 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
	 *
	 * Move semantics version.
	 *
	 * @param Item The item to add
	 * @return Index to the new item
	 * @see AddDefaulted, AddUnique, AddZeroed, Append, Insert
	 */
	FORCEINLINE SizeType Add(ElementType&& Item)
	{
		CheckAddress(&Item);
		return Emplace(MoveTempIfPossible(Item));
	}

	/**
	 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
	 *
	 * @param Item The item to add
	 * @return Index to the new item
	 * @see AddDefaulted, AddUnique, AddZeroed, Append, Insert
	 */
	FORCEINLINE SizeType Add(const ElementType& Item)
	{
		CheckAddress(&Item);
		return Emplace(Item);
	}

	/**
	 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
	 *
	 * Move semantics version.
	 *
	 * @param Item The item to add
	 * @return A reference to the newly-inserted element.
	 * @see AddDefaulted_GetRef, AddUnique_GetRef, AddZeroed_GetRef, Insert_GetRef
	 */
	FORCEINLINE ElementType& Add_GetRef(ElementType&& Item)
	{
		CheckAddress(&Item);
		return Emplace_GetRef(MoveTempIfPossible(Item));
	}

	/**
	 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
	 *
	 * @param Item The item to add
	 * @return A reference to the newly-inserted element.
	 * @see AddDefaulted_GetRef, AddUnique_GetRef, AddZeroed_GetRef, Insert_GetRef
	 */
	FORCEINLINE ElementType& Add_GetRef(const ElementType& Item)
	{
		CheckAddress(&Item);
		return Emplace_GetRef(Item);
	}

	/**
	 * Adds new items to the end of the array, possibly reallocating the whole
	 * array to fit. The new items will be zeroed.
	 *
	 * Caution, AddZeroed() will create elements without calling the
	 * constructor and this is not appropriate for element types that require
	 * a constructor to function properly.
	 *
	 * @param  Count  The number of new items to add.
	 * @return Index to the first of the new items.
	 * @see Add, AddDefaulted, AddUnique, Append, Insert
	 */
	SizeType AddZeroed(SizeType Count = 1)
	{
		const SizeType Index = AddUninitialized(Count);
		FMemory::Memzero((uint8*)AllocatorInstance.GetAllocation() + Index*sizeof(ElementType), Count*sizeof(ElementType));
		return Index;
	}

	/**
	 * Adds a new item to the end of the array, possibly reallocating the whole
	 * array to fit. The new item will be zeroed.
	 *
	 * Caution, AddZeroed_GetRef() will create elements without calling the
	 * constructor and this is not appropriate for element types that require
	 * a constructor to function properly.
	 *
	 * @return A reference to the newly-inserted element.
	 * @see Add_GetRef, AddDefaulted_GetRef, AddUnique_GetRef, Insert_GetRef
	 */
	ElementType& AddZeroed_GetRef()
	{
		const SizeType Index = AddUninitialized(1);
		ElementType* Ptr = GetData() + Index;
		FMemory::Memzero(Ptr, sizeof(ElementType));
		return *Ptr;
	}

	/**
	 * Adds new items to the end of the array, possibly reallocating the whole
	 * array to fit. The new items will be default-constructed.
	 *
	 * @param  Count  The number of new items to add.
	 * @return Index to the first of the new items.
	 * @see Add, AddZeroed, AddUnique, Append, Insert
	 */
	SizeType AddDefaulted(SizeType Count = 1)
	{
		const SizeType Index = AddUninitialized(Count);
		DefaultConstructItems<ElementType>((uint8*)AllocatorInstance.GetAllocation() + Index * sizeof(ElementType), Count);
		return Index;
	}

	/**
	 * Add a new item to the end of the array, possibly reallocating the whole
	 * array to fit. The new item will be default-constructed.
	 *
	 * @return A reference to the newly-inserted element.
	 * @see Add_GetRef, AddZeroed_GetRef, AddUnique_GetRef, Insert_GetRef
	 */
	ElementType& AddDefaulted_GetRef()
	{
		const SizeType Index = AddUninitialized(1);
		ElementType* Ptr = GetData() + Index;
		DefaultConstructItems<ElementType>(Ptr, 1);
		return *Ptr;
	}

	/** Implicit conversion operator to container of compatible element type. */
	template <
		typename AliasElementType = ElementType,
		std::enable_if_t<TIsContainerElementTypeReinterpretable<AliasElementType>::Value>* = nullptr
		>
	operator TArray<typename TContainerElementTypeCompatibility<AliasElementType>::ReinterpretType, AllocatorType>& ()
	{
		using ElementCompat = TContainerElementTypeCompatibility<ElementType>;
		ElementCompat::ReinterpretRange(begin(), end());
		return *reinterpret_cast<TArray<typename ElementCompat::ReinterpretType>*>(this);
	}

	/** Implicit conversion operator to constant container of compatible element type. */
	template <
		typename AliasElementType = ElementType,
		typename std::enable_if_t<TIsContainerElementTypeReinterpretable<AliasElementType>::Value>* = nullptr
		>
	operator const TArray<typename TContainerElementTypeCompatibility<AliasElementType>::ReinterpretType, AllocatorType>& () const
	{
		using ElementCompat = TContainerElementTypeCompatibility<ElementType>;
		ElementCompat::ReinterpretRange(begin(), end());
		return *reinterpret_cast<const TArray<typename ElementCompat::ReinterpretType>*>(this);
	}

	/**
	 * Move assignment operator.
	 * Compatible element type version.
	 *
	 * @param Other Array to assign and move from.
	 */
	template <
		typename AliasElementType = ElementType,
		typename std::enable_if_t<TIsContainerElementTypeCopyable<AliasElementType>::Value>* = nullptr
	>
	TArray& operator=(TArray<typename TContainerElementTypeCompatibility<ElementType>::CopyFromOtherType, AllocatorType>&& Other)
	{
		TContainerElementTypeCompatibility<ElementType>::CopyingFromOtherType();
		DestructItems(GetData(), ArrayNum);
		MoveOrCopy(*this, Other, ArrayMax);
		return *this; 
	}

	/**
	 * Assignment operator. First deletes all currently contained elements
	 * and then copies from other array.
	 * Compatible element type version.
	 *
	 * @param Other The source array to assign from.
	 */
	template <
		typename OtherAllocator,
		typename AliasElementType = ElementType,
		typename std::enable_if_t<TIsContainerElementTypeCopyable<AliasElementType>::Value>* = nullptr
	>
	TArray& operator=(const TArray<typename TContainerElementTypeCompatibility<ElementType>::CopyFromOtherType, OtherAllocator>& Other)
	{
		TContainerElementTypeCompatibility<ElementType>::CopyingFromOtherType();
		DestructItems(GetData(), ArrayNum);
		CopyToEmpty(Other.GetData(), Other.Num(), ArrayMax, 0);
		return *this;
	}

	/**
	 * Inserts given elements into the array at given location.
	 * Compatible element type version.
	 *
	 * @param Items Array of elements to insert.
	 * @param InIndex Tells where to insert the new elements.
	 * @returns Location at which the item was inserted.
	 */
	template <
		typename OtherAllocator,
		typename AliasElementType = ElementType,
		typename std::enable_if_t<TIsContainerElementTypeCopyable<AliasElementType>::Value>* = nullptr
	>
	SizeType Insert(const TArray<typename TContainerElementTypeCompatibility<ElementType>::CopyFromOtherType, OtherAllocator>& Items, const SizeType InIndex)
	{
		TContainerElementTypeCompatibility<ElementType>::CopyingFromOtherType();

		auto NumNewElements = Items.Num();

		InsertUninitializedImpl(InIndex, NumNewElements);
		ConstructItems<ElementType>(GetData() + InIndex, Items.GetData(), NumNewElements);

		return InIndex;
	}

	/**
	 * Inserts given elements into the array at given location.
	 * Compatible element type version.
	 *
	 * @param Items Array of elements to insert.
	 * @param InIndex Tells where to insert the new elements.
	 * @returns Location at which the item was inserted.
	 */
	template <
		typename OtherAllocator,
		typename AliasElementType = ElementType,
		typename std::enable_if_t<TIsContainerElementTypeCopyable<AliasElementType>::Value>* = nullptr
	>
	SizeType Insert(TArray<typename TContainerElementTypeCompatibility<ElementType>::CopyFromOtherType, OtherAllocator>&& Items, const SizeType InIndex)
	{
		check((const void*)this != (const void*)&Items);
		TContainerElementTypeCompatibility<ElementType>::CopyingFromOtherType();

		auto NumNewElements = Items.Num();

		InsertUninitializedImpl(InIndex, NumNewElements);
		RelocateConstructItems<ElementType>(GetData() + InIndex, Items.GetData(), NumNewElements);
		Items.ArrayNum = 0;

		return InIndex;
	}

	/**
	 * Adds a raw array of elements to the end of the TArray.
	 * Compatible element type version.
	 *
	 * @param Ptr   A pointer to an array of elements to add.
	 * @param Count The number of elements to insert from Ptr.
	 * @see Add, Insert
	 */
	template <
		typename AliasElementType = ElementType,
		typename std::enable_if_t<TIsContainerElementTypeCopyable<AliasElementType>::Value>* = nullptr
	>
	void Append(const typename TContainerElementTypeCompatibility<ElementType>::CopyFromOtherType* Ptr, SizeType Count)
	{
		check(Ptr != nullptr || Count == 0);
		TContainerElementTypeCompatibility<ElementType>::CopyingFromOtherType();

		SizeType Pos = AddUninitialized(Count);
		ConstructItems<ElementType>(GetData() + Pos, Ptr, Count);
	}

private:

	/**
	 * Adds unique element to array if it doesn't exist.
	 *
	 * @param Args Item to add.
	 * @returns Index of the element in the array.
	 */
	template <typename ArgsType>
	SizeType AddUniqueImpl(ArgsType&& Args)
	{
		SizeType Index;
		if (Find(Args, Index))
		{
			return Index;
		}

		return Add(Forward<ArgsType>(Args));
	}

public:

	/**
	 * Adds unique element to array if it doesn't exist.
	 *
	 * Move semantics version.
	 *
	 * @param Args Item to add.
	 * @returns Index of the element in the array.
	 * @see Add, AddDefaulted, AddZeroed, Append, Insert
	 */
	FORCEINLINE SizeType AddUnique(ElementType&& Item) { return AddUniqueImpl(MoveTempIfPossible(Item)); }

	/**
	 * Adds unique element to array if it doesn't exist.
	 *
	 * @param Args Item to add.
	 * @returns Index of the element in the array.
	 * @see Add, AddDefaulted, AddZeroed, Append, Insert
	 */
	FORCEINLINE SizeType AddUnique(const ElementType& Item) { return AddUniqueImpl(Item); }

	/**
	 * Reserves memory such that the array can contain at least Number elements.
	 *
	 * @param Number The number of elements that the array should be able to contain after allocation.
	 * @see Shrink
	 */
	FORCEINLINE void Reserve(SizeType Number)
	{
		checkSlow(Number >= 0);
		if (Number > ArrayMax)
		{
			ResizeTo(Number);
		}
	}

	/**
	 * Sets the size of the array, filling it with the given element.
	 *
	 * @param Element The element to fill array with.
	 * @param Number The number of elements that the array should be able to contain after allocation.
	 */
	void Init(const ElementType& Element, SizeType Number)
	{
		Empty(Number);
		for (SizeType Index = 0; Index < Number; ++Index)
		{
			new(*this) ElementType(Element);
		}
	}

	/**
	 * Removes the first occurrence of the specified item in the array,
	 * maintaining order but not indices.
	 *
	 * @param Item The item to remove.
	 * @returns The number of items removed. For RemoveSingleItem, this is always either 0 or 1.
	 * @see Add, Insert, Remove, RemoveAll, RemoveAllSwap
	 */
	SizeType RemoveSingle(const ElementType& Item)
	{
		SizeType Index = Find(Item);
		if (Index == INDEX_NONE)
		{
			return 0;
		}

		auto* RemovePtr = GetData() + Index;

		// Destruct items that match the specified Item.
		DestructItems(RemovePtr, 1);
		const SizeType NextIndex = Index + 1;
		RelocateConstructItems<ElementType>(RemovePtr, RemovePtr + 1, ArrayNum - (Index + 1));

		// Update the array count
		--ArrayNum;

		// Removed one item
		return 1;
	}

	/**
	 * Removes as many instances of Item as there are in the array, maintaining
	 * order but not indices.
	 *
	 * @param Item Item to remove from array.
	 * @returns Number of removed elements.
	 * @see Add, Insert, RemoveAll, RemoveAllSwap, RemoveSingle, RemoveSwap
	 */
	SizeType Remove(const ElementType& Item)
	{
		CheckAddress(&Item);

		// Element is non-const to preserve compatibility with existing code with a non-const operator==() member function
		return RemoveAll([&Item](ElementType& Element) { return Element == Item; });
	}

	/**
	 * Remove all instances that match the predicate, maintaining order but not indices
	 * Optimized to work with runs of matches/non-matches
	 *
	 * @param Predicate Predicate class instance
	 * @returns Number of removed elements.
	 * @see Add, Insert, RemoveAllSwap, RemoveSingle, RemoveSwap
	 */
	template <class PREDICATE_CLASS>
	SizeType RemoveAll(const PREDICATE_CLASS& Predicate)
	{
		const SizeType OriginalNum = ArrayNum;
		if (!OriginalNum)
		{
			return 0; // nothing to do, loop assumes one item so need to deal with this edge case here
		}

		SizeType WriteIndex = 0;
		SizeType ReadIndex = 0;
		bool NotMatch = !::Invoke(Predicate, GetData()[ReadIndex]); // use a ! to guarantee it can't be anything other than zero or one
		do
		{
			SizeType RunStartIndex = ReadIndex++;
			while (ReadIndex < OriginalNum && NotMatch == !::Invoke(Predicate, GetData()[ReadIndex]))
			{
				ReadIndex++;
			}
			SizeType RunLength = ReadIndex - RunStartIndex;
			checkSlow(RunLength > 0);
			if (NotMatch)
			{
				// this was a non-matching run, we need to move it
				if (WriteIndex != RunStartIndex)
				{
					FMemory::Memmove(&GetData()[WriteIndex], &GetData()[RunStartIndex], sizeof(ElementType)* RunLength);
				}
				WriteIndex += RunLength;
			}
			else
			{
				// this was a matching run, delete it
				DestructItems(GetData() + RunStartIndex, RunLength);
			}
			NotMatch = !NotMatch;
		} while (ReadIndex < OriginalNum);

		ArrayNum = WriteIndex;
		return OriginalNum - ArrayNum;
	}

	/**
	 * Remove all instances that match the predicate
	 *
	 * @param Predicate Predicate class instance
	 * @param bAllowShrinking Tell if this function can shrink the memory in-use if suitable.
	 * @see Remove, RemoveSingle, RemoveSingleSwap, RemoveSwap
	 */
	template <class PREDICATE_CLASS>
	SizeType RemoveAllSwap(const PREDICATE_CLASS& Predicate, bool bAllowShrinking = true)
	{
		bool bRemoved = false;
		const SizeType OriginalNum = ArrayNum;
		for (SizeType ItemIndex = 0; ItemIndex < Num();)
		{
			if (::Invoke(Predicate, (*this)[ItemIndex]))
			{
				bRemoved = true;
				RemoveAtSwap(ItemIndex, 1, false);
			}
			else
			{
				++ItemIndex;
			}
		}

		if (bRemoved && bAllowShrinking)
		{
			ResizeShrink();
		}

		return OriginalNum - ArrayNum;
	}

	/**
	 * Removes the first occurrence of the specified item in the array. This version is much more efficient
	 * O(Count) instead of O(ArrayNum), but does not preserve the order
	 *
	 * @param Item The item to remove
	 * @param bAllowShrinking Tell if this function can shrink the memory in-use if suitable.
	 *
	 * @returns The number of items removed. For RemoveSingleItem, this is always either 0 or 1.
	 * @see Add, Insert, Remove, RemoveAll, RemoveAllSwap, RemoveSwap
	 */
	SizeType RemoveSingleSwap(const ElementType& Item, bool bAllowShrinking = true)
	{
		SizeType Index = Find(Item);
		if (Index == INDEX_NONE)
		{
			return 0;
		}

		RemoveAtSwap(Index, 1, bAllowShrinking);

		// Removed one item
		return 1;
	}

	/**
	 * Removes item from the array.
	 *
	 * This version is much more efficient, because it uses RemoveAtSwap
	 * internally which is O(Count) instead of RemoveAt which is O(ArrayNum),
	 * but does not preserve the order.
	 *
	 * @returns Number of elements removed.
	 * @see Add, Insert, Remove, RemoveAll, RemoveAllSwap
	 */
	SizeType RemoveSwap(const ElementType& Item, bool bAllowShrinking = true)
	{
		CheckAddress(&Item);

		const SizeType OriginalNum = ArrayNum;
		bool bRemoved = false;
		for (SizeType Index = 0; Index < ArrayNum; Index++)
		{
			if ((*this)[Index] == Item)
			{
				bRemoved = true;
				RemoveAtSwap(Index--, 1, false);
			}
		}

		if (bRemoved && bAllowShrinking)
		{
			ResizeShrink();
		}

		return OriginalNum - ArrayNum;
	}

	/**
	 * Element-wise array memory swap.
	 *
	 * @param FirstIndexToSwap Position of the first element to swap.
	 * @param SecondIndexToSwap Position of the second element to swap.
	 */
	FORCEINLINE void SwapMemory(SizeType FirstIndexToSwap, SizeType SecondIndexToSwap)
	{
		FMemory::Memswap(
			(uint8*)AllocatorInstance.GetAllocation() + (sizeof(ElementType)*FirstIndexToSwap),
			(uint8*)AllocatorInstance.GetAllocation() + (sizeof(ElementType)*SecondIndexToSwap),
			sizeof(ElementType)
			);
	}

	/**
	 * Element-wise array element swap.
	 *
	 * This version is doing more sanity checks than SwapMemory.
	 *
	 * @param FirstIndexToSwap Position of the first element to swap.
	 * @param SecondIndexToSwap Position of the second element to swap.
	 */
	FORCEINLINE void Swap(SizeType FirstIndexToSwap, SizeType SecondIndexToSwap)
	{
		check((FirstIndexToSwap >= 0) && (SecondIndexToSwap >= 0));
		check((ArrayNum > FirstIndexToSwap) && (ArrayNum > SecondIndexToSwap));
		if (FirstIndexToSwap != SecondIndexToSwap)
		{
			SwapMemory(FirstIndexToSwap, SecondIndexToSwap);
		}
	}

	/**
	 * Searches for the first entry of the specified type, will only work with
	 * TArray<UObject*>. Optionally return the item's index, and can specify
	 * the start index.
	 *
	 * @param Item (Optional output) If it's not null, then it will point to
	 *             the found element. Untouched if element hasn't been found.
	 * @param ItemIndex (Optional output) If it's not null, then it will be set
	 *             to the position of found element in the array. Untouched if
	 *             element hasn't been found.
	 * @param StartIndex (Optional) Index in array at which the function should
	 *             start to look for element.
	 * @returns True if element was found. False otherwise.
	 */
	template<typename SearchType>
	bool FindItemByClass(SearchType **Item = nullptr, SizeType *ItemIndex = nullptr, SizeType StartIndex = 0) const
	{
		UClass* SearchClass = SearchType::StaticClass();
		for (SizeType Idx = StartIndex; Idx < ArrayNum; Idx++)
		{
			if ((*this)[Idx] != nullptr && (*this)[Idx]->IsA(SearchClass))
			{
				if (Item != nullptr)
				{
					*Item = (SearchType*)((*this)[Idx]);
				}
				if (ItemIndex != nullptr)
				{
					*ItemIndex = Idx;
				}
				return true;
			}
		}
		return false;
	}

	// Iterators
	typedef TIndexedContainerIterator<      TArray,       ElementType, SizeType> TIterator;
	typedef TIndexedContainerIterator<const TArray, const ElementType, SizeType> TConstIterator;

	/**
	 * Creates an iterator for the contents of this array
	 *
	 * @returns The iterator.
	 */
	TIterator CreateIterator()
	{
		return TIterator(*this);
	}

	/**
	 * Creates a const iterator for the contents of this array
	 *
	 * @returns The const iterator.
	 */
	TConstIterator CreateConstIterator() const
	{
		return TConstIterator(*this);
	}

	#if TARRAY_RANGED_FOR_CHECKS
		typedef TCheckedPointerIterator<      ElementType, SizeType> RangedForIteratorType;
		typedef TCheckedPointerIterator<const ElementType, SizeType> RangedForConstIteratorType;
	#else
		typedef       ElementType* RangedForIteratorType;
		typedef const ElementType* RangedForConstIteratorType;
	#endif

public:

	/**
	 * DO NOT USE DIRECTLY
	 * STL-like iterators to enable range-based for loop support.
	 */
	#if TARRAY_RANGED_FOR_CHECKS
		FORCEINLINE RangedForIteratorType      begin()       { return RangedForIteratorType     (ArrayNum, GetData()); }
		FORCEINLINE RangedForConstIteratorType begin() const { return RangedForConstIteratorType(ArrayNum, GetData()); }
		FORCEINLINE RangedForIteratorType      end  ()       { return RangedForIteratorType     (ArrayNum, GetData() + Num()); }
		FORCEINLINE RangedForConstIteratorType end  () const { return RangedForConstIteratorType(ArrayNum, GetData() + Num()); }
	#else
		FORCEINLINE RangedForIteratorType      begin()       { return GetData(); }
		FORCEINLINE RangedForConstIteratorType begin() const { return GetData(); }
		FORCEINLINE RangedForIteratorType      end()         { return GetData() + Num(); }
		FORCEINLINE RangedForConstIteratorType end() const   { return GetData() + Num(); }
	#endif

public:

	/**
	 * Sorts the array assuming < operator is defined for the item type.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during sorting.
	 *        Therefore, your array will be sorted by the values being pointed to, rather than the pointers' values.
	 *        If this is not desirable, please use Algo::Sort(MyArray) instead.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	void Sort()
	{
		::Sort(GetData(), Num());
	}

	/**
	 * Sorts the array using user define predicate class.
	 *
	 * @param Predicate Predicate class instance.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during sorting.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        If this is not desirable, please use Algo::Sort(MyArray, Predicate) instead.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	void Sort(const PREDICATE_CLASS& Predicate)
	{
		::Sort(GetData(), Num(), Predicate);
	}

	/**
	 * Stable sorts the array assuming < operator is defined for the item type.
	 *
	 * Stable sort is slower than non-stable algorithm.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during sorting.
	 *        Therefore, your array will be sorted by the values being pointed to, rather than the pointers' values.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	void StableSort()
	{
		::StableSort(GetData(), Num());
	}

	/**
	 * Stable sorts the array using user defined predicate class.
	 *
	 * Stable sort is slower than non-stable algorithm.
	 *
	 * @param Predicate Predicate class instance
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during sorting.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	void StableSort(const PREDICATE_CLASS& Predicate)
	{
		::StableSort(GetData(), Num(), Predicate);
	}

#if defined(_MSC_VER) && !defined(__clang__)	// Relies on MSVC-specific lazy template instantiation to support arrays of incomplete types
private:
	/**
	 * Helper function that can be used inside the debuggers watch window to debug TArrays. E.g. "*Class->Defaults.DebugGet(5)". 
	 *
	 * @param Index Position to get.
	 * @returns Reference to the element at given position.
	 */
	FORCENOINLINE const ElementType& DebugGet(SizeType Index) const
	{
		return GetData()[Index];
	}
#endif

private:
	void AllocatorResizeAllocation(SizeType CurrentArrayNum, SizeType NewArrayMax)
	{
		if constexpr (TAllocatorTraits<AllocatorType>::SupportsElementAlignment)
		{
			AllocatorInstance.ResizeAllocation(CurrentArrayNum, NewArrayMax, sizeof(ElementType), alignof(ElementType));
		}
		else
		{
			AllocatorInstance.ResizeAllocation(CurrentArrayNum, NewArrayMax, sizeof(ElementType));
		}
	}

	SizeType AllocatorCalculateSlackShrink(SizeType CurrentArrayNum, SizeType NewArrayMax)
	{
		if constexpr (TAllocatorTraits<AllocatorType>::SupportsElementAlignment)
		{
			return AllocatorInstance.CalculateSlackShrink(CurrentArrayNum, NewArrayMax, sizeof(ElementType), alignof(ElementType));
		}
		else
		{
			return AllocatorInstance.CalculateSlackShrink(CurrentArrayNum, NewArrayMax, sizeof(ElementType));
		}
	}

	SizeType AllocatorCalculateSlackGrow(SizeType CurrentArrayNum, SizeType NewArrayMax)
	{
		if constexpr (TAllocatorTraits<AllocatorType>::SupportsElementAlignment)
		{
			return AllocatorInstance.CalculateSlackGrow(CurrentArrayNum, NewArrayMax, sizeof(ElementType), alignof(ElementType));
		}
		else
		{
			return AllocatorInstance.CalculateSlackGrow(CurrentArrayNum, NewArrayMax, sizeof(ElementType));
		}
	}

	SizeType AllocatorCalculateSlackReserve(SizeType NewArrayMax)
	{
		if constexpr (TAllocatorTraits<AllocatorType>::SupportsElementAlignment)
		{
			return AllocatorInstance.CalculateSlackReserve(NewArrayMax, sizeof(ElementType), alignof(ElementType));
		}
		else
		{
			return AllocatorInstance.CalculateSlackReserve(NewArrayMax, sizeof(ElementType));
		}
	}

	FORCENOINLINE void ResizeGrow(SizeType OldNum)
	{
		ArrayMax = AllocatorCalculateSlackGrow(ArrayNum, ArrayMax);
		AllocatorResizeAllocation(OldNum, ArrayMax);
	}
	FORCENOINLINE void ResizeShrink()
	{
		const SizeType NewArrayMax = AllocatorCalculateSlackShrink(ArrayNum, ArrayMax);
		if (NewArrayMax != ArrayMax)
		{
			ArrayMax = NewArrayMax;
			check(ArrayMax >= ArrayNum);
			AllocatorResizeAllocation(ArrayNum, ArrayMax);
		}
	}
	FORCENOINLINE void ResizeTo(SizeType NewMax)
	{
		if (NewMax)
		{
			NewMax = AllocatorCalculateSlackReserve(NewMax);
		}
		if (NewMax != ArrayMax)
		{
			ArrayMax = NewMax;
			AllocatorResizeAllocation(ArrayNum, ArrayMax);
		}
	}
	FORCENOINLINE void ResizeForCopy(SizeType NewMax, SizeType PrevMax)
	{
		if (NewMax)
		{
			NewMax = AllocatorCalculateSlackReserve(NewMax);
		}
		if (NewMax > PrevMax)
		{
			AllocatorResizeAllocation(0, NewMax);
			ArrayMax = NewMax;
		}
		else
		{
			ArrayMax = PrevMax;
		}
	}


	/**
	 * Copies data from one array into this array. Uses the fast path if the
	 * data in question does not need a constructor.
	 *
	 * @param Source The source array to copy
	 * @param PrevMax The previous allocated size
	 * @param ExtraSlack Additional amount of memory to allocate at
	 *                   the end of the buffer. Counted in elements. Zero by
	 *                   default.
	 */
	template <typename OtherElementType, typename OtherSizeType>
	void CopyToEmpty(const OtherElementType* OtherData, OtherSizeType OtherNum, SizeType PrevMax, SizeType ExtraSlack)
	{
		SizeType NewNum = (SizeType)OtherNum;
		checkf((OtherSizeType)NewNum == OtherNum, TEXT("Invalid number of elements to add to this array type: %llu"), (unsigned long long)NewNum);

		checkSlow(ExtraSlack >= 0);
		ArrayNum = NewNum;
		if (OtherNum || ExtraSlack || PrevMax)
		{
			ResizeForCopy(NewNum + ExtraSlack, PrevMax);
			ConstructItems<ElementType>(GetData(), OtherData, OtherNum);
		}
		else
		{
			ArrayMax = AllocatorInstance.GetInitialCapacity();
		}
	}

protected:

	template<typename ElementType, typename AllocatorType>
	friend class TIndirectArray;

	ElementAllocatorType AllocatorInstance;
	SizeType             ArrayNum;
	SizeType             ArrayMax;

private:
	template<bool bFreezeMemoryImage, typename Dummy=void>
	struct TSupportsFreezeMemoryImageHelper
	{
		static void WriteMemoryImage(FMemoryImageWriter& Writer, const TArray&)
		{
			// Writing non-freezable TArray is only supported for 64-bit target for now
			// Would need complete layout macros for all allocator types in order to properly write (empty) 32bit versions
			check(Writer.Is64BitTarget());
			Writer.WriteBytes(TArray());
		}

		static void CopyUnfrozen(const FMemoryUnfreezeContent& Context, const TArray&, void* Dst) { new(Dst) TArray(); }
		static void AppendHash(const FPlatformTypeLayoutParameters& LayoutParams, FSHA1& Hasher) {}
		static void ToString(const FPlatformTypeLayoutParameters& LayoutParams, FMemoryToStringContext& OutContext, const TArray& Object) {}
	};

	template<typename Dummy>
	struct TSupportsFreezeMemoryImageHelper<true, Dummy>
	{
		static void WriteMemoryImage(FMemoryImageWriter& Writer, const TArray& Object)
		{
			Object.AllocatorInstance.WriteMemoryImage(Writer, StaticGetTypeLayoutDesc<ElementType>(), Object.ArrayNum);
			Writer.WriteBytes(Object.ArrayNum);
			Writer.WriteBytes(Object.ArrayNum);
		}
		static void CopyUnfrozen(const FMemoryUnfreezeContent& Context, const TArray& Object, void* Dst)
		{
			TArray* DstArray = new(Dst) TArray();
			DstArray->SetNumZeroed(Object.ArrayNum);
			Object.AllocatorInstance.CopyUnfrozen(Context, StaticGetTypeLayoutDesc<ElementType>(), Object.ArrayNum, DstArray->GetData());
		}
		static void AppendHash(const FPlatformTypeLayoutParameters& LayoutParams, FSHA1& Hasher)
		{
			Freeze::AppendHash(StaticGetTypeLayoutDesc<ElementType>(), LayoutParams, Hasher);
		}
		static void ToString(const FPlatformTypeLayoutParameters& LayoutParams, FMemoryToStringContext& OutContext, const TArray& Object)
		{
			Object.AllocatorInstance.ToString(StaticGetTypeLayoutDesc<ElementType>(), Object.ArrayNum, Object.ArrayMax, LayoutParams, OutContext);
		}
	};

public:
	void WriteMemoryImage(FMemoryImageWriter& Writer) const
	{
		static constexpr bool bSupportsFreezeMemoryImage = TAllocatorTraits<AllocatorType>::SupportsFreezeMemoryImage && THasTypeLayout<ElementType>::Value;
		TSupportsFreezeMemoryImageHelper<bSupportsFreezeMemoryImage>::WriteMemoryImage(Writer, *this);
	}

	void CopyUnfrozen(const FMemoryUnfreezeContent& Context, void* Dst) const
	{
		static constexpr bool bSupportsFreezeMemoryImage = TAllocatorTraits<AllocatorType>::SupportsFreezeMemoryImage && THasTypeLayout<ElementType>::Value;
		TSupportsFreezeMemoryImageHelper<bSupportsFreezeMemoryImage>::CopyUnfrozen(Context, *this, Dst);
	}

	static void AppendHash(const FPlatformTypeLayoutParameters& LayoutParams, FSHA1& Hasher)
	{
		static constexpr bool bSupportsFreezeMemoryImage = TAllocatorTraits<AllocatorType>::SupportsFreezeMemoryImage && THasTypeLayout<ElementType>::Value;
		TSupportsFreezeMemoryImageHelper<bSupportsFreezeMemoryImage>::AppendHash(LayoutParams, Hasher);
	}

	void ToString(const FPlatformTypeLayoutParameters& LayoutParams, FMemoryToStringContext& OutContext) const
	{
		static constexpr bool bSupportsFreezeMemoryImage = TAllocatorTraits<AllocatorType>::SupportsFreezeMemoryImage && THasTypeLayout<ElementType>::Value;
		TSupportsFreezeMemoryImageHelper<bSupportsFreezeMemoryImage>::ToString(LayoutParams, OutContext, *this);
	}

	/**
	* Implicit heaps
	*/
public:
	/** 
	 * Builds an implicit heap from the array.
	 *
	 * @param Predicate Predicate class instance.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	FORCEINLINE void Heapify(const PREDICATE_CLASS& Predicate)
	{
		TDereferenceWrapper<ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		Algo::Heapify(*this, PredicateWrapper);
	}

	/**
	 * Builds an implicit heap from the array. Assumes < operator is defined
	 * for the template type.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your array will be heapified by the values being pointed to, rather than the pointers' values.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	void Heapify()
	{
		Heapify(TLess<ElementType>());
	}

	/** 
	 * Adds a new element to the heap.
	 *
	 * @param InItem Item to be added.
	 * @param Predicate Predicate class instance.
	 * @return The index of the new element.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	SizeType HeapPush(ElementType&& InItem, const PREDICATE_CLASS& Predicate)
	{
		// Add at the end, then sift up
		Add(MoveTempIfPossible(InItem));
		TDereferenceWrapper<ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		SizeType Result = AlgoImpl::HeapSiftUp(GetData(), 0, Num() - 1, FIdentityFunctor(), PredicateWrapper);

		return Result;
	}

	/** 
	 * Adds a new element to the heap.
	 *
	 * @param InItem Item to be added.
	 * @param Predicate Predicate class instance.
	 * @return The index of the new element.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	SizeType HeapPush(const ElementType& InItem, const PREDICATE_CLASS& Predicate)
	{
		// Add at the end, then sift up
		Add(InItem);
		TDereferenceWrapper<ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		SizeType Result = AlgoImpl::HeapSiftUp(GetData(), 0, Num() - 1, FIdentityFunctor(), PredicateWrapper);

		return Result;
	}

	/** 
	 * Adds a new element to the heap. Assumes < operator is defined for the
	 * template type.
	 *
	 * @param InItem Item to be added.
	 * @return The index of the new element.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your array will be heapified by the values being pointed to, rather than the pointers' values.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	SizeType HeapPush(ElementType&& InItem)
	{
		return HeapPush(MoveTempIfPossible(InItem), TLess<ElementType>());
	}

	/** 
	 * Adds a new element to the heap. Assumes < operator is defined for the
	 * template type.
	 *
	 * @param InItem Item to be added.
	 * @return The index of the new element.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your array will be heapified by the values being pointed to, rather than the pointers' values.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	SizeType HeapPush(const ElementType& InItem)
	{
		return HeapPush(InItem, TLess<ElementType>());
	}

	/** 
	 * Removes the top element from the heap.
	 *
	 * @param OutItem The removed item.
	 * @param Predicate Predicate class instance.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	void HeapPop(ElementType& OutItem, const PREDICATE_CLASS& Predicate, bool bAllowShrinking = true)
	{
		OutItem = MoveTemp((*this)[0]);
		RemoveAtSwap(0, 1, bAllowShrinking);

		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		AlgoImpl::HeapSiftDown(GetData(), 0, Num(), FIdentityFunctor(), PredicateWrapper);
	}

	/** 
	 * Removes the top element from the heap. Assumes < operator is defined for
	 * the template type.
	 *
	 * @param OutItem The removed item.
	 * @param bAllowShrinking (Optional) Tells if this call can shrink the array allocation if suitable after the pop. Default is true.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your array will be heapified by the values being pointed to, rather than the pointers' values.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	void HeapPop(ElementType& OutItem, bool bAllowShrinking = true)
	{
		HeapPop(OutItem, TLess<ElementType>(), bAllowShrinking);
	}

	/**
	 * Verifies the heap.
	 *
	 * @param Predicate Predicate class instance.
	 */
	template <class PREDICATE_CLASS>
	void VerifyHeap(const PREDICATE_CLASS& Predicate)
	{
		check(Algo::IsHeap(*this, Predicate));
	}

	/** 
	 * Removes the top element from the heap.
	 *
	 * @param Predicate Predicate class instance.
	 * @param bAllowShrinking (Optional) Tells if this call can shrink the array allocation if suitable after the discard. Default is true.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	void HeapPopDiscard(const PREDICATE_CLASS& Predicate, bool bAllowShrinking = true)
	{
		RemoveAtSwap(0, 1, bAllowShrinking);
		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		AlgoImpl::HeapSiftDown(GetData(), 0, Num(), FIdentityFunctor(), PredicateWrapper);
	}

	/** 
	 * Removes the top element from the heap. Assumes < operator is defined for the template type.
	 *
	 * @param bAllowShrinking (Optional) Tells if this call can shrink the array
	 *		allocation if suitable after the discard. Default is true.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your array will be heapified by the values being pointed to, rather than the pointers' values.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	void HeapPopDiscard(bool bAllowShrinking = true)
	{
		HeapPopDiscard(TLess<ElementType>(), bAllowShrinking);
	}

	/** 
	 * Returns the top element from the heap (does not remove the element).
	 *
	 * Const version.
	 *
	 * @returns The reference to the top element from the heap.
	 */
	const ElementType& HeapTop() const
	{
		return (*this)[0];
	}

	/** 
	 * Returns the top element from the heap (does not remove the element).
	 *
	 * @returns The reference to the top element from the heap.
	 */
	ElementType& HeapTop()
	{
		return (*this)[0];
	}

	/**
	 * Removes an element from the heap.
	 *
	 * @param Index Position at which to remove item.
	 * @param Predicate Predicate class instance.
	 * @param bAllowShrinking (Optional) Tells if this call can shrink the array allocation
	 *		if suitable after the remove (default = true).
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	void HeapRemoveAt(SizeType Index, const PREDICATE_CLASS& Predicate, bool bAllowShrinking = true)
	{
		RemoveAtSwap(Index, 1, bAllowShrinking);

		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		AlgoImpl::HeapSiftDown(GetData(), Index, Num(), FIdentityFunctor(), PredicateWrapper);
		AlgoImpl::HeapSiftUp(GetData(), 0, FPlatformMath::Min(Index, Num() - 1), FIdentityFunctor(), PredicateWrapper);
	}

	/**
	 * Removes an element from the heap. Assumes < operator is defined for the template type.
	 *
	 * @param Index Position at which to remove item.
	 * @param bAllowShrinking (Optional) Tells if this call can shrink the array allocation
	 *		if suitable after the remove (default = true).
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your array will be heapified by the values being pointed to, rather than the pointers' values.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	void HeapRemoveAt(SizeType Index, bool bAllowShrinking = true)
	{
		HeapRemoveAt(Index, TLess< ElementType >(), bAllowShrinking);
	}

	/**
	 * Performs heap sort on the array.
	 *
	 * @param Predicate Predicate class instance.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your predicate will be passed references rather than pointers.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	template <class PREDICATE_CLASS>
	void HeapSort(const PREDICATE_CLASS& Predicate)
	{
		TDereferenceWrapper<ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		Algo::HeapSort(*this, PredicateWrapper);
	}

	/**
	 * Performs heap sort on the array. Assumes < operator is defined for the
	 * template type.
	 *
	 * @note: If your array contains raw pointers, they will be automatically dereferenced during heapification.
	 *        Therefore, your array will be heapified by the values being pointed to, rather than the pointers' values.
	 *        The auto-dereferencing behavior does not occur with smart pointers.
	 */
	void HeapSort()
	{
		HeapSort(TLess<ElementType>());
	}

	const ElementAllocatorType& GetAllocatorInstance() const { return AllocatorInstance; }
	ElementAllocatorType& GetAllocatorInstance() { return AllocatorInstance; }
};


namespace Freeze
{
	template<typename T, typename AllocatorType>
	void IntrinsicWriteMemoryImage(FMemoryImageWriter& Writer, const TArray<T, AllocatorType>& Object, const FTypeLayoutDesc&)
	{
		Object.WriteMemoryImage(Writer);
	}

	template<typename T, typename AllocatorType>
	uint32 IntrinsicUnfrozenCopy(const FMemoryUnfreezeContent& Context, const TArray<T, AllocatorType>& Object, void* OutDst)
	{
		Object.CopyUnfrozen(Context, OutDst);
		return sizeof(Object);
	}

	template<typename T, typename AllocatorType>
	uint32 IntrinsicAppendHash(const TArray<T, AllocatorType>* DummyObject, const FTypeLayoutDesc& TypeDesc, const FPlatformTypeLayoutParameters& LayoutParams, FSHA1& Hasher)
	{
		return AppendHashForNameAndSize(TypeDesc.Name, sizeof(TArray<T, AllocatorType>), Hasher);
	}

	template<typename T, typename AllocatorType>
	uint32 IntrinsicGetTargetAlignment(const TArray<T, AllocatorType>* DummyObject, const FTypeLayoutDesc& TypeDesc, const FPlatformTypeLayoutParameters& LayoutParams)
	{
		// Assume alignment of array is drive by pointer
		return FMath::Min(8u, LayoutParams.MaxFieldAlignment);
	}

	template<typename T, typename AllocatorType>
	void IntrinsicToString(const TArray<T, AllocatorType>& Object, const FTypeLayoutDesc& TypeDesc, const FPlatformTypeLayoutParameters& LayoutParams, FMemoryToStringContext& OutContext)
	{
		Object.ToString(LayoutParams, OutContext);
	}
}

DECLARE_TEMPLATE_INTRINSIC_TYPE_LAYOUT((template <typename T, typename AllocatorType>), (TArray<T, AllocatorType>));

template <typename InElementType, typename AllocatorType>
struct TIsZeroConstructType<TArray<InElementType, AllocatorType>>
{
	enum { Value = TAllocatorTraits<AllocatorType>::IsZeroConstruct };
};

template <typename InElementType, typename AllocatorType>
struct TContainerTraits<TArray<InElementType, AllocatorType> > : public TContainerTraitsBase<TArray<InElementType, AllocatorType> >
{
	static_assert(TAllocatorTraits<AllocatorType>::SupportsMove, "TArray no longer supports move-unaware allocators");
	enum { MoveWillEmptyContainer = TAllocatorTraits<AllocatorType>::SupportsMove };
};

template <typename T, typename AllocatorType>
struct TIsContiguousContainer<TArray<T, AllocatorType>>
{
	enum { Value = true };
};

/**
 * Traits class which determines whether or not a type is a TArray.
 */
template <typename T> struct TIsTArray { enum { Value = false }; };

template <typename InElementType, typename InAllocatorType> struct TIsTArray<               TArray<InElementType, InAllocatorType>> { enum { Value = true }; };
template <typename InElementType, typename InAllocatorType> struct TIsTArray<const          TArray<InElementType, InAllocatorType>> { enum { Value = true }; };
template <typename InElementType, typename InAllocatorType> struct TIsTArray<      volatile TArray<InElementType, InAllocatorType>> { enum { Value = true }; };
template <typename InElementType, typename InAllocatorType> struct TIsTArray<const volatile TArray<InElementType, InAllocatorType>> { enum { Value = true }; };


//
// Array operator news.
//
template <typename T,typename AllocatorType> void* operator new( size_t Size, TArray<T,AllocatorType>& Array )
{
	check(Size == sizeof(T));
	const auto Index = Array.AddUninitialized(1);
	return &Array[Index];
}
template <typename T,typename AllocatorType> void* operator new( size_t Size, TArray<T,AllocatorType>& Array, typename TArray<T, AllocatorType>::SizeType Index )
{
	check(Size == sizeof(T));
	Array.InsertUninitialized(Index);
	return &Array[Index];
}
