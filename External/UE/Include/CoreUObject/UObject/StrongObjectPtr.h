// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/GCObject.h"
#include "Templates/EnableIf.h"
#include "Templates/PointerIsConvertibleFromTo.h"
#include "Templates/UniquePtr.h"

namespace UEStrongObjectPtr_Private
{
	struct FInternalReferenceCollectorReferencerNameProvider
	{
		static FString GetReferencerName()
		{
			return TEXT("UEStrongObjectPtr_Private::TInternalReferenceCollector");
		}
	};

	template <typename ReferencerNameProvider = FInternalReferenceCollectorReferencerNameProvider>
	class TInternalReferenceCollector : public FGCObject
	{
	public:
		explicit TInternalReferenceCollector(const volatile UObject* InObject)
			: Object(InObject)
		{
			checkf(IsInGameThread(), TEXT("TStrongObjectPtr can only be created on the game thread otherwise it may introduce threading issues with Grbage Collector"));
		}

		virtual ~TInternalReferenceCollector()
		{
			check(IsInGameThread() || IsInGarbageCollectorThread());
		}

		bool IsValid() const
		{
			return Object != nullptr;
		}

		template <typename UObjectType>
		FORCEINLINE UObjectType* GetAs() const
		{
			return (UObjectType*)Object;
		}

		FORCEINLINE void Set(const volatile UObject* InObject)
		{
			Object = InObject;
		}

		//~ FGCObject interface
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Collector.AddReferencedObject(Object);
		}

		virtual FString GetReferencerName() const override
		{
			return ReferencerNameProvider::GetReferencerName();
		}

	private:
		const volatile UObject* Object;
	};
}

/**
 * Specific implementation of FGCObject that prevents a single UObject-based pointer from being GC'd while this guard is in scope.
 * @note This is the "full-fat" version of FGCObjectScopeGuard which uses a heap-allocated FGCObject so *can* safely be used with containers that treat types as trivially relocatable.
 */
template <typename ObjectType, typename ReferencerNameProvider = UEStrongObjectPtr_Private::FInternalReferenceCollectorReferencerNameProvider>
class TStrongObjectPtr
{
public:
	TStrongObjectPtr(TStrongObjectPtr&& InOther) = default;
	TStrongObjectPtr& operator=(TStrongObjectPtr&& InOther) = default;
	~TStrongObjectPtr() = default;

	FORCEINLINE_DEBUGGABLE TStrongObjectPtr(TYPE_OF_NULLPTR = nullptr)
	{
		static_assert(TPointerIsConvertibleFromTo<ObjectType, const volatile UObject>::Value, "TStrongObjectPtr can only be constructed with UObject types");
	}

	FORCEINLINE_DEBUGGABLE explicit TStrongObjectPtr(ObjectType* InObject)
	{
		static_assert(TPointerIsConvertibleFromTo<ObjectType, const volatile UObject>::Value, "TStrongObjectPtr can only be constructed with UObject types");
		Reset(InObject);
	}

	FORCEINLINE_DEBUGGABLE TStrongObjectPtr(const TStrongObjectPtr& InOther)
	{
		Reset(InOther.Get());
	}

	template <
		typename OtherObjectType,
		typename OtherReferencerNameProvider,
		typename = decltype(ImplicitConv<ObjectType*>((OtherObjectType*)nullptr))
	>
	FORCEINLINE_DEBUGGABLE TStrongObjectPtr(const TStrongObjectPtr<OtherObjectType, OtherReferencerNameProvider>& InOther)
	{
		Reset(InOther.Get());
	}

	FORCEINLINE_DEBUGGABLE TStrongObjectPtr& operator=(const TStrongObjectPtr& InOther)
	{
		Reset(InOther.Get());
		return *this;
	}

	template <
		typename OtherObjectType,
		typename OtherReferencerNameProvider,
		typename = decltype(ImplicitConv<ObjectType*>((OtherObjectType*)nullptr))
	>
	FORCEINLINE_DEBUGGABLE TStrongObjectPtr& operator=(const TStrongObjectPtr<OtherObjectType, OtherReferencerNameProvider>& InOther)
	{
		Reset(InOther.Get());
		return *this;
	}

	FORCEINLINE_DEBUGGABLE ObjectType& operator*() const
	{
		check(IsValid());
		return *Get();
	}

	FORCEINLINE_DEBUGGABLE ObjectType* operator->() const
	{
		check(IsValid());
		return Get();
	}

	FORCEINLINE_DEBUGGABLE bool IsValid() const
	{
		return ReferenceCollector && ReferenceCollector->IsValid();
	}

	FORCEINLINE_DEBUGGABLE explicit operator bool() const
	{
		return IsValid();
	}

	FORCEINLINE_DEBUGGABLE ObjectType* Get() const
	{
		return ReferenceCollector
			? ReferenceCollector->template GetAs<ObjectType>()
			: nullptr;
	}

	FORCEINLINE_DEBUGGABLE void Reset(ObjectType* InNewObject = nullptr)
	{
		if (InNewObject)
		{
			if (ReferenceCollector)
			{
				// Update the referenced object
				ReferenceCollector->Set(InNewObject);
			}
			else
			{
				// Lazily create the ReferenceCollector to allow TStrongObjectPtr to be used during static initialization
				ReferenceCollector = MakeUnique< UEStrongObjectPtr_Private::TInternalReferenceCollector<ReferencerNameProvider> >(InNewObject);
			}
		}
		else
		{
			// Destroy the ReferenceCollector immediately to allow TStrongObjectPtr to be manually cleared prior to 
			// static deinitialization, as not all platforms use the main thread as their game thread
			ReferenceCollector.Reset();
		}
	}

	FORCEINLINE_DEBUGGABLE friend uint32 GetTypeHash(const TStrongObjectPtr& InStrongObjectPtr)
	{
		return GetTypeHash(InStrongObjectPtr.Get());
	}

private:
	TUniquePtr< UEStrongObjectPtr_Private::TInternalReferenceCollector<ReferencerNameProvider> > ReferenceCollector;
};

template <typename LHSObjectType, typename LHSReferencerNameProvider, typename RHSObjectType, typename RHSReferencerNameProvider>
FORCEINLINE bool operator==(const TStrongObjectPtr<LHSObjectType, LHSReferencerNameProvider>& InLHS, const TStrongObjectPtr<RHSObjectType, RHSReferencerNameProvider>& InRHS)
{
	return InLHS.Get() == InRHS.Get();
}

template <typename LHSObjectType, typename LHSReferencerNameProvider, typename RHSObjectType, typename RHSReferencerNameProvider>
FORCEINLINE bool operator!=(const TStrongObjectPtr<LHSObjectType, LHSReferencerNameProvider>& InLHS, const TStrongObjectPtr<RHSObjectType, RHSReferencerNameProvider>& InRHS)
{
	return InLHS.Get() != InRHS.Get();
}
