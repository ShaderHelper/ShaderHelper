// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/Class.h"
#include "Templates/LosesQualifiersFromTo.h"
#include "Traits/IsVoidType.h"

class AActor;
class APawn;
class APlayerController;
class FSoftClassProperty;
class UBlueprint;
class ULevel;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USkinnedMeshComponent;
class UStaticMeshComponent;
/// @cond DOXYGEN_WARNINGS
template<class TClass> class TSubclassOf;
/// @endcond

UE_NORETURN COREUOBJECT_API void CastLogError(const TCHAR* FromType, const TCHAR* ToType);

/**
 * Metafunction which detects whether or not a class is an IInterface.  Rules:
 *
 * 1. A UObject is not an IInterface.
 * 2. A type without a UClassType typedef member is not an IInterface.
 * 3. A type whose UClassType::StaticClassFlags does not have CLASS_Interface set is not an IInterface.
 *
 * Otherwise, assume it's an IInterface.
 */
template <typename T, bool bIsAUObject_IMPL = TPointerIsConvertibleFromTo<T, const volatile UObject>::Value>
struct TIsIInterface
{
	enum { Value = false };
};

template <typename T>
struct TIsIInterface<T, false>
{
	template <typename U> static char (&Resolve(typename U::UClassType*))[(U::UClassType::StaticClassFlags & CLASS_Interface) ? 2 : 1];
	template <typename U> static char (&Resolve(...))[1];

	enum { Value = sizeof(Resolve<T>(0)) - 1 };
};

template <typename T>
struct TIsCastable
{
	// It's from-castable if it's an interface or a UObject-derived type
	enum { Value = TIsIInterface<T>::Value || TPointerIsConvertibleFromTo<T, const volatile UObject>::Value };
};

template <typename T>
struct TIsCastableToPointer : TOr<TIsVoidType<T>, TIsCastable<T>>
{
	// It's to-pointer-castable if it's from-castable or void
};

template <typename T>
FORCEINLINE typename TEnableIf<TIsIInterface<T>::Value, FString>::Type GetTypeName()
{
	return T::UClassType::StaticClass()->GetName();
}

template <typename T>
FORCEINLINE typename TEnableIf<!TIsIInterface<T>::Value, FString>::Type GetTypeName()
{
	return T::StaticClass()->GetName();
}

enum class ECastType
{
	UObjectToUObject,
	InterfaceToUObject,
	UObjectToInterface,
	InterfaceToInterface,
	FromCastFlags
};

template <typename Type>
struct TCastFlags
{
	static const EClassCastFlags Value = CASTCLASS_None;
};

template <typename From, typename To, bool bFromInterface = TIsIInterface<From>::Value, bool bToInterface = TIsIInterface<To>::Value, EClassCastFlags CastClass = TCastFlags<To>::Value>
struct TGetCastType
{
#if USTRUCT_FAST_ISCHILDOF_IMPL == USTRUCT_ISCHILDOF_STRUCTARRAY
	static const ECastType Value = ECastType::UObjectToUObject;
#else
	static const ECastType Value = ECastType::FromCastFlags;
#endif
};

template <typename From, typename To                           > struct TGetCastType<From, To, false, false, CASTCLASS_None> { static const ECastType Value = ECastType::UObjectToUObject;     };
template <typename From, typename To                           > struct TGetCastType<From, To, false, true , CASTCLASS_None> { static const ECastType Value = ECastType::UObjectToInterface;   };
template <typename From, typename To, EClassCastFlags CastClass> struct TGetCastType<From, To, true,  false, CastClass     > { static const ECastType Value = ECastType::InterfaceToUObject;   };
template <typename From, typename To, EClassCastFlags CastClass> struct TGetCastType<From, To, true,  true , CastClass     > { static const ECastType Value = ECastType::InterfaceToInterface; };

template <typename To, typename From>
To* Cast(From* Src);

template <typename From, typename To, ECastType CastType = TGetCastType<From, To>::Value>
struct TCastImpl
{
	// This is the cast flags implementation
	FORCEINLINE static To* DoCast( UObject* Src )
	{
		return Src && Src->GetClass()->HasAnyCastFlag(TCastFlags<To>::Value) ? (To*)Src : nullptr;
	}

	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( UObject* Src )
	{
		return (To*)Src;
	}

	FORCEINLINE static To* DoCast( const FObjectPtr& Src )
	{
		UObject* SrcObj = ResolveObjectHandleNoRead(Src.GetHandleRef());
		if (SrcObj && Src->GetClass()->HasAnyCastFlag(TCastFlags<To>::Value))
		{
			ObjectHandle_Private::OnHandleRead(SrcObj);
			return (To*)SrcObj;
		}
		return nullptr;
	}

	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( const FObjectPtr& Src )
	{
		return (To*)Src.Get();
	}

	UE_DEPRECATED(4.25, "Cast<>() and CastChecked<>() should not be used with FProperties. Use CastField<>() or CastFieldChecked<>() instead.")
	FORCEINLINE static To* DoCast( FField* Src )
	{
		return Src && Src->IsA<To>() ? (To*)Src : nullptr;
	}

	UE_DEPRECATED(4.25, "Cast<>() and CastChecked<>() should not be used with FProperties. Use CastField<>() or CastFieldChecked<>() instead.")
	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( FField* Src )
	{
		return (To*)Src;
	}
};

template <typename From, typename To>
struct TCastImpl<From, To, ECastType::UObjectToUObject>
{
	FORCEINLINE static To* DoCast( UObject* Src )
	{
		return Src && Src->IsA<To>() ? (To*)Src : nullptr;
	}

	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( UObject* Src )
	{
		return (To*)Src;
	}

	FORCEINLINE static To* DoCast( const FObjectPtr& Src )
	{
		UObject* SrcObj = ResolveObjectHandleNoRead(Src.GetHandleRef());
		if (SrcObj && SrcObj->IsA<To>())
		{
			ObjectHandle_Private::OnHandleRead(SrcObj);
			return (To*)SrcObj;
		}
		return nullptr;
	}

	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( const FObjectPtr& Src )
	{
		return (To*)Src.Get();
	}

	UE_DEPRECATED(4.25, "Cast<>() and CastChecked<>() should not be used with FProperties. Use CastField<>() or CastFieldChecked<>() instead.")
	FORCEINLINE static To* DoCast( FField* Src )
	{
		return Src && Src->IsA<To>() ? (To*)Src : nullptr;
	}

	UE_DEPRECATED(4.25, "Cast<>() and CastChecked<>() should not be used with FProperties. Use CastField<>() or CastFieldChecked<>() instead.")
	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( FField* Src )
	{
		return (To*)Src;
	}
};

template <typename From, typename To>
struct TCastImpl<From, To, ECastType::InterfaceToUObject>
{
	FORCEINLINE static To* DoCast( From* Src )
	{
		if (Src)
		{
			UObject* Obj = Src->_getUObject();
			if (Obj && Obj->IsA<To>())
			{
				return (To*)Obj;
			}
		}
		return nullptr;
	}

	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( From* Src )
	{
		return Src ? (To*)Src->_getUObject() : nullptr;
	}
};

template <typename From, typename To>
struct TCastImpl<From, To, ECastType::UObjectToInterface>
{
	FORCEINLINE static To* DoCast( UObject* Src )
	{
		return Src ? (To*)Src->GetInterfaceAddress(To::UClassType::StaticClass()) : nullptr;
	}

	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( UObject* Src )
	{
		return Src ? (To*)Src->GetInterfaceAddress(To::UClassType::StaticClass()) : nullptr;
	}

	FORCEINLINE static To* DoCast( const FObjectPtr& Src )
	{
		UObject* SrcObj = ResolveObjectHandleNoRead(Src.GetHandleRef());
		if (SrcObj)
		{
			ObjectHandle_Private::OnHandleRead(SrcObj);
			return (To*)Src.Get()->GetInterfaceAddress(To::UClassType::StaticClass());
		}
		return nullptr;
	}

	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( const FObjectPtr& Src )
	{
		UObject* SrcObj = ResolveObjectHandleNoRead(Src.GetHandleRef());
		if (SrcObj)
		{
			ObjectHandle_Private::OnHandleRead(SrcObj);
			return (To*)Src.Get()->GetInterfaceAddress(To::UClassType::StaticClass());
		}
		return nullptr;
	}
};

template <typename From, typename To>
struct TCastImpl<From, To, ECastType::InterfaceToInterface>
{
	FORCEINLINE static To* DoCast( From* Src )
	{
		if (Src)
		{
			if (UObject* Obj = Src->_getUObject())
			{
				return (To*)Obj->GetInterfaceAddress(To::UClassType::StaticClass());
			}
		}
		return nullptr;
	}

	FORCEINLINE static To* DoCastCheckedWithoutTypeCheck( From* Src )
	{
		if (Src)
		{
			if (UObject* Obj = Src->_getUObject())
			{
				return (To*)Obj->GetInterfaceAddress(To::UClassType::StaticClass());
			}
		}
		return nullptr;
	}
};

// Dynamically cast an object type-safely.
template <typename To, typename From>
FORCEINLINE To* Cast(From* Src)
{
	return TCastImpl<From, To>::DoCast(Src);
}

template< class T >
FORCEINLINE T* ExactCast( UObject* Src )
{
	return Src && (Src->GetClass() == T::StaticClass()) ? (T*)Src : nullptr;
}

#if DO_CHECK

	// Helper function to get the full name for UObjects and UInterfaces
	template <
		typename T,
		typename TEnableIf<!TIsDerivedFrom<T, FField>::Value>::Type* = nullptr
	>
	FString GetFullNameForCastLogError(T* InObjectOrInterface)
	{
		return Cast<UObject>(InObjectOrInterface)->GetFullName();
	}
	// And a special version for FFields
	template <
		typename T,
		typename TEnableIf<TIsDerivedFrom<T, FField>::Value>::Type* = nullptr
	>
	FString GetFullNameForCastLogError(T* InField)
	{
		return GetFullNameSafe(InField);
	}

	template <typename To, typename From>
	FUNCTION_NON_NULL_RETURN_START
		To* CastChecked(From* Src)
	FUNCTION_NON_NULL_RETURN_END
	{
		if (!Src)
		{
			CastLogError(TEXT("nullptr"), *GetTypeName<To>());
		}

		To* Result = Cast<To>(Src);
		if (!Result)
		{
			CastLogError(*GetFullNameForCastLogError(Src), *GetTypeName<To>());
		}

		return Result;
	}

	template <typename To, typename From>
	To* CastChecked(From* Src, ECastCheckedType::Type CheckType)
	{
		if (Src)
		{
			To* Result = Cast<To>(Src);
			if (!Result)
			{
				CastLogError(*GetFullNameForCastLogError(Src), *GetTypeName<To>());
			}

			return Result;
		}

		if (CheckType == ECastCheckedType::NullChecked)
		{
			CastLogError(TEXT("nullptr"), *GetTypeName<To>());
		}

		return nullptr;
	}

#else

	template <typename To, typename From>
	FUNCTION_NON_NULL_RETURN_START
		FORCEINLINE To* CastChecked(From* Src)
	FUNCTION_NON_NULL_RETURN_END
	{
		return TCastImpl<From, To>::DoCastCheckedWithoutTypeCheck(Src);
	}

	template <typename To, typename From>
	FORCEINLINE To* CastChecked(From* Src, ECastCheckedType::Type CheckType)
	{
		return TCastImpl<From, To>::DoCastCheckedWithoutTypeCheck(Src);
	}

#endif

// auto weak versions
template< class T, class U > FORCEINLINE T* Cast       ( const TWeakObjectPtr<U>& Src                                                                   ) { return Cast       <T>(Src.Get()); }
template< class T, class U > FORCEINLINE T* ExactCast  ( const TWeakObjectPtr<U>& Src                                                                   ) { return ExactCast  <T>(Src.Get()); }
template< class T, class U > FORCEINLINE T* CastChecked( const TWeakObjectPtr<U>& Src, ECastCheckedType::Type CheckType = ECastCheckedType::NullChecked ) { return CastChecked<T>(Src.Get(), CheckType); }

// object ptr versions
template <class T, class U> FORCEINLINE T* Cast       ( const TObjectPtr<U>& Src                                                                   ) { return TCastImpl<U, T>::DoCast((const FObjectPtr&)Src); }
template <class T, class U> FORCEINLINE T* ExactCast  ( const TObjectPtr<U>& Src                                                                   )
{
	UObject* SrcObj = ResolveObjectHandleNoRead(((const FObjectPtr&)Src).GetHandleRef());
	if (SrcObj && (SrcObj->GetClass() == T::StaticClass()))
	{
		ObjectHandle_Private::OnHandleRead(SrcObj);
		return (T*)SrcObj;
	}
	return nullptr;
}
template <class T, class U> FORCEINLINE T* CastChecked( const TObjectPtr<U>& Src, ECastCheckedType::Type CheckType = ECastCheckedType::NullChecked )
{
#if DO_CHECK
	if (!Src.IsNull())
	{
		T* Result = Cast<T>(Src);
		if (!Result)
		{
			CastLogError(*GetFullNameForCastLogError(Src.Get()), *GetTypeName<T>());
		}

		return Result;
	}

	if (CheckType == ECastCheckedType::NullChecked)
	{
		CastLogError(TEXT("nullptr"), *GetTypeName<T>());
	}

	return nullptr;
#else
	return TCastImpl<U, T>::DoCastCheckedWithoutTypeCheck((const FObjectPtr&)Src);
#endif
}

// TSubclassOf versions
template< class T, class U > FORCEINLINE T* Cast       ( const TSubclassOf<U>& Src                                                                   ) { return Cast       <T>(*Src); }
template< class T, class U > FORCEINLINE T* CastChecked( const TSubclassOf<U>& Src, ECastCheckedType::Type CheckType = ECastCheckedType::NullChecked ) { return CastChecked<T>(*Src, CheckType); }

// Const versions of the casts
template< class T, class U > FORCEINLINE const T* Cast       ( const U      * Src                                                                   ) { return Cast       <T>(const_cast<U      *>(Src)); }
template< class T          > FORCEINLINE const T* ExactCast  ( const UObject* Src                                                                   ) { return ExactCast  <T>(const_cast<UObject*>(Src)); }
template< class T, class U > FORCEINLINE const T* CastChecked( const U      * Src, ECastCheckedType::Type CheckType = ECastCheckedType::NullChecked ) { return CastChecked<T>(const_cast<U      *>(Src), CheckType); }

#define DECLARE_CAST_BY_FLAG_FWD(ClassName) class ClassName;
#define DECLARE_CAST_BY_FLAG_CAST(ClassName) \
	template <> \
	struct TCastFlags<ClassName> \
	{ \
		static const EClassCastFlags Value = CASTCLASS_##ClassName; \
	}; \
	template <> \
	struct TCastFlags<const ClassName> \
	{ \
		static const EClassCastFlags Value = CASTCLASS_##ClassName; \
	};

#define DECLARE_CAST_BY_FLAG(ClassName) \
	DECLARE_CAST_BY_FLAG_FWD(ClassName) \
	DECLARE_CAST_BY_FLAG_CAST(ClassName)

#define FINISH_DECLARING_CAST_FLAGS // intentionally defined to do nothing.

// Define a macro that declares all the cast flags.
// This allows us to reuse these declarations elsewhere to define other properties for these classes.
// Note: When adding an item to this list, you must also add a CASTCLASS_ flag in ObjectBase.h and rebuild UnrealHeaderTool.
#define DECLARE_ALL_CAST_FLAGS \
DECLARE_CAST_BY_FLAG(UField)							\
DECLARE_CAST_BY_FLAG(UEnum)								\
DECLARE_CAST_BY_FLAG(UStruct)							\
DECLARE_CAST_BY_FLAG(UScriptStruct)						\
DECLARE_CAST_BY_FLAG(UClass)							\
DECLARE_CAST_BY_FLAG(FProperty)							\
DECLARE_CAST_BY_FLAG(FObjectPropertyBase)				\
DECLARE_CAST_BY_FLAG(FObjectProperty)					\
DECLARE_CAST_BY_FLAG(FObjectPtrProperty)				\
DECLARE_CAST_BY_FLAG(FWeakObjectProperty)				\
DECLARE_CAST_BY_FLAG(FLazyObjectProperty)				\
DECLARE_CAST_BY_FLAG(FSoftObjectProperty)				\
DECLARE_CAST_BY_FLAG(FSoftClassProperty)				\
DECLARE_CAST_BY_FLAG(FBoolProperty)						\
DECLARE_CAST_BY_FLAG(UFunction)							\
DECLARE_CAST_BY_FLAG(FStructProperty)					\
DECLARE_CAST_BY_FLAG(FByteProperty)						\
DECLARE_CAST_BY_FLAG(FIntProperty)						\
DECLARE_CAST_BY_FLAG(FFloatProperty)					\
DECLARE_CAST_BY_FLAG(FDoubleProperty)					\
DECLARE_CAST_BY_FLAG(FClassProperty)					\
DECLARE_CAST_BY_FLAG(FInterfaceProperty)				\
DECLARE_CAST_BY_FLAG(FNameProperty)						\
DECLARE_CAST_BY_FLAG(FStrProperty)						\
DECLARE_CAST_BY_FLAG(FTextProperty)						\
DECLARE_CAST_BY_FLAG(FArrayProperty)					\
DECLARE_CAST_BY_FLAG(FDelegateProperty)					\
DECLARE_CAST_BY_FLAG(FMulticastDelegateProperty)		\
DECLARE_CAST_BY_FLAG(UPackage)							\
DECLARE_CAST_BY_FLAG(ULevel)							\
DECLARE_CAST_BY_FLAG(AActor)							\
DECLARE_CAST_BY_FLAG(APlayerController)					\
DECLARE_CAST_BY_FLAG(APawn)								\
DECLARE_CAST_BY_FLAG(USceneComponent)					\
DECLARE_CAST_BY_FLAG(UPrimitiveComponent)				\
DECLARE_CAST_BY_FLAG(USkinnedMeshComponent)				\
DECLARE_CAST_BY_FLAG(USkeletalMeshComponent)			\
DECLARE_CAST_BY_FLAG(UBlueprint)						\
DECLARE_CAST_BY_FLAG(UDelegateFunction)					\
DECLARE_CAST_BY_FLAG(UStaticMeshComponent)				\
DECLARE_CAST_BY_FLAG(FEnumProperty)						\
DECLARE_CAST_BY_FLAG(FNumericProperty)					\
DECLARE_CAST_BY_FLAG(FInt8Property)						\
DECLARE_CAST_BY_FLAG(FInt16Property)					\
DECLARE_CAST_BY_FLAG(FInt64Property)					\
DECLARE_CAST_BY_FLAG(FUInt16Property)					\
DECLARE_CAST_BY_FLAG(FUInt32Property)					\
DECLARE_CAST_BY_FLAG(FUInt64Property)					\
DECLARE_CAST_BY_FLAG(FMapProperty)						\
DECLARE_CAST_BY_FLAG(FSetProperty)						\
DECLARE_CAST_BY_FLAG(USparseDelegateFunction)			\
DECLARE_CAST_BY_FLAG(FMulticastInlineDelegateProperty)	\
DECLARE_CAST_BY_FLAG(FMulticastSparseDelegateProperty)	\
FINISH_DECLARING_CAST_FLAGS		// This is here to hopefully remind people to include the "\" in all declarations above, especially when copy/pasting the final line.

// Now actually declare the flags
DECLARE_ALL_CAST_FLAGS

#undef DECLARE_CAST_BY_FLAG
#undef DECLARE_CAST_BY_FLAG_CAST
#undef DECLARE_CAST_BY_FLAG_FWD


#if HACK_HEADER_GENERATOR
// Singleton class to get the cast flag for a given class name.
struct COREUOBJECT_API ClassCastFlagMap
{
	static ClassCastFlagMap& Get();

	// Get Mapped name -> cast flag. Returns CASTCLASS_None if name is not found.
	EClassCastFlags GetCastFlag(const FString& ClassName) const;

private:
	ClassCastFlagMap();
	TMap<FString, EClassCastFlags> CastFlagMap;
};
#endif // HACK_HEADER_GENERATOR



namespace UECasts_Private
{
	template <typename To, typename From>
	FORCEINLINE typename TEnableIf<TAnd<TIsPointer<To>, TAnd<TIsCastableToPointer<typename TRemovePointer<To>::Type>, TIsCastable<From>>>::Value, To>::Type DynamicCast(From* Arg)
	{
		typedef typename TRemovePointer<To  >::Type ToValueType;
		typedef typename TRemovePointer<From>::Type FromValueType;

		// Casting away const/volatile
		static_assert(!TLosesQualifiersFromTo<FromValueType, ToValueType>::Value, "Conversion loses qualifiers");

		// If we're casting to void, cast to UObject instead and let it implicitly cast to void
		return Cast<typename TChooseClass<TIsVoidType<ToValueType>::Value, UObject, ToValueType>::Result>(Arg);
	}

	template <typename To, typename From>
	FORCEINLINE typename TEnableIf<!TAnd<TIsPointer<To>, TAnd<TIsCastableToPointer<typename TRemovePointer<To>::Type>, TIsCastable<From>>>::Value, To>::Type DynamicCast(From* Arg)
	{
		return dynamic_cast<To>(Arg);
	}

	template <typename To, typename From>
	FORCEINLINE typename TEnableIf<TAnd<TIsCastable<typename TRemoveReference<To>::Type>, TIsCastable<typename TRemoveReference<From>::Type>>::Value, To>::Type DynamicCast(From&& Arg)
	{
		typedef typename TRemoveReference<From>::Type FromValueType;
		typedef typename TRemoveReference<To  >::Type ToValueType;

		// Casting away const/volatile
		static_assert(!TLosesQualifiersFromTo<FromValueType, ToValueType>::Value, "Conversion loses qualifiers");

		// T&& can only be cast to U&&
		// http://en.cppreference.com/w/cpp/language/dynamic_cast
		static_assert(TOr<TIsLValueReferenceType<From>, TIsRValueReferenceType<To>>::Value, "Cannot dynamic_cast from an rvalue to a non-rvalue reference");

		return Forward<To>(*CastChecked<typename TRemoveReference<To>::Type>(&Arg));
	}

	template <typename To, typename From>
	FORCEINLINE typename TEnableIf<!TAnd<TIsCastable<typename TRemoveReference<To>::Type>, TIsCastable<typename TRemoveReference<From>::Type>>::Value, To>::Type DynamicCast(From&& Arg)
	{
		// This may fail when dynamic_casting rvalue references due to patchy compiler support
		return dynamic_cast<To>(Arg);
	}
}

#define dynamic_cast UECasts_Private::DynamicCast
