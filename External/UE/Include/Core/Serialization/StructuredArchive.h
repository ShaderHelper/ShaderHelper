// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Serialization/StructuredArchiveFormatter.h"
#include "Formatters/BinaryArchiveFormatter.h"
#include "Misc/Optional.h"
#include "Concepts/Insertable.h"
#include "Templates/Models.h"
#include "Containers/Array.h"
#include "Serialization/ArchiveProxy.h"
#include "Templates/UniqueObj.h"

/**
 * DO_STRUCTURED_ARCHIVE_CONTAINER_CHECKS - if set, checks that nested container types are serialized correctly.
 */
#ifndef DO_STRUCTURED_ARCHIVE_CONTAINER_CHECKS
	#if DO_GUARD_SLOW
		#define DO_STRUCTURED_ARCHIVE_CONTAINER_CHECKS 1
	#else
		#define DO_STRUCTURED_ARCHIVE_CONTAINER_CHECKS 0
	#endif
#endif

/**
 * DO_STRUCTURED_ARCHIVE_UNIQUE_FIELD_NAME_CHECKS - if set, checks that field names are unique within a container.  Requires DO_STRUCTURED_ARCHIVE_CONTAINER_CHECKS.
 */
#ifndef DO_STRUCTURED_ARCHIVE_UNIQUE_FIELD_NAME_CHECKS
	#define DO_STRUCTURED_ARCHIVE_UNIQUE_FIELD_NAME_CHECKS 0
#endif

/**
 * Class to contain a named value for serialization. Intended to be created as a temporary and passed to object serialization methods.
 */
template<typename T> struct TNamedValue
{
	FArchiveFieldName Name;
	T& Value;

	FORCEINLINE TNamedValue(FArchiveFieldName InName, T& InValue)
		: Name(InName)
		, Value(InValue)
	{
	}
};

/**
 * Class to contain a named attribute for serialization. Intended to be created as a temporary and passed to object serialization methods.
 */
template<typename T> struct TNamedAttribute
{
	FArchiveFieldName Name;
	T& Value;

	explicit FORCEINLINE TNamedAttribute(FArchiveFieldName InName, T& InValue)
		: Name(InName)
		, Value(InValue)
	{
	}
};

/**
 * Class to contain a named attribute for serialization, with a default. Intended to be created as a temporary and passed to object
 * serialization methods, which can choose not to serialize the attribute if it matches the default.
 */
template<typename T> struct TOptionalNamedAttribute
{
	FArchiveFieldName Name;
	T& Value;
	const T& Default;

	explicit FORCEINLINE TOptionalNamedAttribute(FArchiveFieldName InName, T& InValue, const T& InDefault)
		: Name(InName)
		, Value(InValue)
		, Default(InDefault)
	{
	}
};

/**
 * Helper function to construct a TNamedValue, deducing the value type.
 */
template<typename T> FORCEINLINE TNamedValue<T> MakeNamedValue(FArchiveFieldName Name, T& Value)
{
	return TNamedValue<T>(Name, Value);
}

/**
 * Helper function to construct a TNamedAttribute, deducing the value type.
 */
template<typename T> FORCEINLINE TNamedAttribute<T> MakeNamedAttribute(FArchiveFieldName Name, T& Value)
{
	return TNamedAttribute<T>(Name, Value);
}

/**
 * Helper function to construct a TOptionalNamedAttribute, deducing the value type.
 */
template<typename T> FORCEINLINE TOptionalNamedAttribute<T> MakeOptionalNamedAttribute(FArchiveFieldName Name, T& Value, const typename TIdentity<T>::Type& Default)
{
	return TOptionalNamedAttribute<T>(Name, Value, Default);
}

/** Construct a TNamedValue given an ANSI string and value reference. */
#define SA_VALUE(Name, Value) MakeNamedValue(FArchiveFieldName(Name), Value)

/** Construct a TNamedAttribute given an ANSI string and value reference. */
#define SA_ATTRIBUTE(Name, Value) MakeNamedAttribute(FArchiveFieldName(Name), Value)

/** Construct a TOptionalNamedAttribute given an ANSI string and value reference. */
#define SA_OPTIONAL_ATTRIBUTE(Name, Value, Default) MakeOptionalNamedAttribute(FArchiveFieldName(Name), Value, Default)

/** Typedef for which formatter type to support */
#if WITH_TEXT_ARCHIVE_SUPPORT
	typedef FStructuredArchiveFormatter FArchiveFormatterType;
#else
	typedef FBinaryArchiveFormatter FArchiveFormatterType;
#endif

class FStructuredArchive;
class FStructuredArchiveChildReader;
class FStructuredArchiveSlot;
class FStructuredArchiveRecord;
class FStructuredArchiveArray;
class FStructuredArchiveStream;
class FStructuredArchiveMap;

namespace StructuredArchive_Private
{
	struct FElementId
	{
		FElementId() = default;

		explicit FElementId(uint32 InId)
			: Id(InId)
		{
		}

		bool IsValid() const
		{
			return Id != 0;
		}

		void Reset()
		{
			Id = 0;
		}

		bool operator==(const FElementId& Rhs) const
		{
			return Id == Rhs.Id;
		}

		bool operator!=(const FElementId& Rhs) const
		{
			return Id != Rhs.Id;
		}

	private:
		uint32 Id = 0;
	};

	// Represents a position of a slot within the hierarchy.
	class CORE_API FSlotPosition
	{
		friend class FStructuredArchive;

	public:
		int32 Depth;
		FElementId ElementId;

		FORCEINLINE explicit FSlotPosition(int32 InDepth, FElementId InElementId)
			: Depth(InDepth)
			, ElementId(InElementId)
		{
		}
	};

	// The base class of all slot types
	class CORE_API FSlotBase
#if WITH_TEXT_ARCHIVE_SUPPORT
		: protected FSlotPosition
#endif
	{
		friend class FStructuredArchive;

	public:
		FArchive& GetUnderlyingArchive() const;

		const FArchiveState& GetArchiveState() const;

	protected:
		FStructuredArchive& Ar;

#if WITH_TEXT_ARCHIVE_SUPPORT
		FORCEINLINE explicit FSlotBase(FStructuredArchive& InAr, int32 InDepth, FElementId InElementId)
			: FSlotPosition(InDepth, InElementId)
			, Ar(InAr)
		{
		}
#else
		FORCEINLINE FSlotBase(FStructuredArchive& InAr)
			: Ar(InAr)
		{
		}
#endif
	};

	enum class EElementType : unsigned char
	{
		Root,
		Record,
		Array,
		Stream,
		Map,
		AttributedValue,
	};

	enum class EEnteringAttributeState
	{
		NotEnteringAttribute,
		EnteringAttribute,
	};
}

/**
 * Contains a value in the archive; either a field or array/map element. A slot does not know it's name or location,
 * and can merely have a value serialized into it. That value may be a literal (eg. int, float) or compound object
 * (eg. object, array, map).
 */
class CORE_API FStructuredArchiveSlot final : public StructuredArchive_Private::FSlotBase
{
public:
	FStructuredArchiveRecord EnterRecord();
	FStructuredArchiveRecord EnterRecord_TextOnly(TArray<FString>& OutFieldNames);
	FStructuredArchiveArray EnterArray(int32& Num);
	FStructuredArchiveStream EnterStream();
	FStructuredArchiveStream EnterStream_TextOnly(int32& OutNumElements);
	FStructuredArchiveMap EnterMap(int32& Num);
	FStructuredArchiveSlot EnterAttribute(FArchiveFieldName AttributeName);
	TOptional<FStructuredArchiveSlot> TryEnterAttribute(FArchiveFieldName AttributeName, bool bEnterWhenWriting);

	// We don't support chaining writes to a single slot, so this returns void.
	void operator << (uint8& Value);
	void operator << (uint16& Value);
	void operator << (uint32& Value);
	void operator << (uint64& Value);
	void operator << (int8& Value);
	void operator << (int16& Value);
	void operator << (int32& Value);
	void operator << (int64& Value);
	void operator << (float& Value);
	void operator << (double& Value);
	void operator << (bool& Value);
	void operator << (FString& Value);
	void operator << (FName& Value);
	void operator << (UObject*& Value);
	void operator << (FText& Value);
	void operator << (FWeakObjectPtr& Value);
	void operator << (FSoftObjectPtr& Value);
	void operator << (FSoftObjectPath& Value);
	void operator << (FLazyObjectPtr& Value);
	void operator << (FObjectPtr& Value);

	template <typename T>
	FORCEINLINE void operator<<(TEnumAsByte<T>& Value)
	{
		uint8 Tmp = (uint8)Value.GetValue();
		*this << Tmp;
		Value = (T)Tmp;
	}

	template <
		typename EnumType,
		typename = typename TEnableIf<TIsEnumClass<EnumType>::Value>::Type
	>
	FORCEINLINE void operator<<(EnumType& Value)
	{
		*this << (__underlying_type(EnumType)&)Value;
	}

	template <typename T>
	FORCEINLINE void operator<<(TNamedAttribute<T> Item)
	{
		EnterAttribute(Item.Name) << Item.Value;
	}

	template <typename T>
	FORCEINLINE void operator<<(TOptionalNamedAttribute<T> Item)
	{
		if (TOptional<FStructuredArchiveSlot> Attribute = TryEnterAttribute(Item.Name, Item.Value != Item.Default))
		{
			Attribute.GetValue() << Item.Value;
		}
		else
		{
			Item.Value = Item.Default;
		}
	}

	void Serialize(TArray<uint8>& Data);
	void Serialize(void* Data, uint64 DataSize);

	bool IsFilled() const;

private:
	friend FStructuredArchive;
	friend FStructuredArchiveChildReader;
	friend FStructuredArchiveSlot;
	friend FStructuredArchiveRecord;
	friend FStructuredArchiveArray;
	friend FStructuredArchiveStream;
	friend FStructuredArchiveMap;

	using StructuredArchive_Private::FSlotBase::FSlotBase;
};

/**
 * Represents a record in the structured archive. An object contains slots that are identified by FArchiveName,
 * which may be compiled out with binary-only archives.
 */
class CORE_API FStructuredArchiveRecord final : public StructuredArchive_Private::FSlotBase
{
public:
	FStructuredArchiveSlot EnterField(FArchiveFieldName Name);
	FStructuredArchiveSlot EnterField_TextOnly(FArchiveFieldName Name, EArchiveValueType& OutType);
	FStructuredArchiveRecord EnterRecord(FArchiveFieldName Name);
	FStructuredArchiveRecord EnterRecord_TextOnly(FArchiveFieldName Name, TArray<FString>& OutFieldNames);
	FStructuredArchiveArray EnterArray(FArchiveFieldName Name, int32& Num);
	FStructuredArchiveStream EnterStream(FArchiveFieldName Name);
	FStructuredArchiveStream EnterStream_TextOnly(FArchiveFieldName Name, int32& OutNumElements);
	FStructuredArchiveMap EnterMap(FArchiveFieldName Name, int32& Num);

	TOptional<FStructuredArchiveSlot> TryEnterField(FArchiveFieldName Name, bool bEnterForSaving);

	template<typename T> FORCEINLINE FStructuredArchiveRecord& operator<<(TNamedValue<T> Item)
	{
		EnterField(Item.Name) << Item.Value;
		return *this;
	}

private:
	friend FStructuredArchive;
	friend FStructuredArchiveSlot;

	using StructuredArchive_Private::FSlotBase::FSlotBase;
};

/**
 * Represents an array in the structured archive. An object contains slots that are identified by a FArchiveFieldName,
 * which may be compiled out with binary-only archives.
 */
class CORE_API FStructuredArchiveArray final : public StructuredArchive_Private::FSlotBase
{
public:
	FStructuredArchiveSlot EnterElement();
	FStructuredArchiveSlot EnterElement_TextOnly(EArchiveValueType& OutType);

	template<typename T> FORCEINLINE FStructuredArchiveArray& operator<<(T& Item)
	{
		EnterElement() << Item;
		return *this;
	}

private:
	friend FStructuredArchive;
	friend FStructuredArchiveSlot;

	using StructuredArchive_Private::FSlotBase::FSlotBase;
};

/**
 * Represents an unsized sequence of slots in the structured archive (similar to an array, but without a known size).
 */
class CORE_API FStructuredArchiveStream final : public StructuredArchive_Private::FSlotBase
{
public:
	FStructuredArchiveSlot EnterElement();
	FStructuredArchiveSlot EnterElement_TextOnly(EArchiveValueType& OutType);

	template<typename T> FORCEINLINE FStructuredArchiveStream& operator<<(T& Item)
	{
		EnterElement() << Item;
		return *this;
	}

private:
	friend FStructuredArchive;
	friend FStructuredArchiveSlot;

	using StructuredArchive_Private::FSlotBase::FSlotBase;
};

/**
 * Represents a map in the structured archive. A map is similar to a record, but keys can be read back out from an archive.
 * (This is an important distinction for binary archives).
 */
class CORE_API FStructuredArchiveMap final : public StructuredArchive_Private::FSlotBase
{
public:
	FStructuredArchiveSlot EnterElement(FString& Name);
	FStructuredArchiveSlot EnterElement_TextOnly(FString& Name, EArchiveValueType& OutType);

private:
	friend FStructuredArchive;
	friend FStructuredArchiveSlot;

	using StructuredArchive_Private::FSlotBase::FSlotBase;
};

/**
 * Manages the state of an underlying FStructuredArchiveFormatter, and provides a consistent API for reading and writing to a structured archive.
 * 
 * Both reading and writing to the archive are *forward only* from an interface point of view. There is no point at which it is possible to 
 * require seeking.
 */
class CORE_API FStructuredArchive
{
	friend FStructuredArchiveSlot;
	friend FStructuredArchiveRecord;
	friend FStructuredArchiveArray;
	friend FStructuredArchiveStream;
	friend FStructuredArchiveMap;

public:
	using FSlot   = FStructuredArchiveSlot;
	using FRecord = FStructuredArchiveRecord;
	using FArray  = FStructuredArchiveArray;
	using FStream = FStructuredArchiveStream;
	using FMap    = FStructuredArchiveMap;

	/**
	 * Constructor.
	 *
	 * @param InFormatter Formatter for the archive data
	 */
	FStructuredArchive(FArchiveFormatterType& InFormatter);
	
	/**
	 * Default destructor. Closes the archive.
	 */
	~FStructuredArchive();

	/**
	 * Start writing to the archive, and gets an interface to the root slot.
	 */
	FStructuredArchiveSlot Open();

	/**
	 * Flushes any remaining scope to the underlying formatter and closes the archive.
	 */
	void Close();

	/**
	 * Gets the serialization context from the underlying archive.
	 */
	FORCEINLINE FArchive& GetUnderlyingArchive() const
	{
		return Formatter.GetUnderlyingArchive();
	}

	/**
	 * Gets the archiving state.
	 */
	FORCEINLINE const FArchiveState& GetArchiveState() const
	{
		return GetUnderlyingArchive().GetArchiveState();
	}

	FStructuredArchive(const FStructuredArchive&) = delete;
	FStructuredArchive& operator=(const FStructuredArchive&) = delete;

private:

	friend class FStructuredArchiveChildReader;

	/**
	* Reference to the formatter that actually writes out the data to the underlying archive
	*/
	FArchiveFormatterType& Formatter;

#if WITH_TEXT_ARCHIVE_SUPPORT
	/**
	 * Whether the formatter requires structural metadata. This allows optimizing the path for binary archives in editor builds.
	 */
	const bool bRequiresStructuralMetadata;

	struct FElement
	{
		StructuredArchive_Private::FElementId Id;
		StructuredArchive_Private::EElementType Type;

		FElement(StructuredArchive_Private::FElementId InId, StructuredArchive_Private::EElementType InType)
			: Id(InId)
			, Type(InType)
		{
		}
	};

	struct FIdGenerator
	{
		StructuredArchive_Private::FElementId Generate()
		{
			return StructuredArchive_Private::FElementId(NextId++);
		}

	private:
		uint32 NextId = 1;
	};

	/**
	 * The next element id to be assigned
	 */
	FIdGenerator ElementIdGenerator;

	/**
	 * The ID of the root element.
	 */
	StructuredArchive_Private::FElementId RootElementId;

	/**
	 * The element ID assigned for the current slot. Slots are transient, and only exist as placeholders until something is written into them. This is reset to 0 when something is created in a slot, and the created item can assume the element id.
	 */
	StructuredArchive_Private::FElementId CurrentSlotElementId;

	/**
	 * Tracks the current stack of objects being written. Used by SetScope() to ensure that scopes are always closed correctly in the underlying formatter,
	 * and to make sure that the archive is always written in a forwards-only way (ie. writing to an element id that is not in scope will assert)
	 */
	TArray<FElement, TNonRelocatableInlineAllocator<32>> CurrentScope;

#if DO_STRUCTURED_ARCHIVE_CONTAINER_CHECKS
	struct FContainer;

	/**
	 * For arrays and maps, stores the loop counter and size of the container. Also stores key names for records and maps in builds with DO_STRUCTURED_ARCHIVE_CONTAINER_CHECKS enabled.
	 */
	TArray<TUniqueObj<FContainer>> CurrentContainer;
#endif

	/**
	 * Whether or not we've just entered an attribute
	 */
	StructuredArchive_Private::EEnteringAttributeState CurrentEnteringAttributeState = StructuredArchive_Private::EEnteringAttributeState::NotEnteringAttribute;

	/**
	 * Enters the current slot for serializing a value. Asserts if the archive is not in a state about to write to an empty-slot.
	 */
	void EnterSlot(StructuredArchive_Private::FSlotPosition Slot, bool bEnteringAttributedValue = false);

	/**
	 * Enters the current slot, adding an element onto the stack. Asserts if the archive is not in a state about to write to an empty-slot.
	 *
	 * @return  The depth of the newly-entered slot.
	 */
	int32 EnterSlotAsType(StructuredArchive_Private::FSlotPosition Slot, StructuredArchive_Private::EElementType ElementType);

	/**
	 * Leaves slot at the top of the current scope
	 */
	void LeaveSlot();

	/**
	 * Switches to the scope for the given slot.
	 */
	void SetScope(StructuredArchive_Private::FSlotPosition Slot);
#endif
};

FORCEINLINE FArchive& StructuredArchive_Private::FSlotBase::GetUnderlyingArchive() const
{
	return Ar.GetUnderlyingArchive();
}

FORCEINLINE const FArchiveState& StructuredArchive_Private::FSlotBase::GetArchiveState() const
{
	return Ar.GetArchiveState();
}

FORCEINLINE bool FStructuredArchiveSlot::IsFilled() const
{
#if WITH_TEXT_ARCHIVE_SUPPORT
	return Ar.CurrentSlotElementId != ElementId;
#else
	return true;
#endif
}

template <typename T>
FORCEINLINE_DEBUGGABLE void operator<<(FStructuredArchiveSlot Slot, TArray<T>& InArray)
{
	int32 NumElements = InArray.Num();
	FStructuredArchiveArray Array = Slot.EnterArray(NumElements);

	if (Slot.GetArchiveState().IsLoading())
	{
		InArray.SetNum(NumElements);
	}

	for (int32 ElementIndex = 0; ElementIndex < NumElements; ++ElementIndex)
	{
		FStructuredArchiveSlot ElementSlot = Array.EnterElement();
		ElementSlot << InArray[ElementIndex];
	}
}

template <>
FORCEINLINE_DEBUGGABLE void operator<<(FStructuredArchiveSlot Slot, TArray<uint8>& InArray)
{
	Slot.Serialize(InArray);
}

/**
 * FStructuredArchiveChildReader
 *
 * Utility class for easily creating a structured archive that covers the data hierarchy underneath
 * the given slot
 *
 * Allows serialization code to get an archive instance for the current location, so that it can return to it
 * later on after the master archive has potentially moved on into a different location in the file.
 */
class CORE_API FStructuredArchiveChildReader
{
public:

	FStructuredArchiveChildReader(FStructuredArchiveSlot InSlot);
	~FStructuredArchiveChildReader();

	FORCEINLINE FStructuredArchiveSlot GetRoot() { return Root.GetValue(); }

private:

	FStructuredArchiveFormatter* OwnedFormatter;
	FStructuredArchive* Archive;
	TOptional<FStructuredArchiveSlot> Root;
};

class CORE_API FStructuredArchiveFromArchive
{
	UE_NONCOPYABLE(FStructuredArchiveFromArchive)

	static constexpr uint32 ImplSize      = 400;
	static constexpr uint32 ImplAlignment = 8;

	struct FImpl;

public:
	explicit FStructuredArchiveFromArchive(FArchive& Ar);
	~FStructuredArchiveFromArchive();

	FStructuredArchiveSlot GetSlot();

private:
	// Implmented as a pimpl in order to reduce dependencies, but an inline one to avoid heap allocations
	alignas(ImplAlignment) uint8 ImplStorage[ImplSize];
};

#if WITH_TEXT_ARCHIVE_SUPPORT

class CORE_API FArchiveFromStructuredArchiveImpl : public FArchiveProxy
{
	UE_NONCOPYABLE(FArchiveFromStructuredArchiveImpl)

	struct FImpl;

public:
	explicit FArchiveFromStructuredArchiveImpl(FStructuredArchiveSlot Slot);
	virtual ~FArchiveFromStructuredArchiveImpl();

	virtual void Flush() override;
	virtual bool Close() override;

	virtual int64 Tell() override;
	virtual int64 TotalSize() override;
	virtual void Seek(int64 InPos) override;
	virtual bool AtEnd() override;

	using FArchive::operator<<; // For visibility of the overloads we don't override

	//~ Begin FArchive Interface
	virtual FArchive& operator<<(class FName& Value) override;
	virtual FArchive& operator<<(class UObject*& Value) override;
	virtual FArchive& operator<<(class FText& Value) override;
	//~ End FArchive Interface

	virtual void Serialize(void* V, int64 Length) override;

	virtual FArchive* GetCacheableArchive() override;

	bool ContainsData() const;

protected:
	virtual bool Finalize(FStructuredArchiveRecord Record);
	void OpenArchive();

private:
	void Commit();

	// Implmented as a pimpl in order to reduce dependencies
	TUniqueObj<FImpl> Pimpl;
};

class FArchiveFromStructuredArchive
{
public:
	explicit FArchiveFromStructuredArchive(FStructuredArchiveSlot InSlot)
		: Impl(InSlot)
	{
	}

	      FArchive& GetArchive()       { return Impl; }
	const FArchive& GetArchive() const { return Impl; }

	void Close() { Impl.Close(); }

private:
	FArchiveFromStructuredArchiveImpl Impl;
};

#else

class FArchiveFromStructuredArchive
{
public:
	explicit FArchiveFromStructuredArchive(FStructuredArchiveSlot InSlot)
		: Ar(InSlot.GetUnderlyingArchive())
	{
	}

	      FArchive& GetArchive()       { return Ar; }
	const FArchive& GetArchive() const { return Ar; }

	void Close() {}

private:
	FArchive& Ar;
};

#endif

/**
 * Adapter operator which allows a type to stream to an FArchive when it already supports streaming to an FStructuredArchiveSlot.
 *
 * @param  Ar   The archive to read from or write to.
 * @param  Obj  The object to read or write.
 *
 * @return  A reference to the same archive as Ar.
 */
template <typename T>
typename TEnableIf<
	!TModels<CInsertable<FArchive&>, T>::Value && TModels<CInsertable<FStructuredArchiveSlot>, T>::Value,
	FArchive&
>::Type operator<<(FArchive& Ar, T& Obj)
{
	FStructuredArchiveFromArchive ArAdapt(Ar);
	ArAdapt.GetSlot() << Obj;
	return Ar;
}

/**
 * Adapter operator which allows a type to stream to an FStructuredArchiveSlot when it already supports streaming to an FArchive.
 *
 * @param  Slot  The slot to read from or write to.
 * @param  Obj   The object to read or write.
 */
template <typename T>
typename TEnableIf<
	TModels<CInsertable<FArchive&>, T>::Value &&
	!TModels<CInsertable<FStructuredArchiveSlot>, T>::Value
>::Type operator<<(FStructuredArchiveSlot Slot, T& Obj)
{
#if WITH_TEXT_ARCHIVE_SUPPORT
	FArchiveFromStructuredArchive Adapter(Slot);
	FArchive& Ar = Adapter.GetArchive();
#else
	FArchive& Ar = Slot.GetUnderlyingArchive();
#endif
	Ar << Obj;
#if WITH_TEXT_ARCHIVE_SUPPORT
	Adapter.Close();
#endif
}

#if !WITH_TEXT_ARCHIVE_SUPPORT
	FORCEINLINE FStructuredArchiveChildReader::FStructuredArchiveChildReader(FStructuredArchiveSlot InSlot)
		: OwnedFormatter(nullptr)
		, Archive(nullptr)
	{
		Archive = new FStructuredArchive(InSlot.Ar.Formatter);
		Root.Emplace(Archive->Open());
	}

	FORCEINLINE FStructuredArchiveChildReader::~FStructuredArchiveChildReader()
	{
	}

	//////////// FStructuredArchive ////////////

	FORCEINLINE FStructuredArchive::FStructuredArchive(FArchiveFormatterType& InFormatter)
		: Formatter(InFormatter)
	{
	}

	FORCEINLINE FStructuredArchive::~FStructuredArchive()
	{
	}

	FORCEINLINE FStructuredArchiveSlot FStructuredArchive::Open()
	{
		return FStructuredArchiveSlot(*this);
	}

	FORCEINLINE void FStructuredArchive::Close()
	{
	}

	//////////// FStructuredArchiveSlot ////////////

	FORCEINLINE FStructuredArchiveRecord FStructuredArchiveSlot::EnterRecord()
	{
		return FStructuredArchiveRecord(Ar);
	}

	FORCEINLINE FStructuredArchiveRecord FStructuredArchiveSlot::EnterRecord_TextOnly(TArray<FString>& OutFieldNames)
	{
		Ar.Formatter.EnterRecord_TextOnly(OutFieldNames);
		return FStructuredArchiveRecord(Ar);
	}

	FORCEINLINE FStructuredArchiveArray FStructuredArchiveSlot::EnterArray(int32& Num)
	{
		Ar.Formatter.EnterArray(Num);
		return FStructuredArchiveArray(Ar);
	}

	FORCEINLINE FStructuredArchiveStream FStructuredArchiveSlot::EnterStream()
	{
		return FStructuredArchiveStream(Ar);
	}

	FORCEINLINE FStructuredArchiveStream FStructuredArchiveSlot::EnterStream_TextOnly(int32& OutNumElements)
	{
		Ar.Formatter.EnterStream_TextOnly(OutNumElements);
		return FStructuredArchiveStream(Ar);
	}

	FORCEINLINE FStructuredArchiveMap FStructuredArchiveSlot::EnterMap(int32& Num)
	{
		Ar.Formatter.EnterMap(Num);
		return FStructuredArchiveMap(Ar);
	}

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveSlot::EnterAttribute(FArchiveFieldName FieldName)
	{
		Ar.Formatter.EnterAttribute(FieldName);
		return FStructuredArchiveSlot(Ar);
	}

	FORCEINLINE TOptional<FStructuredArchiveSlot> FStructuredArchiveSlot::TryEnterAttribute(FArchiveFieldName FieldName, bool bEnterWhenWriting)
	{
		if (Ar.Formatter.TryEnterAttribute(FieldName, bEnterWhenWriting))
		{
			return TOptional<FStructuredArchiveSlot>(Ar);
		}
		else
		{
			return TOptional<FStructuredArchiveSlot>();
		}
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (uint8& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (uint16& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (uint32& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (uint64& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (int8& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (int16& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (int32& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (int64& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (float& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (double& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (bool& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (FString& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (FName& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (UObject*& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (FText& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (FWeakObjectPtr& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (FSoftObjectPath& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (FSoftObjectPtr& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (FLazyObjectPtr& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::operator<< (FObjectPtr& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchiveSlot::Serialize(TArray<uint8>& Data)
	{
		Ar.Formatter.Serialize(Data);
	}

	FORCEINLINE void FStructuredArchiveSlot::Serialize(void* Data, uint64 DataSize)
	{
		Ar.Formatter.Serialize(Data, DataSize);
	}

	//////////// FStructuredArchiveRecord ////////////

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveRecord::EnterField(FArchiveFieldName Name)
	{
		return FStructuredArchiveSlot(Ar);
	}

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveRecord::EnterField_TextOnly(FArchiveFieldName Name, EArchiveValueType& OutType)
	{
		Ar.Formatter.EnterField_TextOnly(Name, OutType);
		return FStructuredArchiveSlot(Ar);
	}

	FORCEINLINE FStructuredArchiveRecord FStructuredArchiveRecord::EnterRecord(FArchiveFieldName Name)
	{
		return EnterField(Name).EnterRecord();
	}

	FORCEINLINE FStructuredArchiveRecord FStructuredArchiveRecord::EnterRecord_TextOnly(FArchiveFieldName Name, TArray<FString>& OutFieldNames)
	{
		return EnterField(Name).EnterRecord_TextOnly(OutFieldNames);
	}

	FORCEINLINE FStructuredArchiveArray FStructuredArchiveRecord::EnterArray(FArchiveFieldName Name, int32& Num)
	{
		return EnterField(Name).EnterArray(Num);
	}

	FORCEINLINE FStructuredArchiveStream FStructuredArchiveRecord::EnterStream(FArchiveFieldName Name)
	{
		return EnterField(Name).EnterStream();
	}

	FORCEINLINE FStructuredArchiveMap FStructuredArchiveRecord::EnterMap(FArchiveFieldName Name, int32& Num)
	{
		return EnterField(Name).EnterMap(Num);
	}

	FORCEINLINE TOptional<FStructuredArchiveSlot> FStructuredArchiveRecord::TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting)
	{
		if (Ar.Formatter.TryEnterField(Name, bEnterWhenWriting))
		{
			return TOptional<FStructuredArchiveSlot>(FStructuredArchiveSlot(Ar));
		}
		else
		{
			return TOptional<FStructuredArchiveSlot>();
		}
	}

	//////////// FStructuredArchiveArray ////////////

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveArray::EnterElement()
	{
		return FStructuredArchiveSlot(Ar);
	}

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveArray::EnterElement_TextOnly(EArchiveValueType& OutType)
	{
		Ar.Formatter.EnterArrayElement_TextOnly(OutType);
		return FStructuredArchiveSlot(Ar);
	}

	//////////// FStructuredArchiveStream ////////////

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveStream::EnterElement()
	{
		return FStructuredArchiveSlot(Ar);
	}

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveStream::EnterElement_TextOnly(EArchiveValueType& OutType)
	{
		Ar.Formatter.EnterStreamElement_TextOnly(OutType);
		return FStructuredArchiveSlot(Ar);
	}

	//////////// FStructuredArchiveMap ////////////

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveMap::EnterElement(FString& Name)
	{
		Ar.Formatter.EnterMapElement(Name);
		return FStructuredArchiveSlot(Ar);
	}

	FORCEINLINE FStructuredArchiveSlot FStructuredArchiveMap::EnterElement_TextOnly(FString& Name, EArchiveValueType& OutType)
	{
		Ar.Formatter.EnterMapElement_TextOnly(Name, OutType);
		return FStructuredArchiveSlot(Ar);
	}

#endif

