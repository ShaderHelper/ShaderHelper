// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/SortedMap.h"
#include "Misc/StringBuilder.h"
#include "Templates/RefCounting.h"
#include "Templates/TypeCompatibleBytes.h"


class FAssetTagValueRef;
class FAssetDataTagMapSharedView;
struct FAssetRegistrySerializationOptions;

/**
 * Helper class for condensing strings of these types into  1 - 3 FNames
 * [class]'[package].[object]'
 * [package].[object]
 * [package]
 */
struct COREUOBJECT_API FAssetRegistryExportPath
{
	FAssetRegistryExportPath() = default;
	explicit FAssetRegistryExportPath(FWideStringView String);
	explicit FAssetRegistryExportPath(FAnsiStringView String);

	FName Class;
	FName Package;
	FName Object;

	FString ToString() const;
	FName ToName() const;
	void ToString(FStringBuilderBase& Out) const;

	bool IsEmpty() const { return Class.IsNone() & Package.IsNone() & Object.IsNone(); } //-V792
	explicit operator bool() const { return !IsEmpty(); }
};

bool operator==(const FAssetRegistryExportPath& A, const FAssetRegistryExportPath& B);
uint32 GetTypeHash(const FAssetRegistryExportPath& Export);

namespace FixedTagPrivate
{
	// Compact FAssetRegistryExportPath equivalent for when all FNames are numberless
	struct FNumberlessExportPath
	{
		FNameEntryId Class;
		FNameEntryId Package;
		FNameEntryId Object;

		FString ToString() const;
		FName ToName() const;
		void ToString(FStringBuilderBase& Out) const;
	};

	bool operator==(const FNumberlessExportPath& A, const FNumberlessExportPath& B);
	uint32 GetTypeHash(const FNumberlessExportPath& Export);

	enum class EValueType : uint32;

	struct FValueId
	{
		static constexpr uint32 TypeBits = 3;
		static constexpr uint32 IndexBits = 32 - TypeBits;

		EValueType 		Type : TypeBits;
		uint32 			Index : IndexBits;

		uint32 ToInt() const
		{
			return static_cast<uint32>(Type) | (Index << TypeBits);
		}

		static FValueId FromInt(uint32 Int)
		{
			return { static_cast<EValueType>((Int << IndexBits) >> IndexBits), Int >> TypeBits };
		}
	};

	struct FNumberedPair
	{
		FName Key;
		FValueId Value;
	};

	struct FNumberlessPair
	{
		FNameEntryId Key;
		FValueId Value;
	};

	// Handle to a tag value owned by a managed FStore
	struct COREUOBJECT_API FValueHandle
	{
		uint32 StoreIndex;
		FValueId Id;

		FString						AsString() const;
		FName						AsName() const;
		FAssetRegistryExportPath	AsExportPath() const;
		bool						AsText(FText& Out) const;
		bool						Equals(FStringView Str) const;
		bool						Contains(const TCHAR* Str) const;
	};

	// Handle to a tag map owned by a managed FStore
	struct COREUOBJECT_API alignas(uint64) FMapHandle
	{
		static constexpr uint32 StoreIndexBits = 14;

		uint16 IsValid : 1;
		uint16 HasNumberlessKeys : 1;
		uint16 StoreIndex : StoreIndexBits; // @see FStoreManager
		uint16 Num;
		uint32 PairBegin;

		const FValueId*						FindValue(FName Key) const;

		TArrayView<const FNumberedPair>		GetNumberedView() const;
		TArrayView<const FNumberlessPair>	GetNumberlessView() const;

		// Get numbered pair at an index regardless if numberless keys are used
		FNumberedPair						At(uint32 Index) const;

		friend bool operator==(FMapHandle A, FMapHandle B);

		template<typename Func>
		void ForEachPair(Func Fn) const
		{
			if (HasNumberlessKeys != 0)
			{
				for (FNumberlessPair Pair : GetNumberlessView())
				{
					Fn(FNumberedPair{FName::CreateFromDisplayId(Pair.Key, 0), Pair.Value});
				}
			}
			else
			{
				for (FNumberedPair Pair : GetNumberedView())
				{
					Fn(Pair);
				}
			}
		}
	};

} // end namespace FixedTagPrivate

/**
 * Reference to a tagged value in a FAssetDataTagMapSharedView
 * 
 * Helps avoid needless FString conversions when using fixed / cooked tag values
 * that are stored as FName, FText or FAssetRegistryExportPath.
 */
class COREUOBJECT_API FAssetTagValueRef
{
	friend class FAssetDataTagMapSharedView;

	class FFixedTagValue
	{
		static constexpr uint64 FixedMask = uint64(1) << 63;

		uint64 Bits;

	public:
		uint64 IsFixed() const { return Bits & FixedMask; }
		uint32 GetStoreIndex() const { return static_cast<uint32>((Bits & ~FixedMask) >> 32); }
		uint32 GetValueId() const { return static_cast<uint32>(Bits); }

		FFixedTagValue() = default;
		FFixedTagValue(uint32 StoreIndex, uint32 ValueId)
		: Bits(FixedMask | (uint64(StoreIndex) << 32) | uint64(ValueId)) 
		{}
	};

#if PLATFORM_32BITS
	class FStringPointer
	{
		uint64 Ptr;

	public:
		FStringPointer() = default;
		explicit FStringPointer(const FString* InPtr) : Ptr(reinterpret_cast<uint64>(InPtr)) {}
		FStringPointer& operator=(const FString* InPtr) { Ptr = reinterpret_cast<uint64>(InPtr); return *this; }

		const FString* operator->() const { return reinterpret_cast<const FString*>(Ptr); }
		operator const FString*() const { return reinterpret_cast<const FString*>(Ptr); }
	};
#else
	using FStringPointer = const FString*;
#endif

	union
	{
		FStringPointer Loose;
		FFixedTagValue Fixed;
		uint64 Bits = 0;
	};

	uint64 IsFixed() const { return Fixed.IsFixed(); }
	FixedTagPrivate::FValueHandle AsFixed() const;
	const FString& AsLoose() const;

public:
	FAssetTagValueRef() = default;
	FAssetTagValueRef(const FAssetTagValueRef&) = default;
	FAssetTagValueRef(FAssetTagValueRef&&) = default;
	explicit FAssetTagValueRef(const FString* Str) : Loose(Str) {}
	FAssetTagValueRef(uint32 StoreIndex, FixedTagPrivate::FValueId ValueId) : Fixed(StoreIndex, ValueId.ToInt()) {}

	FAssetTagValueRef& operator=(const FAssetTagValueRef&) = default;
	FAssetTagValueRef& operator=(FAssetTagValueRef&&) = default;

	bool						IsSet() const { return Bits != 0; }

	FString						AsString() const;
	FName						AsName() const;
	FAssetRegistryExportPath	AsExportPath() const;
	FText						AsText() const;
	bool						TryGetAsText(FText& Out) const; // @return false if value isn't a localized string

	FString						GetValue() const { return AsString(); }

	// Get FTexts as unlocalized complex strings. For internal use only, to make new FAssetDataTagMapSharedView.
	FString						ToLoose() const;

	bool						Equals(FStringView Str) const;

	UE_DEPRECATED(4.27, "Use AsString(), AsName(), AsExportPath() or AsText() instead. ")
	operator FString () const { return AsString(); }
};

inline bool operator==(FAssetTagValueRef A, FStringView B) { return  A.Equals(B); }
inline bool operator!=(FAssetTagValueRef A, FStringView B) { return !A.Equals(B); }
inline bool operator==(FStringView A, FAssetTagValueRef B) { return  B.Equals(A); }
inline bool operator!=(FStringView A, FAssetTagValueRef B) { return !B.Equals(A); }

// These overloads can be removed when the deprecated implicit operator FString is removed
inline bool operator==(FAssetTagValueRef A, const FString& B) { return  A.Equals(B); }
inline bool operator!=(FAssetTagValueRef A, const FString& B) { return !A.Equals(B); }
inline bool operator==(const FString& A, FAssetTagValueRef B) { return  B.Equals(A); }
inline bool operator!=(const FString& A, FAssetTagValueRef B) { return !B.Equals(A); }

using FAssetDataTagMapBase = TSortedMap<FName, FString, FDefaultAllocator, FNameFastLess>;

/** "Loose" FName -> FString that is optionally ref-counted and owned by a FAssetDataTagMapSharedView */
class FAssetDataTagMap : public FAssetDataTagMapBase
{
	mutable FThreadSafeCounter RefCount;
	friend class FAssetDataTagMapSharedView;

public:
	FAssetDataTagMap() = default;
	FAssetDataTagMap(const FAssetDataTagMap& O) : FAssetDataTagMapBase(O) {}
	FAssetDataTagMap(FAssetDataTagMap&& O) : FAssetDataTagMapBase(MoveTemp(O)) {}
	FAssetDataTagMap& operator=(const FAssetDataTagMap& O) { return static_cast<FAssetDataTagMap&>(this->FAssetDataTagMapBase::operator=(O)); }
	FAssetDataTagMap& operator=(FAssetDataTagMap&& O) { return static_cast<FAssetDataTagMap&>(this->FAssetDataTagMapBase::operator=(MoveTemp(O))); }
};

/** Reference-counted handle to a loose FAssetDataTagMap or a fixed / immutable cooked tag map */
class COREUOBJECT_API FAssetDataTagMapSharedView
{
	union
	{
		FixedTagPrivate::FMapHandle Fixed;
		FAssetDataTagMap* Loose;
		uint64 Bits = 0;
	};

	bool IsFixed() const
	{
		return Fixed.IsValid;
	}

	bool IsLoose() const
	{
		return (!Fixed.IsValid) & (Loose != nullptr);
	}

	FAssetTagValueRef FindFixedValue(FName Key) const
	{
		checkSlow(IsFixed());
		const FixedTagPrivate::FValueId* Value = Fixed.FindValue(Key);
		return Value ? FAssetTagValueRef(Fixed.StoreIndex, *Value) : FAssetTagValueRef();
	}

	static TPair<FName, FAssetTagValueRef> MakePair(FixedTagPrivate::FNumberedPair FixedPair, uint32 StoreIndex)
	{
		return MakeTuple(FixedPair.Key, FAssetTagValueRef(StoreIndex, FixedPair.Value));
	}

	static TPair<FName, FAssetTagValueRef> MakePair(const FAssetDataTagMap::ElementType& LoosePair)
	{
		return MakeTuple(LoosePair.Key, FAssetTagValueRef(&LoosePair.Value));
	}

public:
	FAssetDataTagMapSharedView() = default;
	FAssetDataTagMapSharedView(const FAssetDataTagMapSharedView& O);
	FAssetDataTagMapSharedView(FAssetDataTagMapSharedView&& O);
	explicit FAssetDataTagMapSharedView(FixedTagPrivate::FMapHandle InFixed);
	explicit FAssetDataTagMapSharedView(FAssetDataTagMap&& InLoose);

	FAssetDataTagMapSharedView& operator=(const FAssetDataTagMapSharedView&);
	FAssetDataTagMapSharedView& operator=(FAssetDataTagMapSharedView&&);

	~FAssetDataTagMapSharedView();

	using FFindTagResult = FAssetTagValueRef;

	/** Find a value by key and return an option indicating if it was found, and if so, what the value is. */
	FAssetTagValueRef FindTag(FName Tag) const
	{
		if (IsFixed())
		{
			return FindFixedValue(Tag);
		}

		return Loose != nullptr ? FAssetTagValueRef(Loose->Find(Tag)) : FAssetTagValueRef();
	}

	/** Return true if this map contains a specific key value pair. Value comparisons are NOT cases sensitive.*/
	bool ContainsKeyValue(FName Tag, const FString& Value) const
	{
		return FindTag(Tag).Equals(Value);
	}

	UE_DEPRECATED(4.27, "Use FindTag().As[String|Name|Text/ExportPath]() instead, this checks internally. ")
	FString FindChecked(FName Key) const
	{
		return FindTag(Key).AsString();
	}
	
	/** Find a value by key (default value if not found) */
	UE_DEPRECATED(4.27, "Use FindTag() instead. ")
	FString FindRef(FName Key) const
	{
		return FindTag(Key).AsString();
	}

	/** Determine whether a key is present in the map */
	bool Contains(FName Key) const
	{
		return FindTag(Key).IsSet();
	}

	/** Retrieve size of map */
	int32 Num() const
	{
		if (IsFixed())
		{
			return Fixed.Num;
		}

		return Loose != nullptr ? Loose->Num() : 0;
	}

	UE_DEPRECATED(4.27, "Use CopyMap() instead if you really need to make a copy. ")
	FAssetDataTagMap GetMap() const
	{
		return CopyMap();
	}

	/** Copy map contents to a loose FAssetDataTagMap */
	FAssetDataTagMap CopyMap() const;

	template<typename Func>
	void ForEach(Func Fn) const
	{
		if (IsFixed())
		{
			Fixed.ForEachPair([&](FixedTagPrivate::FNumberedPair Pair) { Fn(MakePair(Pair, Fixed.StoreIndex)); });
		}
		else if (Loose != nullptr)
		{
			for (const FAssetDataTagMap::ElementType& Pair : *Loose)
			{
				Fn(MakePair(Pair));
			}
		}
	}

	// Note that FAssetDataTagMap isn't sorted and that order matters
	COREUOBJECT_API friend bool operator==(const FAssetDataTagMapSharedView& A, const FAssetDataTagMap& B);
	COREUOBJECT_API friend bool operator==(const FAssetDataTagMapSharedView& A, const FAssetDataTagMapSharedView& B);

	UE_DEPRECATED(4.27, "Use FMemoryCounter instead. ")
	uint32 GetAllocatedSize() const { return 0; }

	///** Shrinks the contained map */
	void Shrink();

	class TConstIterator
	{
	public:
		TConstIterator(const FAssetDataTagMapSharedView& InView, uint32 InIndex)
			: View(InView)
			, Index(InIndex)
		{}

		TPair<FName, FAssetTagValueRef> operator*() const
		{
			check(View.Bits != 0);
			return View.IsFixed()	? MakePair(View.Fixed.At(Index), View.Fixed.StoreIndex)
									: MakePair((&*View.Loose->begin())[Index]);
		}

		TConstIterator& operator++()
		{
			++Index;
			return *this;
		}

		bool operator!=(TConstIterator Rhs) const { return Index != Rhs.Index; }

	protected:
		const FAssetDataTagMapSharedView& View;
		uint32 Index;
	};

	class TConstIteratorWithEnd : public TConstIterator
	{
	public:
		TConstIteratorWithEnd(const FAssetDataTagMapSharedView& InView, uint32 InBeginIndex, uint32 InEndIndex)
			: TConstIterator(InView, InBeginIndex)
			, EndIndex(InEndIndex)
		{}

		explicit operator bool() const		{ return Index != EndIndex; }
		FName Key() const					{ return operator*().Key; }
		FAssetTagValueRef Value() const		{ return operator*().Value; }

	private:
		const uint32 EndIndex;
	};

	TConstIteratorWithEnd CreateConstIterator() const
	{
		return TConstIteratorWithEnd(*this, 0, Num());
	}

	/** Range for iterator access - DO NOT USE DIRECTLY */
	TConstIterator begin() const
	{
		return TConstIterator(*this, 0);
	}

	/** Range for iterator access - DO NOT USE DIRECTLY */
	TConstIterator end() const
	{
		return TConstIterator(*this, Num());
	}

	/** Helps count deduplicated memory usage */
	class COREUOBJECT_API FMemoryCounter
	{
		TSet<uint32> FixedStoreIndices;
		SIZE_T LooseBytes = 0;
	public:
		void Include(const FAssetDataTagMapSharedView& Tags);
		SIZE_T GetLooseSize() const { return LooseBytes; }
		SIZE_T GetFixedSize() const;		
	};
};

inline bool operator==(const FAssetDataTagMap& A, const FAssetDataTagMapSharedView& B)				{ return B == A; }
inline bool operator!=(const FAssetDataTagMap& A, const FAssetDataTagMapSharedView& B)				{ return !(B == A); }
inline bool operator!=(const FAssetDataTagMapSharedView& A, const FAssetDataTagMap& B)				{ return !(A == B); }
inline bool operator!=(const FAssetDataTagMapSharedView& A, const FAssetDataTagMapSharedView& B)	{ return !(A == B); }