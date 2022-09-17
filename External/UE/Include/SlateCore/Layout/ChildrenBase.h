// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlotBase.h"
#include "SlateGlobals.h"
#include "Debugging/SlateDebugging.h"
#include "Types/ReflectionMetadata.h"

class SWidget;
class FSlotBase;


/**
 * FChildren is an interface that must be implemented by all child containers.
 * It allows iteration over a list of any Widget's children regardless of how
 * the underlying Widget happens to store its children.
 * 
 * FChildren is intended to be returned by the GetChildren() method.
 */
PRAGMA_DISABLE_DEPRECATION_WARNINGS
class SLATECORE_API FChildren
{
public:
	FChildren(SWidget* InOwner)
		: Owner(InOwner)
		, Name("Children")
	{
		check(InOwner);
	}
	FChildren(SWidget* InOwner, FName InName)
		: Owner(InOwner)
		, Name(InName)
	{
		check(InOwner);
		check(!Name.IsNone());
	}

	FChildren(std::nullptr_t) = delete;

	//~ Prevents allocation of FChilren. It creates confusion between FSlot and FChildren
	void* operator new (size_t) = delete;
	void* operator new[](size_t) = delete;

	/** @return the number of children */
	virtual int32 Num() const = 0;
	/** @return pointer to the Widget at the specified Index. */
	virtual TSharedRef<SWidget> GetChildAt( int32 Index ) = 0;
	/** @return const pointer to the Widget at the specified Index. */
	virtual TSharedRef<const SWidget> GetChildAt( int32 Index ) const = 0;

	/** @return the SWidget that own the FChildren */
	SWidget& GetOwner() const { return *Owner; }

	/** Applies the predicate to all the widgets contained by the FChildren. */
	template<typename Predicate>
	void ForEachWidget(Predicate Pred)
	{
#if WITH_SLATE_DEBUGGING 
		TGuardValue<bool> IteratingGuard(bIsIteratingChildren, true);
#endif

		for (int32 Index = 0; Index < Num(); ++Index)
		{
			TSharedRef<SWidget> Ref = GetChildAt(Index);
			Pred(Ref.Get());
		}
	}

	/** Applies the predicate to all the widgets contained by the FChildren. */
	template<typename Predicate>
	void ForEachWidget(Predicate Pred) const
	{
#if WITH_SLATE_DEBUGGING 
		TGuardValue<bool> IteratingGuard(bIsIteratingChildren, true);
#endif

		for (int32 Index = 0; Index < Num(); ++Index)
		{
			TSharedRef<const SWidget> Ref = GetChildAt(Index);
			Pred(Ref.Get());
		}
	}

	/** @return the number of slot the children has. */
	virtual int32 NumSlot() const
	{
		return Num();
	}

	/** @return the const reference to the slot at the specified Index */
	virtual const FSlotBase& GetSlotAt(int32 ChildIndex) const = 0;

	/** @returns if the Children can support slot with SlateSlotAttribute. */
	virtual bool SupportSlotWithSlateAttribute() const
	{
		return false;
	}

	/** Option to give a name to Children to SlotAttribute purposes or for debugging. */
	FName GetName() const
	{
		return Name;
	}

protected:
	friend class FCombinedChildren;
	template<typename T>
	friend class TOneDynamicChild;

	enum ECopyConstruct { CopyConstruct };
	enum ERefConstruct { ReferenceConstruct };

	struct FWidgetRef
	{
	private:
		TOptional<TSharedRef<SWidget>> WidgetCopy;
		SWidget& WidgetReference;

	public:
		FWidgetRef(ECopyConstruct, TSharedRef<SWidget> InWidgetCopy)
			: WidgetCopy(MoveTemp(InWidgetCopy))
			, WidgetReference(WidgetCopy.GetValue().Get())
		{}
		FWidgetRef(ERefConstruct, SWidget& InWidgetRef)
			: WidgetCopy()
			, WidgetReference(InWidgetRef)
		{}
		FWidgetRef(const FWidgetRef& Other)
			: WidgetCopy(Other.WidgetCopy)
			, WidgetReference(WidgetCopy.IsSet() ? WidgetCopy.GetValue().Get() : Other.WidgetReference)
		{}
		FWidgetRef(FWidgetRef&& Other)
			: WidgetCopy(MoveTemp(Other.WidgetCopy))
			, WidgetReference(WidgetCopy.IsSet() ? WidgetCopy.GetValue().Get() : Other.WidgetReference)
		{}
		FWidgetRef& operator=(const FWidgetRef&) = delete;
		FWidgetRef& operator=(FWidgetRef&&) = delete;
		SWidget& GetWidget() const
		{
			return WidgetReference;
		}
	};

	struct FConstWidgetRef
	{
	private:
		TOptional<TSharedRef<const SWidget>> WidgetCopy;
		const SWidget& WidgetReference;

	public:
		FConstWidgetRef(ECopyConstruct, TSharedRef<const SWidget> InWidgetCopy)
			: WidgetCopy(MoveTemp(InWidgetCopy))
			, WidgetReference(WidgetCopy.GetValue().Get())
		{}
		FConstWidgetRef(ERefConstruct, const SWidget& InWidgetRef)
			: WidgetCopy()
			, WidgetReference(InWidgetRef)
		{}
		FConstWidgetRef(const FConstWidgetRef& Other)
			: WidgetCopy(Other.WidgetCopy)
			, WidgetReference(WidgetCopy.IsSet() ? WidgetCopy.GetValue().Get() : Other.WidgetReference)
		{}
		FConstWidgetRef(FConstWidgetRef&& Other)
			: WidgetCopy(MoveTemp(Other.WidgetCopy))
			, WidgetReference(WidgetCopy.IsSet() ? WidgetCopy.GetValue().Get() : Other.WidgetReference)
		{}
		FConstWidgetRef& operator=(const FConstWidgetRef&) = delete;
		FConstWidgetRef& operator=(FConstWidgetRef&&) = delete;
		const SWidget& GetWidget() const
		{
			return WidgetReference;
		}
	};

	/** @return ref to the Widget at the specified Index. */
	virtual FWidgetRef GetChildRefAt(int32 Index) = 0;
	/** @return ref to the Widget at the specified Index. */
	virtual FConstWidgetRef GetChildRefAt(int32 Index) const = 0;


protected:
#if !WITH_SLATE_DEBUGGING 
	virtual ~FChildren() = default;
#else
	virtual ~FChildren()
	{
		UE_CLOG(bIsIteratingChildren, LogSlate, Error,
			TEXT("Destroying widget while iterating children! Owner: %s [%s]"),
			*FReflectionMetaData::GetWidgetDebugInfo(Owner),
			*Name.ToString());
	}
#endif // !WITH_SLATE_DEBUGGING

protected:
	UE_DEPRECATED(5.0, "Direct access to Owner is now deprecated. Use the getter.")
	SWidget* Owner;

private:
	FName Name;

#if WITH_SLATE_DEBUGGING
	mutable bool bIsIteratingChildren = false;
#endif // WITH_SLATE_DEBUGGING
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS
