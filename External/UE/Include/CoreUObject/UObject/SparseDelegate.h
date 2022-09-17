// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "Delegates/Delegate.h"
#include "Misc/ScopeLock.h"

struct FSparseDelegate;

/**
*  Sparse delegates can be used for infrequently bound dynamic delegates so that the object uses only 
*  1 byte of storage instead of having the full overhead of the delegate invocation list.
*  The cost to invoke, add, remove, etc. from the delegate is higher than using the delegate
*  directly and thus the memory savings benefit should be traded off against the frequency
*  with which you would expect the delegate to be bound.
*/


/** Helper class for handling sparse delegate bindings */
struct COREUOBJECT_API FSparseDelegateStorage
{
public:

	/** Registers the sparse delegate so that the offset can be determined. */
	static void RegisterDelegateOffset(const UObject* OwningObject, FName DelegateName, size_t OffsetToOwner);

	/** Binds a sparse delegate to the owner. Returns if the delegate was successfully bound. */
	static bool Add(const UObject* DelegateOwner, const FName DelegateName, FScriptDelegate Delegate);

	/** Binds a sparse delegate to the owner, verifying first that the delegate is not already bound. Returns if the delegate was successfully bound. */
	static bool AddUnique(const UObject* DelegateOwner, const FName DelegateName, FScriptDelegate Delegate);

	/** Returns whether a sparse delegate is bound to the owner. */
	static bool Contains(const UObject* DelegateOwner, const FName DelegateName, const FScriptDelegate& Delegate);
	static bool Contains(const UObject* DelegateOwner, const FName DelegateName, const UObject* InObject, FName InFunctionName);

	/** Removes a delegate binding from the owner's sparse delegate storage. Returns true if there are still bindings to the delegate. */
	static bool Remove(const UObject* DelegateOwner, const FName DelegateName, const FScriptDelegate& Delegate);
	static bool Remove(const UObject* DelegateOwner, const FName DelegateName, const UObject* InObject, FName InFunctionName);

	/** Removes all sparse delegate binding from the owner for a given object. Returns true if there are still bindings to the delegate. */
	static bool RemoveAll(const UObject* DelegateOwner, const FName DelegateName, const UObject* UserObject);

	/** Clear all of the named sparse delegate bindings from the owner. */
	static void Clear(const UObject* DelegateOwner, const FName DelegateName);

	/** Acquires the actual Multicast Delegate from the annotation if any delegates are bound to it. Will be null if no entry exists in the annotation for this object/delegatename. */
	static FMulticastScriptDelegate* GetMulticastDelegate(const UObject* DelegateOwner, const FName DelegateName);

	/** Acquires the actual Multicast Delegate from the annotation if any delegates are bound to it as a shared pointer. Will be null if no entry exists in the annotation for this object/delegatename. */
	static TSharedPtr<FMulticastScriptDelegate> GetSharedMulticastDelegate(const UObject* DelegateOwner, const FName DelegateName);

	/** Directly sets the Multicast Delegate for this object/delegatename pair. If the delegate is unbound it will be assigned/inserted anyways. */
	static void SetMulticastDelegate(const UObject* DelegateOwner, const FName DelegateName, FMulticastScriptDelegate Delegate);

	/** Using the registry of sparse delegates recover the FSparseDelegate address from the UObject and name. */
	static FSparseDelegate* ResolveSparseDelegate(const UObject* OwningObject, FName DelegateName);

	/** Using the registry of sparse delegates recover the UObject owner from the FSparseDelegate address owning class and delegate names. */
	static UObject* ResolveSparseOwner(const FSparseDelegate& SparseDelegate, const FName OwningClassName, const FName DelegateName);

	/** Outputs a report about which delegates are bound. */
	static void SparseDelegateReport(const TArray<FString>&, UWorld*, FOutputDevice&);

private:

	struct FObjectListener : public FUObjectArray::FUObjectDeleteListener
	{
		virtual ~FObjectListener();
		virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override;
		virtual void OnUObjectArrayShutdown();
		void EnableListener();
		void DisableListener();
	};

	/** Allow the object listener to use the critical section and remove objects from the map */
	friend struct FObjectListener;

	/** A listener to get notified when objects have been deleted and remove them from the map */
	static FObjectListener SparseDelegateObjectListener;

	/** Critical Section for locking access to the sparse delegate map */
	static FCriticalSection SparseDelegateMapCritical;

	/** Delegate map is a map of Delegate names to a shared pointer of the multicast script delegate */
	typedef TMap<FName, TSharedPtr<FMulticastScriptDelegate>> FSparseDelegateMap;

	/** Map of objects to the map of delegates that are bound to that object */
	static TMap<const UObjectBase*, FSparseDelegateMap> SparseDelegates;
	
	/** Sparse delegate offsets are indexed by ActorClass/DelegateName pair */
	static TMap<TPair<FName, FName>, size_t> SparseDelegateObjectOffsets;
};

/** Base implementation for all sparse delegate types */
struct FSparseDelegate
{
public:
	FSparseDelegate()
		: bIsBound(false)
	{
	}

	/**
	* Checks to see if any functions are bound to this multi-cast delegate
	*
	* @return	True if any functions are bound
	*/
	bool IsBound() const
	{
		return bIsBound;
	}

	/**
	* Adds a function delegate to this multi-cast delegate's invocation list if a delegate with the same signature
	* doesn't already exist in the invocation list
	*
	* @param	DelegateOwner	UObject that owns the resolved sparse delegate
	* @param	DelegateName	Name of the resolved sparse delegate
	* @param	InDelegate		Delegate to bind to the sparse delegate
	* 
	* NOTE:  Only call this function from blueprint sparse delegate infrastructure on a resolved generic FScriptDelegate pointer.
	*        Generally from C++ you should call AddUnique() directly.
	*/
	void __Internal_AddUnique(const UObject* DelegateOwner, FName DelegateName, FScriptDelegate InDelegate)
	{
		bIsBound |= FSparseDelegateStorage::AddUnique(DelegateOwner, DelegateName, MoveTemp(InDelegate));
	}

	/**
	* Removes a function from this multi-cast delegate's invocation list (performance is O(N)).  Note that the
	* order of the delegates may not be preserved!
	*
	* @param	DelegateOwner	UObject that owns the resolved sparse delegate
	* @param	DelegateName	Name of the resolved sparse delegate
	* @param	InDelegate		Delegate to remove from the sparse delegate
	*
	* NOTE:  Only call this function from blueprint sparse delegate infrastructure on a resolved generic FScriptDelegate pointer.
	*        Generally from C++ you should call Remove() directly.
	*/
	void __Internal_Remove(const UObject* DelegateOwner, FName DelegateName, const FScriptDelegate& InDelegate)
	{
		if (bIsBound)
		{
			bIsBound = FSparseDelegateStorage::Remove(DelegateOwner, DelegateName, InDelegate);
		}
	}

	/**
	* Removes all functions from this delegate's invocation list
	*
	* @param	DelegateOwner	UObject that owns the resolved sparse delegate
	* @param	DelegateName	Name of the resolved sparse delegate
	*
	* NOTE:  Only call this function from blueprint sparse delegate infrastructure on a resolved generic FScriptDelegate pointer.
	*        Generally from C++ you should call Clear() directly.
	*/
	void __Internal_Clear(const UObject* DelegateOwner, FName DelegateName)
	{
		if (bIsBound)
		{
			FSparseDelegateStorage::Clear(DelegateOwner, DelegateName);
			bIsBound = false;
		}
	}

protected:

	friend class FMulticastSparseDelegateProperty;
	bool bIsBound;
};

/** Sparse version of TBaseDynamicDelegate */
template <typename MulticastDelegate, typename OwningClass, typename DelegateInfoClass>
struct TSparseDynamicDelegate : public FSparseDelegate
{
public:
	typedef typename MulticastDelegate::FDelegate FDelegate; 

protected:
	FName GetDelegateName() const
	{
		static const FName DelegateFName(DelegateInfoClass::GetDelegateName());
		return DelegateFName;
	}

private:
	UObject* GetDelegateOwner() const
	{
		const size_t OffsetToOwner = DelegateInfoClass::template GetDelegateOffset<OwningClass>();
		check(OffsetToOwner);
		UObject* DelegateOwner = reinterpret_cast<UObject*>((uint8*)this - OffsetToOwner);
		check(DelegateOwner->IsValidLowLevelFast(false)); // Most likely the delegate is trying to be used on the stack, in an object it wasn't defined for, or for a class member with a different name than it was defined for. It is only valid for a sparse delegate to be used for the exact class/property name it is defined with.
		return DelegateOwner;
	}

public:
	/** Returns the multicast delegate if any delegates are bound to the sparse delegate */
	UE_DEPRECATED(4.25, "This function has been deprecated - please use GetShared() instead")
	MulticastDelegate* Get() const
	{
		MulticastDelegate* Result = nullptr;
		if (bIsBound)
		{
			Result = static_cast<MulticastDelegate*>(FSparseDelegateStorage::GetMulticastDelegate(GetDelegateOwner(), GetDelegateName()));
		}
		return Result;
	}

	/** Returns the multicast delegate if any delegates are bound to the sparse delegate */
	TSharedPtr<MulticastDelegate> GetShared() const
	{
		TSharedPtr<MulticastDelegate> Result;
		if (bIsBound)
		{
			Result = StaticCastSharedPtr<MulticastDelegate>(FSparseDelegateStorage::GetSharedMulticastDelegate(GetDelegateOwner(), GetDelegateName()));
		}
		return Result;
	}

	/**
	* Checks whether a function delegate is already a member of this multi-cast delegate's invocation list
	*
	* @param	InDelegate	Delegate to check
	* @return	True if the delegate is already in the list.
	*/
	bool Contains(const FScriptDelegate& InDelegate) const
	{
		return (bIsBound ? FSparseDelegateStorage::Contains(GetDelegateOwner(), GetDelegateName(), InDelegate) : false);
	}

	/**
	* Checks whether a function delegate is already a member of this multi-cast delegate's invocation list
	*
	* @param	InObject		Object of the delegate to check
	* @param	InFunctionName	Function name of the delegate to check
	* @return	True if the delegate is already in the list.
	*/
	bool Contains(const UObject* InObject, FName InFunctionName) const
	{
		return (bIsBound ? FSparseDelegateStorage::Contains(GetDelegateOwner(), GetDelegateName(), InObject, InFunctionName) : false);
	}

	/**
	* Adds a function delegate to this multi-cast delegate's invocation list
	*
	* @param	InDelegate	Delegate to add
	*/
	void Add(FScriptDelegate InDelegate)
	{
		bIsBound |= FSparseDelegateStorage::Add(GetDelegateOwner(), GetDelegateName(), MoveTemp(InDelegate));
	}

	/**
	* Adds a function delegate to this multi-cast delegate's invocation list if a delegate with the same signature
	* doesn't already exist in the invocation list
	*
	* @param	InDelegate	Delegate to add
	*/
	void AddUnique(FScriptDelegate InDelegate)
	{
		FSparseDelegate::__Internal_AddUnique(GetDelegateOwner(), GetDelegateName(), MoveTemp(InDelegate));
	}

	/**
	* Removes a function from this multi-cast delegate's invocation list (performance is O(N)).  Note that the
	* order of the delegates may not be preserved!
	*
	* @param	InDelegate	Delegate to remove
	*/
	void Remove(const FScriptDelegate& InDelegate)
	{
		FSparseDelegate::__Internal_Remove(GetDelegateOwner(), GetDelegateName(), InDelegate);
	}

	/**
	* Removes a function from this multi-cast delegate's invocation list (performance is O(N)).  Note that the
	* order of the delegates may not be preserved!
	*
	* @param	InObject		Object of the delegate to remove
	* @param	InFunctionName	Function name of the delegate to remove
	*/
	void Remove(const UObject* InObject, FName InFunctionName)
	{
		if (bIsBound)
		{
			bIsBound = FSparseDelegateStorage::Remove(GetDelegateOwner(), GetDelegateName(), InObject, InFunctionName);
		}
	}

	/**
	* Removes all delegate bindings from this multicast delegate's
	* invocation list that are bound to the specified object.
	*
	* This method also compacts the invocation list.
	*
	* @param InObject The object to remove bindings for.
	*/
	void RemoveAll(const UObject* Object)
	{
		if (bIsBound)
		{
			bIsBound = FSparseDelegateStorage::RemoveAll(GetDelegateOwner(), GetDelegateName(), Object);
		}
	}

	/**
	* Removes all functions from this delegate's invocation list
	*/
	void Clear()
	{
		FSparseDelegate::__Internal_Clear(GetDelegateOwner(), GetDelegateName());
	}
	
	/**
	* Broadcasts this delegate to all bound objects, except to those that may have expired.
	*/
	template<typename... ParamTypes>
	void Broadcast(ParamTypes... Params)
	{
		if (TSharedPtr<MulticastDelegate> MCDelegate = GetShared())
		{
			MCDelegate->Broadcast(Params...);
		}
	}

	/**
	* Tests if a UObject instance and a UObject method address pair are already bound to this multi-cast delegate.
	*
	* @param	InUserObject		UObject instance
	* @param	InMethodPtr			Member function address pointer
	* @param	InFunctionName		Name of member function, without class name
	* @return	True if the instance/method is already bound.
	*
	* NOTE:  Do not call this function directly.  Instead, call IsAlreadyBound() which is a macro proxy function that
	*        automatically sets the function name string for the caller.
	*/
	template< class UserClass >
	bool __Internal_IsAlreadyBound( UserClass* InUserObject, typename FDelegate::template TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, FName InFunctionName ) const
	{
		check( InUserObject != nullptr && InMethodPtr != nullptr );

		// NOTE: We're not actually using the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		return this->Contains( InUserObject, InFunctionName );
	}

	/**
	* Binds a UObject instance and a UObject method address to this multi-cast delegate.
	*
	* @param	InUserObject		UObject instance
	* @param	InMethodPtr			Member function address pointer
	* @param	InFunctionName		Name of member function, without class name
	*
	* NOTE:  Do not call this function directly.  Instead, call AddDynamic() which is a macro proxy function that
	*        automatically sets the function name string for the caller.
	*/
	template< class UserClass >
	void __Internal_AddDynamic( UserClass* InUserObject, typename FDelegate::template TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, FName InFunctionName )
	{
		check( InUserObject != nullptr && InMethodPtr != nullptr );

		// NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		FDelegate NewDelegate;
		NewDelegate.__Internal_BindDynamic( InUserObject, InMethodPtr, InFunctionName );

		this->Add( NewDelegate );
	}

	/**
	* Binds a UObject instance and a UObject method address to this multi-cast delegate, but only if it hasn't been bound before.
	*
	* @param	InUserObject		UObject instance
	* @param	InMethodPtr			Member function address pointer
	* @param	InFunctionName		Name of member function, without class name
	*
	* NOTE:  Do not call this function directly.  Instead, call AddUniqueDynamic() which is a macro proxy function that
	*        automatically sets the function name string for the caller.
	*/
	template< class UserClass >
	void __Internal_AddUniqueDynamic( UserClass* InUserObject, typename FDelegate::template TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, FName InFunctionName )
	{
		check( InUserObject != nullptr && InMethodPtr != nullptr );

		// NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		FDelegate NewDelegate;
		NewDelegate.__Internal_BindDynamic( InUserObject, InMethodPtr, InFunctionName );

		this->AddUnique( NewDelegate );
	}

	/**
	* Unbinds a UObject instance and a UObject method address from this multi-cast delegate.
	*
	* @param	InUserObject		UObject instance
	* @param	InMethodPtr			Member function address pointer
	* @param	InFunctionName		Name of member function, without class name
	*
	* NOTE:  Do not call this function directly.  Instead, call RemoveDynamic() which is a macro proxy function that
	*        automatically sets the function name string for the caller.
	*/
	template< class UserClass >
	void __Internal_RemoveDynamic( UserClass* InUserObject, typename FDelegate::template TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, FName InFunctionName )
	{
		check( InUserObject != nullptr && InMethodPtr != nullptr );

		// NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		this->Remove( InUserObject, InFunctionName );
	}

private:
	// Hide internal functions that never need to be called on the derived classes
	void __Internal_AddUnique(const UObject*, FName, FScriptDelegate);
	void __Internal_Remove(const UObject*, FName, const FScriptDelegate&);
	void __Internal_Clear(const UObject*, FName);
};

#define FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE(SparseDelegateClassName, OwningClass, DelegateName, FuncParamList, FuncParamPassThru, ...) \
	FUNC_DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeakObjectPtr, SparseDelegateClassName##_MCSignature, SparseDelegateClassName##_DelegateWrapper, FUNC_CONCAT(FuncParamList), FUNC_CONCAT(FuncParamPassThru), __VA_ARGS__) \
	struct SparseDelegateClassName##InfoGetter \
	{ \
		static const char* GetDelegateName() { return #DelegateName; } \
		template<typename T> \
		static size_t GetDelegateOffset() { return offsetof(T, DelegateName); } \
	}; \
	struct SparseDelegateClassName : public TSparseDynamicDelegate<SparseDelegateClassName##_MCSignature, OwningClass, SparseDelegateClassName##InfoGetter> \
	{ \
	};

/** Declares a sparse blueprint-accessible broadcast delegate that can bind to multiple native UFUNCTIONs simultaneously */
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, , FUNC_CONCAT( *this ), void )

#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1 ), FUNC_CONCAT( *this, InParam1 ), void, Param1Type )
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name, Param2Type, Param2Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1, Param2Type InParam2 ), FUNC_CONCAT( *this, InParam1, InParam2 ), void, Param1Type, Param2Type )
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_ThreeParams( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name, Param2Type, Param2Name, Param3Type, Param3Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1, Param2Type InParam2, Param3Type InParam3 ), FUNC_CONCAT( *this, InParam1, InParam2, InParam3 ), void, Param1Type, Param2Type, Param3Type )
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name, Param2Type, Param2Name, Param3Type, Param3Name, Param4Type, Param4Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1, Param2Type InParam2, Param3Type InParam3, Param4Type InParam4 ), FUNC_CONCAT( *this, InParam1, InParam2, InParam3, InParam4 ), void, Param1Type, Param2Type, Param3Type, Param4Type )
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FiveParams( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name, Param2Type, Param2Name, Param3Type, Param3Name, Param4Type, Param4Name, Param5Type, Param5Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1, Param2Type InParam2, Param3Type InParam3, Param4Type InParam4, Param5Type InParam5 ), FUNC_CONCAT( *this, InParam1, InParam2, InParam3, InParam4, InParam5 ), void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type )
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_SixParams( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name, Param2Type, Param2Name, Param3Type, Param3Name, Param4Type, Param4Name, Param5Type, Param5Name, Param6Type, Param6Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1, Param2Type InParam2, Param3Type InParam3, Param4Type InParam4, Param5Type InParam5, Param6Type InParam6 ), FUNC_CONCAT( *this, InParam1, InParam2, InParam3, InParam4, InParam5, InParam6 ), void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type )
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_SevenParams( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name, Param2Type, Param2Name, Param3Type, Param3Name, Param4Type, Param4Name, Param5Type, Param5Name, Param6Type, Param6Name, Param7Type, Param7Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1, Param2Type InParam2, Param3Type InParam3, Param4Type InParam4, Param5Type InParam5, Param6Type InParam6, Param7Type InParam7 ), FUNC_CONCAT( *this, InParam1, InParam2, InParam3, InParam4, InParam5, InParam6, InParam7 ), void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type )
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_EightParams( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name, Param2Type, Param2Name, Param3Type, Param3Name, Param4Type, Param4Name, Param5Type, Param5Name, Param6Type, Param6Name, Param7Type, Param7Name, Param8Type, Param8Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1, Param2Type InParam2, Param3Type InParam3, Param4Type InParam4, Param5Type InParam5, Param6Type InParam6, Param7Type InParam7, Param8Type InParam8 ), FUNC_CONCAT( *this, InParam1, InParam2, InParam3, InParam4, InParam5, InParam6, InParam7, InParam8 ), void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type )
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_NineParams( SparseDelegateClass, OwningClass, DelegateName, Param1Type, Param1Name, Param2Type, Param2Name, Param3Type, Param3Name, Param4Type, Param4Name, Param5Type, Param5Name, Param6Type, Param6Name, Param7Type, Param7Name, Param8Type, Param8Name, Param9Type, Param9Name ) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_DELEGATE) FUNC_DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE( SparseDelegateClass, OwningClass, DelegateName, FUNC_CONCAT( Param1Type InParam1, Param2Type InParam2, Param3Type InParam3, Param4Type InParam4, Param5Type InParam5, Param6Type InParam6, Param7Type InParam7, Param8Type InParam8, Param9Type InParam9 ), FUNC_CONCAT( *this, InParam1, InParam2, InParam3, InParam4, InParam5, InParam6, InParam7, InParam8, InParam9 ), void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type, Param9Type )
