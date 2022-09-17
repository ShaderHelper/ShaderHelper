// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Serialization/Archive.h"
#include "Serialization/MemoryImageWriter.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/EnumClassFlags.h"

template<typename Allocator > class TBitArray;

// Functions for manipulating bit sets.
struct FBitSet
{
	/** Clears the next set bit in the mask and returns its index. */
	static FORCEINLINE uint32 GetAndClearNextBit(uint32& Mask)
	{
		const uint32 LowestBitMask = (Mask) & (-(int32)Mask);
		const uint32 BitIndex = FMath::FloorLog2(LowestBitMask);
		Mask ^= LowestBitMask;
		return BitIndex;
	}

	// Clang generates 7 instructions for int32 DivideAndRoundUp but only 2 for uint32
	static constexpr uint32 BitsPerWord = NumBitsPerDWORD;

	FORCEINLINE static uint32 CalculateNumWords(int32 NumBits)
	{
		checkSlow(NumBits >= 0);
		return FMath::DivideAndRoundUp(static_cast<uint32>(NumBits), BitsPerWord);
	}
};


// Forward declaration.
template<typename Allocator = FDefaultBitArrayAllocator>
class TBitArray;

template<typename Allocator = FDefaultBitArrayAllocator>
class TConstSetBitIterator;

template<typename Allocator = FDefaultBitArrayAllocator,typename OtherAllocator = FDefaultBitArrayAllocator, bool Both=true>
class TConstDualSetBitIterator;

template <typename AllocatorType, typename InDerivedType = void>
class TScriptBitArray;

/** Flag enumeration for control bitwise operator functionality */
enum class EBitwiseOperatorFlags
{
	/** Specifies that the result should be sized Max(A.Num(), B.Num()) */
	MaxSize = 1 << 0,
	/** Specifies that the result should be sized Min(A.Num(), B.Num()) */
	MinSize = 1 << 1,
	/** Only valid for self-mutating bitwise operators - indicates that the size of the LHS operand should not be changed */
	MaintainSize = 1 << 2,

	/** When MaxSize or MaintainSize is specified and the operands are sized differently, any missing bits in the resulting bit array will be considered as 1, rather than 0 */
	OneFillMissingBits = 1 << 4,
};
ENUM_CLASS_FLAGS(EBitwiseOperatorFlags)

/**
 * Serializer (predefined for no friend injection in gcc 411)
 */
template<typename Allocator>
FArchive& operator<<(FArchive& Ar, TBitArray<Allocator>& BitArray);

/** Used to read/write a bit in the array as a bool. */
class FBitReference
{
public:

	FORCEINLINE FBitReference(uint32& InData,uint32 InMask)
	:	Data(InData)
	,	Mask(InMask)
	{}

	FORCEINLINE operator bool() const
	{
		 return (Data & Mask) != 0;
	}
	FORCEINLINE void operator=(const bool NewValue)
	{
		if(NewValue)
		{
			Data |= Mask;
		}
		else
		{
			Data &= ~Mask;
		}
	}
	FORCEINLINE void operator|=(const bool NewValue)
	{
		if (NewValue)
		{
			Data |= Mask;
		}
	}
	FORCEINLINE void operator&=(const bool NewValue)
	{
		if (!NewValue)
		{
			Data &= ~Mask;
		}
	}
	FORCEINLINE void AtomicSet(const bool NewValue)
	{
		if(NewValue)
		{
			if (!(Data & Mask))
			{
				while (1)
				{
					uint32 Current = Data;
					uint32 Desired = Current | Mask;
					if (Current == Desired || FPlatformAtomics::InterlockedCompareExchange((volatile int32*)&Data, (int32)Desired, (int32)Current) == (int32)Current)
					{
						return;
					}
				}
			}
		}
		else
		{
			if (Data & Mask)
			{
				while (1)
				{
					uint32 Current = Data;
					uint32 Desired = Current & ~Mask;
					if (Current == Desired || FPlatformAtomics::InterlockedCompareExchange((volatile int32*)&Data, (int32)Desired, (int32)Current) == (int32)Current)
					{
						return;
					}
				}
			}
		}
	}
	FORCEINLINE FBitReference& operator=(const FBitReference& Copy)
	{
		// As this is emulating a reference, assignment should not rebind,
		// it should write to the referenced bit.
		*this = (bool)Copy;
		return *this;
	}

private:
	uint32& Data;
	uint32 Mask;
};


/** Used to read a bit in the array as a bool. */
class FConstBitReference
{
public:

	FORCEINLINE FConstBitReference(const uint32& InData,uint32 InMask)
	:	Data(InData)
	,	Mask(InMask)
	{}

	FORCEINLINE operator bool() const
	{
		 return (Data & Mask) != 0;
	}

private:
	const uint32& Data;
	uint32 Mask;
};


/** Used to reference a bit in an unspecified bit array. */
class FRelativeBitReference
{
public:
	FORCEINLINE explicit FRelativeBitReference(int32 BitIndex)
		: WordIndex(BitIndex >> NumBitsPerDWORDLogTwo)
		, Mask(1 << (BitIndex & (NumBitsPerDWORD - 1)))
	{
	}

	int32  WordIndex;
	uint32 Mask;
};

class FBitArrayMemory
{
public:
	/**
	 * Copy NumBits bits from the source pointer and offset into the dest pointer and offset.
	 * This function is not suitable for general use because it uses a bit order that is specific to the uint32 internal storage of BitArray
	 *
	 * Bits within each word are read or written in the current platform's mathematical bitorder (Data[0] & 0x1, Data[0] & 0x2, ... Data[0] & 0x100, ... Data[0] & 0x80000000, Data[1] & 0x1 ...
	 * Correctly handles overlap between destination range and source range; the array of destination bits will be a copy of the source bits as they were before the copy started.
	 * @param DestBits The base location to which the bits are written.
	 * @param DestOffset The (word-order) bit within DestBits at which to start writing. Can be any value; offsets outside of [0,NumBitsPerDWORD) will be equivalent to modifying the DestBits pointer.
	 * @param SourceBits The base location from which the bits are read.
	 * @param SourceOffset The (word-order) bit within SourceBits at which to start reading. Can be any value; offsets outside of [0,NumBitsPerDWORD) will be equivalent to modifying the SourceBits pointer.
	 * @param NumBits Number of bits to copy. Must be >= 0.
	 */
	static CORE_API void MemmoveBitsWordOrder(uint32* DestBits, int32 DestOffset, const uint32* SourceBits, int32 SourceOffset, uint32 NumBits);
	static inline void MemmoveBitsWordOrder(int32* DestBits, int32 DestOffset, const int32* SourceBits, int32 SourceOffset, uint32 NumBits)
	{
		MemmoveBitsWordOrder(reinterpret_cast<uint32*>(DestBits), DestOffset, reinterpret_cast<const uint32*>(SourceBits), SourceOffset, NumBits);
	}

	/** Given Data and Offset that specify a specific bit in a specific word, modify Data and Offset so that they specify the same bit but that 0 <= Offset < NumBitsPerDWORD. */
	inline static void ModularizeWordOffset(uint32*& Data, int32& Offset)
	{
		ModularizeWordOffset(const_cast<uint32 const*&>(Data), Offset);
	}

	/** Given Data and Offset that specify a specific bit in a specific word, modify Data and Offset so that they specify the same bit but that 0 <= Offset < NumBitsPerDWORD. */
	static void ModularizeWordOffset(uint32 const*& Data, int32& Offset);

private:

	/**
	 * Copy NumBits bits from the source pointer at the given offset into the dest pointer at the given offset.
	 * It has the same behavior as MemmoveBitsWordOrder under the constaint that DestOffset == SourceOffset.
	 */
	static void MemmoveBitsWordOrderAlignedInternal(uint32*const StartDest, const uint32*const StartSource, int32 StartOffset, uint32 NumBits);

	friend class FBitArrayMemoryTest;
};

/**
 * A dynamically sized bit array.
 * An array of Booleans.  They stored in one bit/Boolean.  There are iterators that efficiently iterate over only set bits.
 */
template<typename Allocator /*= FDefaultBitArrayAllocator*/>
class TBitArray
{
	template <typename, typename>
	friend class TScriptBitArray;

	typedef uint32 WordType;
	static constexpr WordType FullWordMask = (WordType)-1;

public:

	typedef typename Allocator::template ForElementType<uint32> AllocatorType;

	template<typename>
	friend class TConstSetBitIterator;

	template<typename,typename,bool>
	friend class TConstDualSetBitIterator;

	TBitArray()
	:	NumBits(0)
	,	MaxBits(AllocatorInstance.GetInitialCapacity() * NumBitsPerDWORD)
	{
		// ClearPartialSlackBits is already satisfied since final word does not exist when NumBits == 0
	}

	/**
	 * Minimal initialization constructor.
	 * @param Value - The value to initial the bits to.
	 * @param InNumBits - The initial number of bits in the array.
	 */
	FORCEINLINE explicit TBitArray(bool bValue, int32 InNumBits)
	:	MaxBits(AllocatorInstance.GetInitialCapacity() * NumBitsPerDWORD)
	{
		Init(bValue, InNumBits);
	}

	/**
	 * Move constructor.
	 */
	FORCEINLINE TBitArray(TBitArray&& Other)
	{
		MoveOrCopy(*this, Other);
	}

	/**
	 * Copy constructor.
	 */
	FORCEINLINE TBitArray(const TBitArray& Copy)
	:	NumBits(0)
	,	MaxBits(0)
	{
		Assign<Allocator>(Copy);
	}

	template<typename OtherAllocator>
	FORCEINLINE TBitArray(const TBitArray<OtherAllocator> & Copy)
		: NumBits(0)
		, MaxBits(0)
	{
		Assign<OtherAllocator>(Copy);
	}

	/**
	 * Move assignment.
	 */
	FORCEINLINE TBitArray& operator=(TBitArray&& Other)
	{
		if (this != &Other)
		{
			MoveOrCopy(*this, Other);
		}

		return *this;
	}

	/**
	 * Assignment operator.
	 */
	FORCEINLINE TBitArray& operator=(const TBitArray& Copy)
	{
		// check for self assignment since we don't use swap() mechanic
		if (this != &Copy)
		{
			Assign<Allocator>(Copy);
		}
		return *this;
	}


	template<typename OtherAllocator>
	FORCEINLINE TBitArray& operator=(const TBitArray< OtherAllocator >& Copy)
	{
		// No need for a self-assignment check. If we were the same, we'd be using
		// the default assignment operator.
		Assign<OtherAllocator>(Copy);
		return *this;
	}

	FORCEINLINE bool operator==(const TBitArray<Allocator>& Other) const
	{
		if (Num() != Other.Num())
		{
			return false;
		}

		return FMemory::Memcmp(GetData(), Other.GetData(), GetNumWords() * sizeof(uint32)) == 0;
	}

	FORCEINLINE bool operator<(const TBitArray<Allocator>& Other) const
	{
		//sort by length
		if (Num() != Other.Num())
		{
			return Num() < Other.Num();
		}

		uint32 NumWords = GetNumWords();
		const uint32* Data0 = GetData();
		const uint32* Data1 = Other.GetData();

		//lexicographically compare
		for (uint32 i = 0; i < NumWords; i++)
		{
			if (Data0[i] != Data1[i])
			{
				return Data0[i] < Data1[i];
			}
		}
		return false;
	}

	FORCEINLINE bool operator!=(const TBitArray<Allocator>& Other) const
	{
		return !(*this == Other);
	}

private:
	FORCEINLINE uint32 GetNumWords() const
	{
		return FBitSet::CalculateNumWords(NumBits);
	}

	FORCEINLINE uint32 GetMaxWords() const
	{
		return FBitSet::CalculateNumWords(MaxBits);
	}

	FORCEINLINE uint32 GetLastWordMask() const
	{
		const uint32 UnusedBits = (FBitSet::BitsPerWord - static_cast<uint32>(NumBits) % FBitSet::BitsPerWord) % FBitSet::BitsPerWord;
		return ~0u >> UnusedBits;
	}

	FORCEINLINE static void SetWords(uint32* Words, int32 NumWords, bool bValue)
	{
		if (NumWords > 8)
		{
			FMemory::Memset(Words, bValue ? 0xff : 0, NumWords * sizeof(uint32));
		}
		else
		{
			uint32 Word = bValue ? ~0u : 0u;
			for (int32 Idx = 0; Idx < NumWords; ++Idx)
			{
				Words[Idx] = Word;
			}
		}
	}

	template <typename BitArrayType>
	static FORCEINLINE typename TEnableIf<TContainerTraits<BitArrayType>::MoveWillEmptyContainer>::Type MoveOrCopy(BitArrayType& ToArray, BitArrayType& FromArray)
	{
		ToArray.AllocatorInstance.MoveToEmpty(FromArray.AllocatorInstance);

		ToArray  .NumBits = FromArray.NumBits;
		ToArray  .MaxBits = FromArray.MaxBits;
		FromArray.NumBits = 0;
		FromArray.MaxBits = 0;
		// No need to call this.ClearPartialSlackBits, because the words we're copying or moving from satisfy the invariant
		// No need to call FromArray.ClearPartialSlackBits because NumBits == 0 automatically satisfies the invariant
	}

	template <typename BitArrayType>
	static FORCEINLINE typename TEnableIf<!TContainerTraits<BitArrayType>::MoveWillEmptyContainer>::Type MoveOrCopy(BitArrayType& ToArray, BitArrayType& FromArray)
	{
		ToArray = FromArray;
	}

	template<typename OtherAllocator>
	void Assign(const TBitArray<OtherAllocator>& Other)
	{
		Empty(Other.Num());
		NumBits = Other.Num();
		if (NumBits)
		{
			FMemory::Memcpy(GetData(), Other.GetData(), GetNumWords() * sizeof(uint32));
			// No need to call ClearPartialSlackBits, because the bits we're copying from satisfy the invariant
			// If NumBits == 0, the invariant is automatically satisfied so we also don't need ClearPartialSlackBits in that case.
		}
	}


public:

	/**
	 * Checks the invariants of this class
	 */
	void CheckInvariants() const
	{
#if DO_CHECK
		checkf(NumBits <= MaxBits, TEXT("TBitArray::NumBits (%d) should never be greater than MaxBits (%d)"), NumBits, MaxBits);
		checkf(NumBits >= 0 && MaxBits >= 0, TEXT("NumBits (%d) and MaxBits (%d) should always be >= 0"), NumBits, MaxBits);

		// Verify the ClearPartialSlackBits invariant
		const int32 UsedBits = (NumBits % NumBitsPerDWORD);
		if (UsedBits != 0)
		{
			const int32 LastWordIndex = NumBits / NumBitsPerDWORD;
			const uint32 SlackMask = FullWordMask << UsedBits;

			const uint32 LastWord = *(GetData() + LastWordIndex);
			checkf((LastWord & SlackMask) == 0, TEXT("TBitArray slack bits are non-zero, this will result in undefined behavior."));
		}
#endif
	}

	/**
	 * Serializer
	 */
	friend FArchive& operator<<(FArchive& Ar, TBitArray& BitArray)
	{
		// serialize number of bits
		Ar << BitArray.NumBits;

		if (Ar.IsLoading())
		{
			// no need for slop when reading; set MaxBits to the smallest legal value that is >= NumBits
			BitArray.MaxBits = NumBitsPerDWORD * FMath::Max(FBitSet::CalculateNumWords(BitArray.NumBits), (uint32)BitArray.AllocatorInstance.GetInitialCapacity());

			// allocate room for new bits
			BitArray.Realloc(0);
		}

		// serialize the data as one big chunk
		Ar.Serialize(BitArray.GetData(), BitArray.GetNumWords() * sizeof(uint32));

		if (Ar.IsLoading() && !Ar.IsObjectReferenceCollector() && !Ar.IsCountingMemory())
		{
			// Clear slack bits incase they were serialized non-null
			BitArray.ClearPartialSlackBits();
		}
		return Ar;
	}

	/**
	 * Adds a bit to the array with the given value.
	 * @return The index of the added bit.
	 */
	int32 Add(const bool Value)
	{
		const int32 Index = AddUninitialized(1);
		SetBitNoCheck(Index, Value);
		return Index;
	}

	/**
	 * Adds multiple bits to the array with the given value.
	 * @return The index of the first added bit.
	 */
	int32 Add(const bool Value, int32 NumBitsToAdd)
	{
		// Support for legacy behavior requires us to silently ignore NumBitsToAdd < 0
		if (NumBitsToAdd < 0)
		{
			return NumBits;
		}
		const int32 Index = AddUninitialized(NumBitsToAdd);
		SetRange(Index, NumBitsToAdd, Value);
		return Index;
	}

	/**
	 * Adds multiple bits read from the given pointer.
	 * @param ReadBits The address of sized integers to read the bits from.
	 *                 Bits are read from ReadBits in the current platform's mathematical bitorder (ReadBits[0] & 0x1, ReadBits[0] & 0x2, ... ReadBits[0] & 0x100, ... ReadBits[0] & 0x80000000, ReadBits[1] & 0x1 ...
	 * @param NumBitsToAdd The number of bits to add. Must be >= 0.
	 * @param ReadOffsetBits Number of bits into ReadBits at which to start reading. Must be >= 0.
	 * @return The index of the first added bit.
	 */
	template <typename InWordType>
	int32 AddRange(const InWordType* ReadBits, int32 NumBitsToAdd, int32 ReadOffsetBits = 0)
	{
		const int32 Index = AddUninitialized(NumBitsToAdd);
		SetRangeFromRange(Index, NumBitsToAdd, ReadBits, ReadOffsetBits);
		return Index;
	}

	/**
	 * Adds multiple bits read from the given BitArray.
	 * @param ReadBits The value to set the bits to.
	 * @param NumBitsToAdd The number of bits to add. Must be >= 0.
	 * @param ReadOffsetBits Number of bits into ReadBits at which to start reading. Must be >= 0.
	 * @return The index of the first added bit.
	 */
	template <typename OtherAllocator>
	int32 AddRange(const TBitArray<OtherAllocator>& ReadBits, int32 NumBitsToAdd, int32 ReadOffsetBits = 0)
	{
		check(0 <= ReadOffsetBits && ReadOffsetBits + NumBitsToAdd <= ReadBits.NumBits);
		const int32 Index = AddUninitialized(NumBitsToAdd);
		SetRangeFromRange(Index, NumBitsToAdd, ReadBits, ReadOffsetBits);
		return Index;
	}

	/**
	 * Inserts space for multiple bits at the end of the array.
	 * The inserted bits are set to arbitrary values and should be written using SetRange or otherwise before being read.
	 * @param NumBitsToAdd The number of bits to add. Must be >= 0.
	 */
	int32 AddUninitialized(int32 NumBitsToAdd)
	{
		check(NumBitsToAdd >= 0);
		int32 AddedIndex = NumBits;
		if (NumBitsToAdd > 0)
		{
			int32 OldLastWordIndex = NumBits == 0 ? -1 : (NumBits - 1) / NumBitsPerDWORD;
			int32 NewLastWordIndex = (NumBits + NumBitsToAdd - 1) / NumBitsPerDWORD;
			if (NewLastWordIndex == OldLastWordIndex)
			{
				// We're not extending into a new word, so we don't need to reserve more memory and we don't need to clear the unused bits on the final word
				NumBits += NumBitsToAdd;
			}
			else
			{
				Reserve(NumBits + NumBitsToAdd);
				NumBits += NumBitsToAdd;
				ClearPartialSlackBits();
			}
		}
		return AddedIndex;
	}

	/**
	 * Adds a bit with the given value at the given index in the array.
	 * @param Value The value of the bit to add
	 * @param Index - The index at which to add; must be 0 <= Index <= Num().
	 */
	void Insert(bool Value, int32 Index)
	{
		InsertUninitialized(Index, 1);
		SetBitNoCheck(Index, Value);
	}

	/**
	 * Inserts multiple bits with the given value into the array, starting at the given Index.
	 * @param Value The value of the bits to add
	 * @param Index The index at which to add; must be 0 <= Index <= Num().
	 * @param NumBitsToAdd The number of bits to add. Must be >= 0.
	 */
	void Insert(bool Value, int32 Index, int32 NumBitsToAdd)
	{
		InsertUninitialized(Index, NumBitsToAdd);
		SetRange(Index, NumBitsToAdd, Value);
	}

	/**
	 * Inserts multiple bits read from the given pointer, starting at the given index.
	 * @param ReadBits The address of sized integers to read the bits from.
	 *                 Bits are read from ReadBits in the current platform's mathematical bitorder (ReadBits[0] & 0x1, ReadBits[0] & 0x2, ... ReadBits[0] & 0x100, ... ReadBits[0] & 0x80000000, ReadBits[1] & 0x1 ...
	 * @param Index The index at which to add; must be 0 <= Index <= Num().
	 * @param NumBitsToAdd The number of bits to add. Must be >= 0.
	 * @param ReadOffsetBits Number of bits into ReadBits at which to start reading.
	 */
	template <typename InWordType>
	void InsertRange(const InWordType* ReadBits, int32 Index, int32 NumBitsToAdd, int32 ReadOffsetBits = 0)
	{
		InsertUninitialized(Index, NumBitsToAdd);
		SetRangeFromRange(Index, NumBitsToAdd, ReadBits, ReadOffsetBits);
	}

	/**
	 * Inserts multiple bits read from the given BitArray into the array, starting at the given index.
	 * @param ReadBits The value to set the bits to.
	 * @param Index The index at which to add; must be 0 <= Index <= Num().
	 * @param NumBitsToAdd The number of bits to add. Must be >= 0.
	 * @param ReadOffsetBits Number of bits into ReadBits at which to start reading.
	 */
	template <typename OtherAllocator>
	void InsertRange(const TBitArray<OtherAllocator>& ReadBits, int32 Index, int32 NumBitsToAdd, int32 ReadOffsetBits = 0)
	{
		check(0 <= ReadOffsetBits && ReadOffsetBits + NumBitsToAdd <= ReadBits.NumBits);
		InsertUninitialized(Index, NumBitsToAdd);
		SetRangeFromRange(Index, NumBitsToAdd, ReadBits, ReadOffsetBits);
	}

	/**
	 * Inserts space for multiple bits into the array, starting at the given index.
	 * The inserted bits are set to arbitrary values and should be written using SetRange or otherwise before being read.
	 * @param Index The index at which to add; must be 0 <= Index <= Num().
	 * @param NumBitsToAdd The number of bits to add. Must be >= 0.
	 */
	void InsertUninitialized(int32 Index, int32 NumBitsToAdd)
	{
		check(0 <= Index && Index <= NumBits);
		check(NumBitsToAdd >= 0);

		if (NumBitsToAdd > 0)
		{
			uint32 OldNumBits = NumBits;
			AddUninitialized(NumBitsToAdd);
			uint32 NumToShift = OldNumBits - Index;
			if (NumToShift > 0)
			{
				// MemmoveBitsWordOrder handles overlapping source and dest
				FBitArrayMemory::MemmoveBitsWordOrder(GetData(), Index + NumBitsToAdd, GetData(), Index, NumToShift);
			}
		}
	}

	/**
	 * Removes all bits from the array, potentially leaving space allocated for an expected number of bits about to be added.
	 * @param ExpectedNumBits - The expected number of bits about to be added.
	 */
	void Empty(int32 ExpectedNumBits = 0)
	{
		ExpectedNumBits = static_cast<int32>(FBitSet::CalculateNumWords(ExpectedNumBits)) * NumBitsPerDWORD;
		const int32 InitialMaxBits = AllocatorInstance.GetInitialCapacity() * NumBitsPerDWORD;

		NumBits = 0;

		// If we need more bits or can shrink our allocation, do so
		// Otherwise, reuse current initial allocation
		if (ExpectedNumBits > MaxBits || MaxBits > InitialMaxBits)
		{
			MaxBits = FMath::Max(ExpectedNumBits, InitialMaxBits);
			Realloc(0);
		}
	}

	/**
	 * Reserves memory such that the array can contain at least Number bits.
	 *
	 * @Number  The number of bits to reserve space for.
	 */
	void Reserve(int32 Number)
	{
		if (Number > MaxBits)
		{
			const uint32 MaxWords = AllocatorInstance.CalculateSlackGrow(
				FBitSet::CalculateNumWords(Number),
				GetMaxWords(),
				sizeof(uint32)
				);
			MaxBits = MaxWords * NumBitsPerDWORD;
			Realloc(NumBits);
		}
	}

	/**
	 * Removes all bits from the array retaining any space already allocated.
	 */
	void Reset()
	{
		NumBits = 0;
	}

	/**
	 * Resets the array's contents. Use TBitArray(bool bValue, int32 InNumBits) instead of default constructor and Init().
	 *
	 * @param Value - The value to initial the bits to.
	 * @param NumBits - The number of bits in the array.
	 */
	FORCEINLINE void Init(bool bValue, int32 InNumBits)
	{
		NumBits = InNumBits;
		
		const uint32 NumWords = GetNumWords();
		const uint32 MaxWords = GetMaxWords();

		if (NumWords > 0)
		{
			if (NumWords > MaxWords)
			{
				AllocatorInstance.ResizeAllocation(0, NumWords, sizeof(uint32));
				MaxBits = NumWords * NumBitsPerDWORD;
			}

			SetWords(GetData(), NumWords, bValue);
			ClearPartialSlackBits();
		}
	}

	/** Sets number of bits without initializing new bits. */
	void SetNumUninitialized(int32 InNumBits)
	{
		int32 PreviousNumBits = NumBits;
		NumBits = InNumBits;

		if (InNumBits > MaxBits)
		{
			const int32 PreviousNumWords = FBitSet::CalculateNumWords(PreviousNumBits);
			const uint32 MaxWords = AllocatorInstance.CalculateSlackReserve(
				FBitSet::CalculateNumWords(InNumBits), sizeof(uint32));
			
			AllocatorInstance.ResizeAllocation(PreviousNumWords, MaxWords, sizeof(uint32));	

			MaxBits = MaxWords * NumBitsPerDWORD;
		}

		ClearPartialSlackBits();
	}

	/**
	 * Sets or unsets a range of bits within the array.
	 * @param Index  The index of the first bit to set; must be 0 <= Index <= Num().
	 * @param NumBitsToSet The number of bits to set, must satisify Index + NumBitsToSet <= Num().
	 * @param  Value  The value to set the bits to.
	 */
	FORCENOINLINE void SetRange(int32 Index, int32 NumBitsToSet, bool Value)
	{
		check(Index >= 0 && NumBitsToSet >= 0 && Index + NumBitsToSet <= NumBits);

		if (NumBitsToSet == 0)
		{
			return;
		}

		// Work out which uint32 index to set from, and how many
		uint32 StartIndex = Index / NumBitsPerDWORD;
		uint32 Count      = (Index + NumBitsToSet + (NumBitsPerDWORD - 1)) / NumBitsPerDWORD - StartIndex;

		// Work out masks for the start/end of the sequence
		uint32 StartMask  = FullWordMask << (Index % NumBitsPerDWORD);
		uint32 EndMask    = FullWordMask >> (NumBitsPerDWORD - (Index + NumBitsToSet) % NumBitsPerDWORD) % NumBitsPerDWORD;

		uint32* Data = GetData() + StartIndex;
		if (Value)
		{
			if (Count == 1)
			{
				*Data |= StartMask & EndMask;
			}
			else
			{
				*Data++ |= StartMask;
				Count -= 2;
				while (Count != 0)
				{
					*Data++ = ~0;
					--Count;
				}
				*Data |= EndMask;
			}
		}
		else
		{
			if (Count == 1)
			{
				*Data &= ~(StartMask & EndMask);
			}
			else
			{
				*Data++ &= ~StartMask;
				Count -= 2;
				while (Count != 0)
				{
					*Data++ = 0;
					--Count;
				}
				*Data &= ~EndMask;
			}
		}

		CheckInvariants();
	}

	/**
	 * Sets range of bits within the TBitArray to the values read out of a pointer.
	 * @param Index  The index of the first bit to set; must be 0 <= Index <= Num().
	 * @param NumBitsToSet The number of bits to set, must satisify 0 <= NumBitsToSet && Index + NumBitsToSet <= Num().
	 * @param ReadBits The address of sized integers to read the bits from.
	 *                 Bits are read from ReadBits in the current platform's mathematical bitorder (ReadBits[0] & 0x1, ReadBits[0] & 0x2, ... ReadBits[0] & 0x100, ... ReadBits[0] & 0x80000000, ReadBits[1] & 0x1 ...
	 * @param ReadOffsetBits Number of bits into ReadBits at which to start reading. 
	 */
	template <typename InWordType>
	void SetRangeFromRange(int32 Index, int32 NumBitsToSet, const InWordType* ReadBits, int32 ReadOffsetBits = 0)
	{
		check(Index >= 0 && NumBitsToSet >= 0 && Index + NumBitsToSet <= NumBits);
		check(NumBitsToSet == 0 || ReadBits != nullptr);

		// The planned implementation for big endian:
		// iterate over each InWordType in ReadBits, and upcast it to the TBitArray's WordType. Call MemmoveBitsWordOrder(GetData(), Index + n, &UpcastedWord, ReadOffsetBits, Min(NumBitsToSet, BitsPerWord-ReadOffsetBits) for each word
		static_assert(PLATFORM_LITTLE_ENDIAN, "SetRangeFromRange does not yet support big endian platforms");
		FBitArrayMemory::MemmoveBitsWordOrder(GetData(), Index, ReadBits, ReadOffsetBits, NumBitsToSet);
	}

	/**
	 * Sets range of bits within this TBitArray to the values read out another TBitArray.
	 * @param Index  The index of the first bit to set; must be 0 <= Index <= Num().
	 * @param NumBitsToSet The number of bits to set, must satisify 0 <= NumBitsToSet && Index + NumBitsToSet <= Num().
	 * @param ReadBits The value to set the bits to.
	 * @param ReadOffsetBits Number of bits into ReadBits at which to start reading.
	 */
	template <typename OtherAllocator>
	FORCEINLINE void SetRangeFromRange(int32 Index, int32 NumBitsToSet, const TBitArray<OtherAllocator>& ReadBits, int32 ReadOffsetBits = 0)
	{
		check(Index >= 0 && NumBitsToSet >= 0 && Index + NumBitsToSet <= NumBits);
		check(0 <= ReadOffsetBits && ReadOffsetBits + NumBitsToSet <= ReadBits.NumBits);
		FBitArrayMemory::MemmoveBitsWordOrder(GetData(), Index, ReadBits.GetData(), ReadOffsetBits, NumBitsToSet);
	}

	/**
	 * Reads a range of bits within the array and writes them to the given pointer.
	 * @param Index  The index of the first bit to read; must be 0 <= Index <= Num().
	 * @param NumBitsToGet The number of bits to read, must satisify 0 <= NumBitsToGet && Index + NumBitsToGet <= Num().
	 * @param WriteBits The address of sized integers to write the bits to.
	 *                  Bits are written into WriteBits in the current platform's mathematical bitorder (WriteBits[0] & 0x1, WriteBits[0] & 0x2, ... WriteBits[0] & 0x100, ... WriteBits[0] & 0x80000000, WriteBits[1] & 0x1 ...
	 * @param WriteOffsetBits Number of bits into WriteBits at which to start writing.
	 */
	template <typename InWordType>
	FORCEINLINE void GetRange(int32 Index, int32 NumBitsToGet, InWordType* WriteBits, int32 WriteOffsetBits = 0) const
	{
		check(Index >= 0 && NumBitsToGet >= 0 && Index + NumBitsToGet <= NumBits);
		check(NumBitsToGet == 0 || WriteBits != nullptr);

		// See SetRangeFromRange for notes on the planned big endian implementation
		static_assert(PLATFORM_LITTLE_ENDIAN, "SetRangeFromRange does not yet support big endian platforms");
		FBitArrayMemory::MemmoveBitsWordOrder(WriteBits, WriteOffsetBits, GetData(), Index, NumBitsToGet);
	}

	/**
	 * Removes bits from the array.
	 * @param BaseIndex - The index of the first bit to remove.
	 * @param NumBitsToRemove - The number of consecutive bits to remove.
	 */
	void RemoveAt(int32 BaseIndex,int32 NumBitsToRemove = 1)
	{
		check(BaseIndex >= 0 && NumBitsToRemove >= 0 && BaseIndex + NumBitsToRemove <= NumBits);

		if (BaseIndex + NumBitsToRemove != NumBits)
		{
			// MemmoveBitsWordOrder handles overlapping source and dest
			uint32 NumToShift = NumBits - (BaseIndex + NumBitsToRemove);
			FBitArrayMemory::MemmoveBitsWordOrder(GetData(), BaseIndex, GetData(), BaseIndex + NumBitsToRemove, NumToShift);
		}

		NumBits -= NumBitsToRemove;

		ClearPartialSlackBits();
		CheckInvariants();
	}

	/* Removes bits from the array by swapping them with bits at the end of the array.
	 * This is mainly implemented so that other code using TArray::RemoveSwap will have
	 * matching indices.
 	 * @param BaseIndex - The index of the first bit to remove.
	 * @param NumBitsToRemove - The number of consecutive bits to remove.
	 */
	void RemoveAtSwap( int32 BaseIndex, int32 NumBitsToRemove=1 )
	{
		check(BaseIndex >= 0 && NumBitsToRemove >= 0 && BaseIndex + NumBitsToRemove <= NumBits);
		if( BaseIndex < NumBits - NumBitsToRemove )
		{
			// Copy bits from the end to the region we are removing
			for( int32 Index=0;Index<NumBitsToRemove;Index++ )
			{
#if PLATFORM_MAC || PLATFORM_LINUX
				// Clang compiler doesn't understand the short syntax, so let's be explicit
				int32 FromIndex = NumBits - NumBitsToRemove + Index;
				FConstBitReference From(GetData()[FromIndex / NumBitsPerDWORD],1 << (FromIndex & (NumBitsPerDWORD - 1)));

				int32 ToIndex = BaseIndex + Index;
				FBitReference To(GetData()[ToIndex / NumBitsPerDWORD],1 << (ToIndex & (NumBitsPerDWORD - 1)));

				To = (bool)From;
#else
				(*this)[BaseIndex + Index] = (bool)(*this)[NumBits - NumBitsToRemove + Index];
#endif
			}
		}

		// Remove the bits from the end of the array.
		NumBits -= NumBitsToRemove;

		ClearPartialSlackBits();
		CheckInvariants();
	}
	

	/** 
	 * Helper function to return the amount of memory allocated by this container 
	 * @return number of bytes allocated by this container
	 */
	uint32 GetAllocatedSize( void ) const
	{
		return FBitSet::CalculateNumWords(MaxBits) * sizeof(uint32);
	}

	/** Tracks the container's memory use through an archive. */
	void CountBytes(FArchive& Ar) const
	{
		Ar.CountBytes(
			GetNumWords() * sizeof(uint32),
			GetMaxWords() * sizeof(uint32)
		);
	}

	/**
	 * Finds the first true/false bit in the array, and returns the bit index.
	 * If there is none, INDEX_NONE is returned.
	 */
	int32 Find(bool bValue) const
	{
		// Iterate over the array until we see a word with a matching bit
		const uint32 Test = bValue ? 0u : (uint32)-1;

		const uint32* RESTRICT DwordArray = GetData();
		const int32 LocalNumBits = NumBits;
		const int32 DwordCount = FBitSet::CalculateNumWords(LocalNumBits);
		int32 DwordIndex = 0;
		while (DwordIndex < DwordCount && DwordArray[DwordIndex] == Test)
		{
			++DwordIndex;
		}

		if (DwordIndex < DwordCount)
		{
			// If we're looking for a false, then we flip the bits - then we only need to find the first one bit
			const uint32 Bits = bValue ? (DwordArray[DwordIndex]) : ~(DwordArray[DwordIndex]);
			UE_ASSUME(Bits != 0);
			const int32 LowestBitIndex = FMath::CountTrailingZeros(Bits) + (DwordIndex << NumBitsPerDWORDLogTwo);
			if (LowestBitIndex < LocalNumBits)
			{
				return LowestBitIndex;
			}
		}

		return INDEX_NONE;
	}

	/**
	* Finds the last true/false bit in the array, and returns the bit index.
	* If there is none, INDEX_NONE is returned.
	*/
	int32 FindLast(bool bValue) const 
	{
		const int32 LocalNumBits = NumBits;

		// Get the correct mask for the last word
		uint32 Mask = GetLastWordMask();

		// Iterate over the array until we see a word with a zero bit.
		uint32 DwordIndex = FBitSet::CalculateNumWords(LocalNumBits);
		const uint32* RESTRICT DwordArray = GetData();
		const uint32 Test = bValue ? 0u : ~0u;
		for (;;)
		{
			if (DwordIndex == 0)
			{
				return INDEX_NONE;
			}
			--DwordIndex;
			if ((DwordArray[DwordIndex] & Mask) != (Test & Mask))
			{
				break;
			}
			Mask = ~0u;
		}

		// Flip the bits, then we only need to find the first one bit -- easy.
		const uint32 Bits = (bValue ? DwordArray[DwordIndex] : ~DwordArray[DwordIndex]) & Mask;
		UE_ASSUME(Bits != 0);

		uint32 BitIndex = (NumBitsPerDWORD - 1) - FMath::CountLeadingZeros(Bits);

		int32 Result = BitIndex + (DwordIndex << NumBitsPerDWORDLogTwo);
		return Result;
	}

	FORCEINLINE bool Contains(bool bValue) const
	{
		return Find(bValue) != INDEX_NONE;
	}

	/**
	 * Finds the first zero bit in the array, sets it to true, and returns the bit index.
	 * If there is none, INDEX_NONE is returned.
	 */
	int32 FindAndSetFirstZeroBit(int32 ConservativeStartIndex = 0)
	{
		// Iterate over the array until we see a word with a zero bit.
		uint32* RESTRICT DwordArray = GetData();
		const int32 LocalNumBits = NumBits;
		const int32 DwordCount = FBitSet::CalculateNumWords(LocalNumBits);
		int32 DwordIndex = FMath::DivideAndRoundDown(ConservativeStartIndex, NumBitsPerDWORD);
		while (DwordIndex < DwordCount && DwordArray[DwordIndex] == (uint32)-1)
		{
			++DwordIndex;
		}

		if (DwordIndex < DwordCount)
		{
			// Flip the bits, then we only need to find the first one bit -- easy.
			const uint32 Bits = ~(DwordArray[DwordIndex]);
			UE_ASSUME(Bits != 0);
			const uint32 LowestBit = (Bits) & (-(int32)Bits);
			const int32 LowestBitIndex = FMath::CountTrailingZeros(Bits) + (DwordIndex << NumBitsPerDWORDLogTwo);
			if (LowestBitIndex < LocalNumBits)
			{
				DwordArray[DwordIndex] |= LowestBit;
				CheckInvariants();
				return LowestBitIndex;
			}
		}

		return INDEX_NONE;
	}

	/**
	 * Finds the last zero bit in the array, sets it to true, and returns the bit index.
	 * If there is none, INDEX_NONE is returned.
	 */
	int32 FindAndSetLastZeroBit()
	{
		const int32 LocalNumBits = NumBits;

		// Get the correct mask for the last word
		uint32 Mask = GetLastWordMask();

		// Iterate over the array until we see a word with a zero bit.
		uint32 DwordIndex = FBitSet::CalculateNumWords(LocalNumBits);
		uint32* RESTRICT DwordArray = GetData();
		for (;;)
		{
			if (DwordIndex == 0)
			{
				return INDEX_NONE;
			}
			--DwordIndex;
			if ((DwordArray[DwordIndex] & Mask) != Mask)
			{
				break;
			}
			Mask = ~0u;
		}

		// Flip the bits, then we only need to find the first one bit -- easy.
		const uint32 Bits = ~DwordArray[DwordIndex] & Mask;
		UE_ASSUME(Bits != 0);

		uint32 BitIndex = (NumBitsPerDWORD - 1) - FMath::CountLeadingZeros(Bits);
		DwordArray[DwordIndex] |= 1u << BitIndex;

		CheckInvariants();

		int32 Result = BitIndex + (DwordIndex << NumBitsPerDWORDLogTwo);
		return Result;
	}


	/**
	 * Return the bitwise AND of two bit arrays. The resulting bit array will be sized according to InFlags.
	 */
	static TBitArray BitwiseAND(const TBitArray& A, const TBitArray& B, EBitwiseOperatorFlags InFlags)
	{
		TBitArray Result;
		BitwiseBinaryOperatorImpl(A, B, Result, InFlags, [](uint32 InA, uint32 InB) { return InA & InB; });
		return Result;
	}

	/**
	 * Perform a bitwise AND on this bit array with another. This array receives the result and will be sized max(A.Num(), B.Num()).
	 */
	TBitArray& CombineWithBitwiseAND(const TBitArray& InOther, EBitwiseOperatorFlags InFlags)
	{
		BitwiseOperatorImpl(InOther, *this, InFlags, [](uint32 InA, uint32 InB) { return InA & InB; });
		return *this;
	}

	/**
	 * Return the bitwise OR of two bit arrays. The resulting bit array will be sized according to InFlags.
	 */
	static TBitArray BitwiseOR(const TBitArray& A, const TBitArray& B, EBitwiseOperatorFlags InFlags)
	{
		check(&A != &B);

		TBitArray Result;
		BitwiseBinaryOperatorImpl(A, B, Result, InFlags, [](uint32 InA, uint32 InB) { return InA | InB; });
		return Result;
	}

	/**
	 * Return the bitwise OR of two bit arrays. The resulting bit array will be sized according to InFlags.
	 */
	TBitArray& CombineWithBitwiseOR(const TBitArray& InOther, EBitwiseOperatorFlags InFlags)
	{
		BitwiseOperatorImpl(InOther, *this, InFlags, [](uint32 InA, uint32 InB) { return InA | InB; });
		return *this;
	}

	/**
	 * Return the bitwise XOR of two bit arrays. The resulting bit array will be sized according to InFlags.
	 */
	static TBitArray BitwiseXOR(const TBitArray& A, const TBitArray& B, EBitwiseOperatorFlags InFlags)
	{
		TBitArray Result;
		BitwiseBinaryOperatorImpl(A, B, Result, InFlags, [](uint32 InA, uint32 InB) { return InA ^ InB; });
		return Result;
	}

	/**
	 * Return the bitwise XOR of two bit arrays. The resulting bit array will be sized according to InFlags.
	 */
	TBitArray& CombineWithBitwiseXOR(const TBitArray& InOther, EBitwiseOperatorFlags InFlags)
	{
		BitwiseOperatorImpl(InOther, *this, InFlags, [](uint32 InA, uint32 InB) { return InA ^ InB; });
		return *this;
	}

	/**
	 * Perform a bitwise NOT on all the bits in this array
	 */
	void BitwiseNOT()
	{
		for (FWordIterator It(*this); It; ++It)
		{
			It.SetWord(~It.GetWord());
		}
	}

	/**
	 * Count the number of set bits in this array  FromIndex <= bit < ToIndex
	 */
	int32 CountSetBits(int32 FromIndex = 0, int32 ToIndex = INDEX_NONE) const
	{
		if (ToIndex == INDEX_NONE)
		{
			ToIndex = NumBits;
		}

		checkSlow(FromIndex >= 0);
		checkSlow(ToIndex >= FromIndex && ToIndex <= NumBits);

		int32 NumSetBits = 0;
		for (FConstWordIterator It(*this, FromIndex, ToIndex); It; ++It)
		{
			NumSetBits += FMath::CountBits(It.GetWord());
		}
		return NumSetBits;
	}

	/**
	 * Returns true if Other contains all the same set bits as this, accounting for differences in length.
	 * Similar to operator== but can handle different length arrays by zero or one-filling missing bits.
	 *
	 * @param Other The array to compare against
	 * @param bMissingBitValue The value to use for missing bits when considering bits that are outside the range of either array
	 * @return true if this array matches Other, including any missing bits, false otherwise
	 */
	bool CompareSetBits(const TBitArray& Other, const bool bMissingBitValue) const
	{
		const uint32 MissingBitsFill = bMissingBitValue ? ~0u : 0;

		FConstWordIterator ThisIterator(*this);
		FConstWordIterator OtherIterator(Other);

		ThisIterator.FillMissingBits(MissingBitsFill);
		OtherIterator.FillMissingBits(MissingBitsFill);

		while (ThisIterator || OtherIterator)
		{
			const uint32 A = ThisIterator  ? ThisIterator.GetWord()  : MissingBitsFill;
			const uint32 B = OtherIterator ? OtherIterator.GetWord() : MissingBitsFill;
			if (A != B)
			{
				return false;
			}

			++ThisIterator;
			++OtherIterator;
		}

		return true;
	}

	/**
	 * Pad this bit array with the specified value to ensure that it is at least the specified length. Does nothing if Num() >= DesiredNum.
	 * 
	 * @param DesiredNum The desired number of elements that should exist in the array.
	 * @param bPadValue  The value to pad with (0 or 1)
	 * @return The number of bits that were added to the array, or 0 if Num() >= DesiredNum.
	 */
	int32 PadToNum(int32 DesiredNum, bool bPadValue)
	{
		const int32 NumToAdd = DesiredNum - Num();
		if (NumToAdd > 0)
		{
			Add(bPadValue, NumToAdd);
			return NumToAdd;
		}
		return 0;
	}

	// Accessors.
	FORCEINLINE bool IsValidIndex(int32 InIndex) const
	{
		return InIndex >= 0 && InIndex < NumBits;
	}

	/**
	 * Returns true if the bit array is empty and contains no elements. 
	 *
	 * @returns True if the bit array is empty.
	 * @see Num
	 */
	bool IsEmpty() const 
	{ 
		return NumBits == 0; 
	}

	FORCEINLINE int32 Num() const { return NumBits; }
	FORCEINLINE int32 Max() const { return MaxBits; }
	FORCEINLINE FBitReference operator[](int32 Index)
	{
		check(Index>=0 && Index<NumBits);
		return FBitReference(
			GetData()[Index / NumBitsPerDWORD],
			1 << (Index & (NumBitsPerDWORD - 1))
			);
	}
	FORCEINLINE const FConstBitReference operator[](int32 Index) const
	{
		check(Index>=0 && Index<NumBits);
		return FConstBitReference(
			GetData()[Index / NumBitsPerDWORD],
			1 << (Index & (NumBitsPerDWORD - 1))
			);
	}
	FORCEINLINE FBitReference AccessCorrespondingBit(const FRelativeBitReference& RelativeReference)
	{
		checkSlow(RelativeReference.Mask);
		checkSlow(RelativeReference.WordIndex >= 0);
		checkSlow(((uint32)RelativeReference.WordIndex + 1) * NumBitsPerDWORD - 1 - FMath::CountLeadingZeros(RelativeReference.Mask) < (uint32)NumBits);
		return FBitReference(
			GetData()[RelativeReference.WordIndex],
			RelativeReference.Mask
			);
	}
	FORCEINLINE const FConstBitReference AccessCorrespondingBit(const FRelativeBitReference& RelativeReference) const
	{
		checkSlow(RelativeReference.Mask);
		checkSlow(RelativeReference.WordIndex >= 0);
		checkSlow(((uint32)RelativeReference.WordIndex + 1) * NumBitsPerDWORD - 1 - FMath::CountLeadingZeros(RelativeReference.Mask) < (uint32)NumBits);
		return FConstBitReference(
			GetData()[RelativeReference.WordIndex],
			RelativeReference.Mask
			);
	}

	/** BitArray iterator. */
	class FIterator : public FRelativeBitReference
	{
	public:
		FORCEINLINE FIterator(TBitArray<Allocator>& InArray,int32 StartIndex = 0)
		:	FRelativeBitReference(StartIndex)
		,	Array(InArray)
		,	Index(StartIndex)
		{
		}
		FORCEINLINE FIterator& operator++()
		{
			++Index;
			this->Mask <<= 1;
			if(!this->Mask)
			{
				// Advance to the next uint32.
				this->Mask = 1;
				++this->WordIndex;
			}
			return *this;
		}
		/** conversion to "bool" returning true if the iterator is valid. */
		FORCEINLINE explicit operator bool() const
		{ 
			return Index < Array.Num(); 
		}
		/** inverse of the "bool" operator */
		FORCEINLINE bool operator !() const 
		{
			return !(bool)*this;
		}

		FORCEINLINE FBitReference GetValue() const { return FBitReference(Array.GetData()[this->WordIndex],this->Mask); }
		FORCEINLINE int32 GetIndex() const { return Index; }
	private:
		TBitArray<Allocator>& Array;
		int32 Index;
	};

	/** Const BitArray iterator. */
	class FConstIterator : public FRelativeBitReference
	{
	public:
		FORCEINLINE FConstIterator(const TBitArray<Allocator>& InArray,int32 StartIndex = 0)
		:	FRelativeBitReference(StartIndex)
		,	Array(InArray)
		,	Index(StartIndex)
		{
		}
		FORCEINLINE FConstIterator& operator++()
		{
			++Index;
			this->Mask <<= 1;
			if(!this->Mask)
			{
				// Advance to the next uint32.
				this->Mask = 1;
				++this->WordIndex;
			}
			return *this;
		}

		/** conversion to "bool" returning true if the iterator is valid. */
		FORCEINLINE explicit operator bool() const
		{ 
			return Index < Array.Num(); 
		}
		/** inverse of the "bool" operator */
		FORCEINLINE bool operator !() const 
		{
			return !(bool)*this;
		}

		FORCEINLINE FConstBitReference GetValue() const { return FConstBitReference(Array.GetData()[this->WordIndex],this->Mask); }
		FORCEINLINE int32 GetIndex() const { return Index; }
	private:
		const TBitArray<Allocator>& Array;
		int32 Index;
	};

	/** Const reverse iterator. */
	class FConstReverseIterator : public FRelativeBitReference
	{
	public:
		FORCEINLINE FConstReverseIterator(const TBitArray<Allocator>& InArray)
			:	FRelativeBitReference(InArray.Num() - 1)
			,	Array(InArray)
			,	Index(InArray.Num() - 1)
		{
		}
		FORCEINLINE FConstReverseIterator& operator++()
		{
			--Index;
			this->Mask >>= 1;
			if(!this->Mask)
			{
				// Advance to the next uint32.
				this->Mask = (1 << (NumBitsPerDWORD-1));
				--this->WordIndex;
			}
			return *this;
		}

		/** conversion to "bool" returning true if the iterator is valid. */
		FORCEINLINE explicit operator bool() const
		{ 
			return Index >= 0; 
		}
		/** inverse of the "bool" operator */
		FORCEINLINE bool operator !() const 
		{
			return !(bool)*this;
		}

		FORCEINLINE FConstBitReference GetValue() const { return FConstBitReference(Array.GetData()[this->WordIndex],this->Mask); }
		FORCEINLINE int32 GetIndex() const { return Index; }
	private:
		const TBitArray<Allocator>& Array;
		int32 Index;
	};

	FORCEINLINE const uint32* GetData() const
	{
		return (uint32*)AllocatorInstance.GetAllocation();
	}

	FORCEINLINE uint32* GetData()
	{
		return (uint32*)AllocatorInstance.GetAllocation();
	}

private:

	template<typename ProjectionType>
	static void BitwiseBinaryOperatorImpl(const TBitArray& InA, const TBitArray& InB, TBitArray& OutResult, EBitwiseOperatorFlags InFlags, ProjectionType&& InProjection)
	{
		check((&InA != &InB) && (&InA != &OutResult) && (&InB != &OutResult));

		if (EnumHasAnyFlags(InFlags, EBitwiseOperatorFlags::MinSize))
		{

			const int32 MinNumBits = FMath::Min(InA.Num(), InB.Num());
			if (MinNumBits > 0)
			{
				OutResult.Reserve(MinNumBits);
				OutResult.NumBits = MinNumBits;

				FConstWordIterator IteratorA(InA);
				FConstWordIterator IteratorB(InB);

				FWordIterator IteratorResult(OutResult);

				for ( ; IteratorResult; ++IteratorResult, ++IteratorA, ++IteratorB)
				{
					const uint32 NewValue = Invoke(InProjection, IteratorA.GetWord(), IteratorB.GetWord());
					IteratorResult.SetWord(NewValue);
				}
			}

		}
		else if (EnumHasAnyFlags(InFlags, EBitwiseOperatorFlags::MaxSize))
		{

			const int32 MaxNumBits = FMath::Max(InA.Num(), InB.Num());
			const uint32 MissingBitsFill = EnumHasAnyFlags(InFlags, EBitwiseOperatorFlags::OneFillMissingBits) ? ~0u : 0;

			if (MaxNumBits)
			{
				OutResult.Reserve(MaxNumBits);
				OutResult.NumBits = MaxNumBits;

				FConstWordIterator IteratorA(InA);
				FConstWordIterator IteratorB(InB);

				IteratorA.FillMissingBits(MissingBitsFill);
				IteratorB.FillMissingBits(MissingBitsFill);

				FWordIterator IteratorResult(OutResult);

				for ( ; IteratorResult; ++IteratorResult, ++IteratorA, ++IteratorB)
				{
					uint32 A = IteratorA ? IteratorA.GetWord() : MissingBitsFill;
					uint32 B = IteratorB ? IteratorB.GetWord() : MissingBitsFill;

					IteratorResult.SetWord(Invoke(InProjection, A, B));
				}
			}

		}
		else
		{
			checkf(false, TEXT("Invalid size flag specified for binary bitwise AND"));
		}

		OutResult.CheckInvariants();
	}


	template<typename ProjectionType>
	static void BitwiseOperatorImpl(const TBitArray& InOther, TBitArray& OutResult, EBitwiseOperatorFlags InFlags, ProjectionType&& InProjection)
	{
		check(&InOther != &OutResult);

		int32 NewNumBits = OutResult.NumBits;
		if (EnumHasAnyFlags(InFlags, EBitwiseOperatorFlags::MinSize))
		{
			NewNumBits = FMath::Min(InOther.Num(), OutResult.Num());
		}
		else if (EnumHasAnyFlags(InFlags, EBitwiseOperatorFlags::MaxSize))
		{
			NewNumBits = FMath::Max(InOther.Num(), OutResult.Num());
		}

		const int32 SizeDifference = NewNumBits - OutResult.NumBits;
		if (SizeDifference < 0)
		{
			OutResult.NumBits = NewNumBits;
			OutResult.ClearPartialSlackBits();
		}
		else if (SizeDifference > 0)
		{
			const bool bPadValue = EnumHasAnyFlags(InFlags, EBitwiseOperatorFlags::OneFillMissingBits);
			OutResult.Add(bPadValue, SizeDifference);
		}

		const uint32 MissingBitsFill = EnumHasAnyFlags(InFlags, EBitwiseOperatorFlags::OneFillMissingBits) ? ~0u : 0;
		if (OutResult.NumBits != 0)
		{
			FConstWordIterator IteratorOther(InOther);
			IteratorOther.FillMissingBits(MissingBitsFill);

			FWordIterator IteratorResult(OutResult);

			for ( ; IteratorResult; ++IteratorResult, ++IteratorOther)
			{
				const uint32 OtherValue = IteratorOther ? IteratorOther.GetWord() : MissingBitsFill;
				IteratorResult.SetWord(Invoke(InProjection, IteratorResult.GetWord(), OtherValue));
			}
		}

		OutResult.CheckInvariants();
	}


	template<typename WordType>
	struct TWordIteratorBase
	{
		explicit operator bool() const
		{
			return CurrentIndex < NumWords;
		}

		int32 GetIndex() const
		{
			return CurrentIndex;
		}

		uint32 GetWord() const
		{
			checkSlow(CurrentIndex < NumWords);

			if (CurrentMask == ~0u)
			{
				return Data[CurrentIndex];
			}
			else if (MissingBitsFill == 0)
			{
				return Data[CurrentIndex] & CurrentMask;
			}
			else
			{
				return (Data[CurrentIndex] & CurrentMask) | (MissingBitsFill & ~CurrentMask);
			}
		}

		void operator++()
		{
			++this->CurrentIndex;
			if (this->CurrentIndex == NumWords-1)
			{
				CurrentMask = FinalMask;
			}
			else
			{
				CurrentMask = ~0u;
			}
		}

		void FillMissingBits(uint32 InMissingBitsFill)
		{
			MissingBitsFill = InMissingBitsFill;
		}

	protected:

		explicit TWordIteratorBase(WordType* InData, int32 InStartBitIndex, int32 InEndBitIndex)
			: Data(InData)
			, CurrentIndex(InStartBitIndex / NumBitsPerDWORD)
			, NumWords(FMath::DivideAndRoundUp(InEndBitIndex, NumBitsPerDWORD))
			, CurrentMask(~0u << (InStartBitIndex % NumBitsPerDWORD))
			, FinalMask(~0u)
			, MissingBitsFill(0)
		{
			const int32 Shift = NumBitsPerDWORD - (InEndBitIndex % NumBitsPerDWORD);
			if (Shift < NumBitsPerDWORD)
			{
				FinalMask = ~0u >> Shift;
			}

			if (CurrentIndex == NumWords - 1)
			{
				CurrentMask &= FinalMask;
				FinalMask = CurrentMask;
			}
		}

		WordType* RESTRICT Data;

		int32 CurrentIndex;
		int32 NumWords;

		uint32 CurrentMask;
		uint32 FinalMask;
		uint32 MissingBitsFill;
	};

public:
	struct FConstWordIterator : TWordIteratorBase<const uint32>
	{
		explicit FConstWordIterator(const TBitArray<Allocator>& InArray)
			: TWordIteratorBase<const uint32>(InArray.GetData(), 0, InArray.Num())
		{}

		explicit FConstWordIterator(const TBitArray<Allocator>& InArray, int32 InStartBitIndex, int32 InEndBitIndex)
			: TWordIteratorBase<const uint32>(InArray.GetData(), InStartBitIndex, InEndBitIndex)
		{
			checkSlow(InStartBitIndex <= InEndBitIndex && InStartBitIndex <= InArray.Num() && InEndBitIndex <= InArray.Num());
			checkSlow(InStartBitIndex >= 0 && InEndBitIndex >= 0);
		}
	};


	struct FWordIterator : TWordIteratorBase<uint32>
	{
		explicit FWordIterator(TBitArray<Allocator>& InArray)
			: TWordIteratorBase<uint32>(InArray.GetData(), 0, InArray.Num())
		{}

		void SetWord(uint32 InWord)
		{
			checkSlow(this->CurrentIndex < this->NumWords);

			if (this->CurrentIndex == this->NumWords-1)
			{
				this->Data[this->CurrentIndex] = InWord & this->FinalMask;
			}
			else
			{
				this->Data[this->CurrentIndex] = InWord;
			}
		}
	};

private:
	AllocatorType AllocatorInstance;
	int32         NumBits;
	int32         MaxBits;

	FORCENOINLINE void Realloc(int32 PreviousNumBits)
	{
		const uint32 PreviousNumWords = FBitSet::CalculateNumWords(PreviousNumBits);
		const uint32 MaxWords = FBitSet::CalculateNumWords(MaxBits);

		AllocatorInstance.ResizeAllocation(PreviousNumWords,MaxWords,sizeof(uint32));
		ClearPartialSlackBits(); // Implement class invariant
	}

	void SetBitNoCheck(int32 Index, bool Value)
	{
		uint32& Word = GetData()[Index/NumBitsPerDWORD];
		uint32 BitOffset = (Index % NumBitsPerDWORD);
		Word = (Word & ~(1 << BitOffset)) | (((uint32)Value) << BitOffset);
	}

	/**
	 * Clears the slack bits within the final partially relevant Word
	 */
	void ClearPartialSlackBits()
	{
		// TBitArray has a contract about bits outside of the active range - the bits in the final word past NumBits are guaranteed to be 0
		// This prevents easy-to-make determinism errors from users of TBitArray that do not carefully mask the final word.
		// It also allows us optimize some operations which would otherwise require us to mask the last word.
		const int32 UsedBits = NumBits % NumBitsPerDWORD;
		if (UsedBits != 0)
		{
			const int32  LastWordIndex = NumBits / NumBitsPerDWORD;
			const uint32 SlackMask = FullWordMask >> (NumBitsPerDWORD - UsedBits);

			uint32* LastWord = (GetData() + LastWordIndex);
			*LastWord = *LastWord & SlackMask;
		}
	}

	template<bool bFreezeMemoryImage, typename Dummy=void>
	struct TSupportsFreezeMemoryImageHelper
	{
		static void WriteMemoryImage(FMemoryImageWriter& Writer, const TBitArray&) { Writer.WriteBytes(TBitArray()); }
	};

	template<typename Dummy>
	struct TSupportsFreezeMemoryImageHelper<true, Dummy>
	{
		static void WriteMemoryImage(FMemoryImageWriter& Writer, const TBitArray& Object)
		{
			const int32 NumWords = FMath::DivideAndRoundUp(Object.NumBits, NumBitsPerDWORD);
			Object.AllocatorInstance.WriteMemoryImage(Writer, StaticGetTypeLayoutDesc<uint32>(), NumWords);
			Writer.WriteBytes(Object.NumBits);
			Writer.WriteBytes(Object.NumBits);
		}
	};

public:
	void WriteMemoryImage(FMemoryImageWriter& Writer) const
	{
		static constexpr bool bSupportsFreezeMemoryImage = TAllocatorTraits<Allocator>::SupportsFreezeMemoryImage;
		checkf(!Writer.Is32BitTarget(), TEXT("TBitArray does not currently support freezing for 32bits"));
		TSupportsFreezeMemoryImageHelper<bSupportsFreezeMemoryImage>::WriteMemoryImage(Writer, *this);
	}
};

namespace Freeze
{
	template<typename Allocator>
	void IntrinsicWriteMemoryImage(FMemoryImageWriter& Writer, const TBitArray<Allocator>& Object, const FTypeLayoutDesc&)
	{
		Object.WriteMemoryImage(Writer);
	}
}

DECLARE_TEMPLATE_INTRINSIC_TYPE_LAYOUT(template<typename Allocator>, TBitArray<Allocator>);

template<typename Allocator>
FORCEINLINE uint32 GetTypeHash(const TBitArray<Allocator>& BitArray)
{
	uint32 NumWords = FBitSet::CalculateNumWords(BitArray.Num());
	uint32 Hash = NumWords;
	const uint32* Data = BitArray.GetData();
	for (uint32 i = 0; i < NumWords; i++)
	{
		Hash ^= Data[i];
	}
	return Hash;
}

template<typename Allocator>
struct TContainerTraits<TBitArray<Allocator> > : public TContainerTraitsBase<TBitArray<Allocator> >
{
	static_assert(TAllocatorTraits<Allocator>::SupportsMove, "TBitArray no longer supports move-unaware allocators");
	enum { MoveWillEmptyContainer = TAllocatorTraits<Allocator>::SupportsMove };
};


/** An iterator which only iterates over set bits. */
template<typename Allocator>
class TConstSetBitIterator : public FRelativeBitReference
{
public:

	/** Constructor. */
	TConstSetBitIterator(const TBitArray<Allocator>& InArray,int32 StartIndex = 0)
		: FRelativeBitReference(StartIndex)
		, Array                (InArray)
		, UnvisitedBitMask     ((~0U) << (StartIndex & (NumBitsPerDWORD - 1)))
		, CurrentBitIndex      (StartIndex)
		, BaseBitIndex         (StartIndex & ~(NumBitsPerDWORD - 1))
	{
		check(StartIndex >= 0 && StartIndex <= Array.Num());
		if (StartIndex != Array.Num())
		{
			FindFirstSetBit();
		}
	}

	/** Forwards iteration operator. */
	FORCEINLINE TConstSetBitIterator& operator++()
	{
		// Mark the current bit as visited.
		UnvisitedBitMask &= ~this->Mask;

		// Find the first set bit that hasn't been visited yet.
		FindFirstSetBit();

		return *this;
	}

	FORCEINLINE friend bool operator==(const TConstSetBitIterator& Lhs, const TConstSetBitIterator& Rhs) 
	{
		// We only need to compare the bit index and the array... all the rest of the state is unobservable.
		return Lhs.CurrentBitIndex == Rhs.CurrentBitIndex && &Lhs.Array == &Rhs.Array;
	}

	FORCEINLINE friend bool operator!=(const TConstSetBitIterator& Lhs, const TConstSetBitIterator& Rhs)
	{ 
		return !(Lhs == Rhs);
	}

	/** conversion to "bool" returning true if the iterator is valid. */
	FORCEINLINE explicit operator bool() const
	{ 
		return CurrentBitIndex < Array.Num(); 
	}
	/** inverse of the "bool" operator */
	FORCEINLINE bool operator !() const 
	{
		return !(bool)*this;
	}

	/** Index accessor. */
	FORCEINLINE int32 GetIndex() const
	{
		return CurrentBitIndex;
	}

private:

	const TBitArray<Allocator>& Array;

	uint32 UnvisitedBitMask;
	int32 CurrentBitIndex;
	int32 BaseBitIndex;

	/** Find the first set bit starting with the current bit, inclusive. */
	void FindFirstSetBit()
	{
		const uint32* ArrayData      = Array.GetData();
		const int32   ArrayNum       = Array.Num();
		const int32   LastWordIndex = (ArrayNum - 1) / NumBitsPerDWORD;

		// Advance to the next non-zero uint32.
		uint32 RemainingBitMask = ArrayData[this->WordIndex] & UnvisitedBitMask;
		while (!RemainingBitMask)
		{
			++this->WordIndex;
			BaseBitIndex += NumBitsPerDWORD;
			if (this->WordIndex > LastWordIndex)
			{
				// We've advanced past the end of the array.
				CurrentBitIndex = ArrayNum;
				return;
			}

			RemainingBitMask = ArrayData[this->WordIndex];
			UnvisitedBitMask = ~0;
		}

		// This operation has the effect of unsetting the lowest set bit of BitMask
		const uint32 NewRemainingBitMask = RemainingBitMask & (RemainingBitMask - 1);

		// This operation XORs the above mask with the original mask, which has the effect
		// of returning only the bits which differ; specifically, the lowest bit
		this->Mask = NewRemainingBitMask ^ RemainingBitMask;

		// If the Nth bit was the lowest set bit of BitMask, then this gives us N
		CurrentBitIndex = BaseBitIndex + NumBitsPerDWORD - 1 - FMath::CountLeadingZeros(this->Mask);

		// If we've accidentally iterated off the end of an array but still within the same Word
		// then set the index to the last index of the array
		if (CurrentBitIndex > ArrayNum)
		{
			CurrentBitIndex = ArrayNum;
		}
	}
};


/** An iterator which only iterates over the bits which are set in both of two bit-arrays, 
    if the Both template argument is true, or either if the argument is false. */
template<typename Allocator,typename OtherAllocator, bool Both>
class TConstDualSetBitIterator : public FRelativeBitReference
{
public:

	/** Constructor. */
	FORCEINLINE TConstDualSetBitIterator(
		const TBitArray<Allocator>& InArrayA,
		const TBitArray<OtherAllocator>& InArrayB,
		int32 StartIndex = 0
		)
	:	FRelativeBitReference(StartIndex)
	,	ArrayA(InArrayA)
	,	ArrayB(InArrayB)
	,	UnvisitedBitMask((~0U) << (StartIndex & (NumBitsPerDWORD - 1)))
	,	CurrentBitIndex(StartIndex)
	,	BaseBitIndex(StartIndex & ~(NumBitsPerDWORD - 1))
	{
		check(ArrayA.Num() == ArrayB.Num());

		FindFirstSetBit();
	}

	/** Advancement operator. */
	FORCEINLINE TConstDualSetBitIterator& operator++()
	{
		checkSlow(ArrayA.Num() == ArrayB.Num());

		// Mark the current bit as visited.
		UnvisitedBitMask &= ~this->Mask;

		// Find the first set bit that hasn't been visited yet.
		FindFirstSetBit();

		return *this;

	}

	/** conversion to "bool" returning true if the iterator is valid. */
	FORCEINLINE explicit operator bool() const
	{ 
		return CurrentBitIndex < ArrayA.Num(); 
	}
	/** inverse of the "bool" operator */
	FORCEINLINE bool operator !() const 
	{
		return !(bool)*this;
	}

	/** Index accessor. */
	FORCEINLINE int32 GetIndex() const
	{
		return CurrentBitIndex;
	}

private:

	const TBitArray<Allocator>& ArrayA;
	const TBitArray<OtherAllocator>& ArrayB;

	uint32 UnvisitedBitMask;
	int32 CurrentBitIndex;
	int32 BaseBitIndex;

	/** Find the first bit that is set in both arrays, starting with the current bit, inclusive. */
	void FindFirstSetBit()
	{
		const uint32 EmptyArrayData = 0;
		const uint32* ArrayDataA = IfAThenAElseB(ArrayA.GetData(),&EmptyArrayData);
		const uint32* ArrayDataB = IfAThenAElseB(ArrayB.GetData(),&EmptyArrayData);

		// Advance to the next non-zero uint32.
		uint32 RemainingBitMask;
		
		if (Both)
		{
			RemainingBitMask = ArrayDataA[this->WordIndex] & ArrayDataB[this->WordIndex] & UnvisitedBitMask;
		}
		else
		{
			RemainingBitMask = (ArrayDataA[this->WordIndex] | ArrayDataB[this->WordIndex]) & UnvisitedBitMask;
		}

		while(!RemainingBitMask)
		{
			this->WordIndex++;
			BaseBitIndex += NumBitsPerDWORD;
			const int32 LastWordIndex = (ArrayA.Num() - 1) / NumBitsPerDWORD;
			if (this->WordIndex <= LastWordIndex)
			{
				if (Both)
				{
					RemainingBitMask = ArrayDataA[this->WordIndex] & ArrayDataB[this->WordIndex];
				}
				else
				{
					RemainingBitMask = ArrayDataA[this->WordIndex] | ArrayDataB[this->WordIndex];
				}

				UnvisitedBitMask = ~0;
			}
			else
			{
				// We've advanced past the end of the array.
				CurrentBitIndex = ArrayA.Num();
				return;
			}
		};

		// We can assume that RemainingBitMask!=0 here.
		checkSlow(RemainingBitMask);

		// This operation has the effect of unsetting the lowest set bit of BitMask
		const uint32 NewRemainingBitMask = RemainingBitMask & (RemainingBitMask - 1);

		// This operation XORs the above mask with the original mask, which has the effect
		// of returning only the bits which differ; specifically, the lowest bit
		this->Mask = NewRemainingBitMask ^ RemainingBitMask;

		// If the Nth bit was the lowest set bit of BitMask, then this gives us N
		CurrentBitIndex = BaseBitIndex + NumBitsPerDWORD - 1 - FMath::CountLeadingZeros(this->Mask);
	}
};

// A specialization of TConstDualSetBitIterator for iterating over two TBitArray containers and 
// stop only when bits in the same location in each are both set.
template<typename Allocator, typename OtherAllocator>
using TConstDualBothSetBitIterator = TConstDualSetBitIterator<Allocator, OtherAllocator, true>;

// A specialization of TConstDualSetBitIterator for iterating over two TBitArray containers and 
// stop only when either, or both, of the bits in the same location in each are set.
template<typename Allocator, typename OtherAllocator>
using TConstDualEitherSetBitIterator = TConstDualSetBitIterator<Allocator, OtherAllocator, false>;

// Untyped bit array type for accessing TBitArray data, like FScriptArray for TArray.
// Must have the same memory representation as a TBitArray.
template <typename Allocator, typename InDerivedType>
class TScriptBitArray
{
	using DerivedType = typename TChooseClass<TIsVoidType<InDerivedType>::Value, TScriptBitArray, InDerivedType>::Result;

public:
	/**
	 * Minimal initialization constructor.
	 * @param Value - The value to initial the bits to.
	 * @param InNumBits - The initial number of bits in the array.
	 */
	TScriptBitArray()
		: NumBits(0)
		, MaxBits(0)
	{
	}

	bool IsValidIndex(int32 Index) const
	{
		return Index >= 0 && Index < NumBits;
	}

	FBitReference operator[](int32 Index)
	{
		check(IsValidIndex(Index));
		return FBitReference(GetData()[Index / NumBitsPerDWORD], 1 << (Index & (NumBitsPerDWORD - 1)));
	}

	FConstBitReference operator[](int32 Index) const
	{
		check(IsValidIndex(Index));
		return FConstBitReference(GetData()[Index / NumBitsPerDWORD], 1 << (Index & (NumBitsPerDWORD - 1)));
	}

	void MoveAssign(DerivedType& Other)
	{
		checkSlow(this != &Other);
		Empty(0);
		AllocatorInstance.MoveToEmpty(Other.AllocatorInstance);
		NumBits = Other.NumBits; Other.NumBits = 0;
		MaxBits = Other.MaxBits; Other.MaxBits = 0;
	}

	void Empty(int32 Slack = 0)
	{
		NumBits = 0;

		Slack = FBitSet::CalculateNumWords(Slack) * NumBitsPerDWORD;
		// If the expected number of bits doesn't match the allocated number of bits, reallocate.
		if (MaxBits != Slack)
		{
			MaxBits = Slack;
			Realloc(0);
		}
	}

	int32 Add(const bool Value)
	{
		const int32 Index = NumBits;
		NumBits++;
		if (NumBits > MaxBits)
		{
			ReallocGrow(NumBits - 1);
		}
		(*this)[Index] = Value;
		return Index;
	}

private:
	typedef typename Allocator::template ForElementType<uint32> AllocatorType;

	AllocatorType AllocatorInstance;
	int32         NumBits;
	int32         MaxBits;

	// This function isn't intended to be called, just to be compiled to validate the correctness of the type.
	static void CheckConstraints()
	{
		typedef TScriptBitArray ScriptType;
		typedef TBitArray<>     RealType;

		// Check that the class footprint is the same
		static_assert(sizeof (ScriptType) == sizeof (RealType), "TScriptBitArray's size doesn't match TBitArray");
		static_assert(alignof(ScriptType) == alignof(RealType), "TScriptBitArray's alignment doesn't match TBitArray");

		// Check member sizes
		static_assert(sizeof(DeclVal<ScriptType>().AllocatorInstance) == sizeof(DeclVal<RealType>().AllocatorInstance), "TScriptBitArray's AllocatorInstance member size does not match TBitArray's");
		static_assert(sizeof(DeclVal<ScriptType>().NumBits)           == sizeof(DeclVal<RealType>().NumBits),           "TScriptBitArray's NumBits member size does not match TBitArray's");
		static_assert(sizeof(DeclVal<ScriptType>().MaxBits)           == sizeof(DeclVal<RealType>().MaxBits),           "TScriptBitArray's MaxBits member size does not match TBitArray's");

		// Check member offsets
		static_assert(STRUCT_OFFSET(ScriptType, AllocatorInstance) == STRUCT_OFFSET(RealType, AllocatorInstance), "TScriptBitArray's AllocatorInstance member offset does not match TBitArray's");
		static_assert(STRUCT_OFFSET(ScriptType, NumBits)           == STRUCT_OFFSET(RealType, NumBits),           "TScriptBitArray's NumBits member offset does not match TBitArray's");
		static_assert(STRUCT_OFFSET(ScriptType, MaxBits)           == STRUCT_OFFSET(RealType, MaxBits),           "TScriptBitArray's MaxBits member offset does not match TBitArray's");
	}

	FORCEINLINE uint32* GetData()
	{
		return (uint32*)AllocatorInstance.GetAllocation();
	}

	FORCEINLINE const uint32* GetData() const
	{
		return (const uint32*)AllocatorInstance.GetAllocation();
	}

	FORCENOINLINE void Realloc(int32 PreviousNumBits)
	{
		const uint32 MaxWords = AllocatorInstance.CalculateSlackReserve(
			FBitSet::CalculateNumWords(MaxBits),
			sizeof(uint32)
			);
		MaxBits = MaxWords * NumBitsPerDWORD;
		const uint32 PreviousNumWords = FBitSet::CalculateNumWords(PreviousNumBits);

		AllocatorInstance.ResizeAllocation(PreviousNumWords, MaxWords, sizeof(uint32));

		if (MaxWords && MaxWords > PreviousNumWords)
		{
			// Reset the newly allocated slack Words.
			FMemory::Memzero((uint32*)AllocatorInstance.GetAllocation() + PreviousNumWords, (MaxWords - PreviousNumWords) * sizeof(uint32));
		}
	}
	FORCENOINLINE void ReallocGrow(int32 PreviousNumBits)
	{
		// Allocate memory for the new bits.
		const uint32 MaxWords = AllocatorInstance.CalculateSlackGrow(
			FBitSet::CalculateNumWords(NumBits),
			FBitSet::CalculateNumWords(MaxBits),
			sizeof(uint32)
			);
		MaxBits = MaxWords * NumBitsPerDWORD;
		const uint32 PreviousNumWords = FBitSet::CalculateNumWords(PreviousNumBits);
		AllocatorInstance.ResizeAllocation(PreviousNumWords, MaxWords, sizeof(uint32));
		if (MaxWords && MaxWords > PreviousNumWords)
		{
			// Reset the newly allocated slack Words.
			FMemory::Memzero((uint32*)AllocatorInstance.GetAllocation() + PreviousNumWords, (MaxWords - PreviousNumWords) * sizeof(uint32));
		}
	}

public:
	// These should really be private, because they shouldn't be called, but there's a bunch of code
	// that needs to be fixed first.
	TScriptBitArray(const TScriptBitArray&) { check(false); }
	void operator=(const TScriptBitArray&) { check(false); }
};

template <typename AllocatorType, typename InDerivedType>
struct TIsZeroConstructType<TScriptBitArray<AllocatorType, InDerivedType>>
{
	enum { Value = true };
};

class FScriptBitArray : public TScriptBitArray<FDefaultBitArrayAllocator, FScriptBitArray>
{
	using Super = TScriptBitArray<FDefaultBitArrayAllocator, FScriptBitArray>;

public:
	using Super::Super;
};
