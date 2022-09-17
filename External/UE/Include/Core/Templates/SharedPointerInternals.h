// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Templates/RemoveReference.h"
#include "Templates/TypeCompatibleBytes.h"
#include <atomic>
#include <type_traits>


/** Default behavior. */
#define THREAD_SANITISE_UNSAFEPTR 0

#if THREAD_SANITISE_UNSAFEPTR
	#define TSAN_SAFE_UNSAFEPTR
#else
	#define TSAN_SAFE_UNSAFEPTR TSAN_SAFE
#endif

/**
 * ESPMode is used select between either 'fast' or 'thread safe' shared pointer types.
 * This is only used by templates at compile time to generate one code path or another.
 */
enum class ESPMode : uint8
{
	/** Forced to be not thread-safe. */
	NotThreadSafe = 0,

	/** Thread-safe, never spin locks, but slower */
	ThreadSafe = 1,

	/**
	 * Fast was sometimes ThreadSafe and sometimes NotThreadSafe, but Fast is a misnomer.
	 * Deprecated since ThreadSafe became the default.
	 */
	Fast UE_DEPRECATED(5.0, "ESPMode::Fast has been deprecated - please use ESPMode::ThreadSafe instead") = 1
};


// Forward declarations.  By default, thread safety features are turned on. (Mode = ESPMode::ThreadSafe).
// If you need more concerned with performance of ref-counting, you should use ESPMode::NotThreadSafe.
template< class ObjectType, ESPMode Mode = ESPMode::ThreadSafe > class TSharedRef;
template< class ObjectType, ESPMode Mode = ESPMode::ThreadSafe > class TSharedPtr;
template< class ObjectType, ESPMode Mode = ESPMode::ThreadSafe > class TWeakPtr;
template< class ObjectType, ESPMode Mode = ESPMode::ThreadSafe > class TSharedFromThis;


/**
 * SharedPointerInternals contains internal workings of shared and weak pointers.  You should
 * hopefully never have to use anything inside this namespace directly.
 */
namespace SharedPointerInternals
{
	// Forward declarations
	template< ESPMode Mode > class FWeakReferencer;

	/** Dummy structures used internally as template arguments for typecasts */
	struct FStaticCastTag {};
	struct FConstCastTag {};

	// NOTE: The following is an Unreal extension to standard shared_ptr behavior
	struct FNullTag {};

	template <ESPMode Mode>
	class TReferenceControllerBase
	{
		using RefCountType = std::conditional_t<Mode == ESPMode::ThreadSafe, std::atomic<int32>, int32>;

	public:
		FORCEINLINE explicit TReferenceControllerBase() = default;

		// Number of shared references to this object.  When this count reaches zero, the associated object
		// will be destroyed (even if there are still weak references!), but not the reference controller.
		//
		// This starts at 1 because we create reference controllers via the construction of a TSharedPtr,
		// and that is the first reference.  There is no point in starting at 0 and then incrementing it.
		RefCountType SharedReferenceCount{1};

		// Number of weak references to this object.  If there are any shared references, that counts as one
		// weak reference too.  When this count reaches zero, the reference controller will be deleted.
		//
		// This starts at 1 because it represents the shared reference that we are also initializing
		// SharedReferenceCount with.
		RefCountType WeakReferenceCount{1};

		/** Destroys the object associated with this reference counter.  */
		virtual void DestroyObject() = 0;

		virtual ~TReferenceControllerBase()
		{
		}

		/** Returns the shared reference count */
		FORCEINLINE int32 GetSharedReferenceCount() const
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				// A 'live' shared reference count is unstable by nature and so there's no benefit
				// to try and enforce memory ordering around the reading of it.
				//
				// This is equivalent to https://en.cppreference.com/w/cpp/memory/shared_ptr/use_count

				// This reference count may be accessed by multiple threads
				return SharedReferenceCount.load(std::memory_order_relaxed);
			}
			else
			{
				return SharedReferenceCount;
			}
		}

		/** Adds a shared reference to this counter */
		FORCEINLINE void AddSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				// Incrementing a reference count with relaxed ordering is always safe because no other action is taken
				// in response to the increment, so there's nothing to order with.

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
				// We do a regular SC increment here because it maps to an _InterlockedIncrement (lock inc).
				// The codegen for a relaxed fetch_add is actually much worse under MSVC (lock xadd).
				++SharedReferenceCount;
#else
				SharedReferenceCount.fetch_add(1, std::memory_order_relaxed);
#endif
			}
			else
			{
				++SharedReferenceCount;
			}
		}

		/**
		 * Adds a shared reference to this counter ONLY if there is already at least one reference
		 *
		 * @return  True if the shared reference was added successfully
		 */
		bool ConditionallyAddSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				// See AddSharedReference for the same reasons that std::memory_order_relaxed is used in this function.

				// Peek at the current shared reference count.  Remember, this value may be updated by
				// multiple threads.
				int32 OriginalCount = SharedReferenceCount.load(std::memory_order_relaxed);

				for ( ; ; )
				{
					if( OriginalCount == 0 )
					{
						// Never add a shared reference if the pointer has already expired
						return false;
					}

					// Attempt to increment the reference count.
					//
					// We need to make sure that we never revive a counter that has already expired, so if the
					// actual value what we expected (because it was touched by another thread), then we'll try
					// again.  Note that only in very unusual cases will this actually have to loop.
					//
					// We do a weak read here because we require a loop and this is the recommendation:
					//
					// https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
					//
					// > When a compare-and-exchange is in a loop, the weak version will yield better performance on some platforms.
					// > When a weak compare-and-exchange would require a loop and a strong one would not, the strong one is preferable
					if (SharedReferenceCount.compare_exchange_weak(OriginalCount, OriginalCount + 1, std::memory_order_relaxed))
					{
						return true;
					}
				}
			}
			else
			{
				if( SharedReferenceCount == 0 )
				{
					// Never add a shared reference if the pointer has already expired
					return false;
				}

				++SharedReferenceCount;
				return true;
			}
		}

		/** Releases a shared reference to this counter */
		FORCEINLINE void ReleaseSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				// std::memory_order_release is used here so that, if we do end up executing the destructor, it's not possible
				// for side effects from executing the destructor end up being visible before we've determined that the shared
				// reference count is actually zero.

				int32 OldSharedCount = SharedReferenceCount.fetch_sub(1, std::memory_order_release);
				checkSlow(OldSharedCount > 0);
				if (OldSharedCount == 1)
				{
					// Ensure that all other threads' accesses to the object are visible to this thread before we call the
					// destructor.
					std::atomic_thread_fence(std::memory_order_acquire);

					// Last shared reference was released!  Destroy the referenced object.
					DestroyObject();

					// No more shared referencers, so decrement the weak reference count by one.  When the weak
					// reference count reaches zero, this object will be deleted.
					ReleaseWeakReference();
				}
			}
			else
			{
				checkSlow( SharedReferenceCount > 0 );

				if( --SharedReferenceCount == 0 )
				{
					// Last shared reference was released!  Destroy the referenced object.
					DestroyObject();

					// No more shared referencers, so decrement the weak reference count by one.  When the weak
					// reference count reaches zero, this object will be deleted.
					ReleaseWeakReference();
				}
			}
		}

		/** Adds a weak reference to this counter */
		FORCEINLINE void AddWeakReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				// See AddSharedReference for the same reasons that std::memory_order_relaxed is used in this function.

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
				// We do a regular SC increment here because it maps to an _InterlockedIncrement (lock inc).
				// The codegen for a relaxed fetch_add is actually much worse under MSVC (lock xadd).
				++WeakReferenceCount;
#else
				WeakReferenceCount.fetch_add(1, std::memory_order_relaxed);
#endif
			}
			else
			{
				++WeakReferenceCount;
			}
		}

		/** Releases a weak reference to this counter */
		void ReleaseWeakReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				// See ReleaseSharedReference for the same reasons that std::memory_order_release and std::memory_order_acquire are used in this function.

				int32 OldWeakCount = WeakReferenceCount.fetch_sub(1, std::memory_order_release);
				checkSlow(OldWeakCount > 0);
				if (OldWeakCount == 1)
				{
					std::atomic_thread_fence(std::memory_order_acquire);

					// Disable this if running clang's static analyzer. Passing shared pointers
					// and references to functions it cannot reason about, produces false
					// positives about use-after-free in the TSharedPtr/TSharedRef destructors.
#if !defined(__clang_analyzer__)
					// No more references to this reference count.  Destroy it!
					delete this;
#endif
				}
			}
			else
			{
				checkSlow( WeakReferenceCount > 0 );

				if( --WeakReferenceCount == 0 )
				{
					// No more references to this reference count.  Destroy it!
#if !defined(__clang_analyzer__)
					delete this;
#endif
				}
			}
		}

		// Non-copyable
		TReferenceControllerBase(const TReferenceControllerBase&) = delete;
		TReferenceControllerBase& operator=(const TReferenceControllerBase&) = delete;
	};

	// A helper class that efficiently stores a custom deleter and is intended to be derived from. If the custom deleter is an empty class,
	// TDeleterHolder derives from it exploiting empty base optimisation (https://en.cppreference.com/w/cpp/language/ebo). Otherwise
	// it stores the custom deleter as a member to allow a function pointer to be used as a custom deleter (a function pointer can't 
	// be a base class)
	template <typename DeleterType, bool bIsZeroSize = std::is_empty_v<DeleterType>>
	struct TDeleterHolder : private DeleterType
	{
		explicit TDeleterHolder(DeleterType&& Arg)
			: DeleterType(MoveTemp(Arg))
		{
		}

		template <typename ObjectType>
		void InvokeDeleter(ObjectType * Object)
		{
			Invoke(*static_cast<DeleterType*>(this), Object);
		}
	};

	template <typename DeleterType>
	struct TDeleterHolder<DeleterType, false>
	{
		explicit TDeleterHolder(DeleterType&& Arg)
			: Deleter(MoveTemp(Arg))
		{
		}

		template <typename ObjectType>
		void InvokeDeleter(ObjectType * Object)
		{
			Invoke(Deleter, Object);
		}

	private:
		DeleterType Deleter;
	};

	template <typename ObjectType, typename DeleterType, ESPMode Mode>
	class TReferenceControllerWithDeleter : private TDeleterHolder<DeleterType>, public TReferenceControllerBase<Mode>
	{
	public:
		explicit TReferenceControllerWithDeleter(ObjectType* InObject, DeleterType&& Deleter)
			: TDeleterHolder<DeleterType>(MoveTemp(Deleter))
			, Object(InObject)
		{
		}

		virtual void DestroyObject() override
		{
			this->InvokeDeleter(Object);
		}

		// Non-copyable
		TReferenceControllerWithDeleter(const TReferenceControllerWithDeleter&) = delete;
		TReferenceControllerWithDeleter& operator=(const TReferenceControllerWithDeleter&) = delete;

	private:
		/** The object associated with this reference counter.  */
		ObjectType* Object;
	};

	template <typename ObjectType, ESPMode Mode>
	class TIntrusiveReferenceController : public TReferenceControllerBase<Mode>
	{
	public:
		template <typename... ArgTypes>
		explicit TIntrusiveReferenceController(ArgTypes&&... Args)
		{
			new ((void*)&ObjectStorage) ObjectType(Forward<ArgTypes>(Args)...);
		}

		ObjectType* GetObjectPtr() const
		{
			return (ObjectType*)&ObjectStorage;
		}

		virtual void DestroyObject() override
		{
			DestructItem((ObjectType*)&ObjectStorage);
		}

		// Non-copyable
		TIntrusiveReferenceController(const TIntrusiveReferenceController&) = delete;
		TIntrusiveReferenceController& operator=(const TIntrusiveReferenceController&) = delete;

	private:
		/** The object associated with this reference counter.  */
		mutable TTypeCompatibleBytes<ObjectType> ObjectStorage;
	};


	/** Deletes an object via the standard delete operator */
	template <typename Type>
	struct DefaultDeleter
	{
		FORCEINLINE void operator()(Type* Object) const
		{
			delete Object;
		}
	};

	/** Creates a reference controller which just calls delete */
	template <ESPMode Mode, typename ObjectType>
	inline TReferenceControllerBase<Mode>* NewDefaultReferenceController(ObjectType* Object)
	{
		return new TReferenceControllerWithDeleter<ObjectType, DefaultDeleter<ObjectType>, Mode>(Object, DefaultDeleter<ObjectType>());
	}

	/** Creates a custom reference controller with a specified deleter */
	template <ESPMode Mode, typename ObjectType, typename DeleterType>
	inline TReferenceControllerBase<Mode>* NewCustomReferenceController(ObjectType* Object, DeleterType&& Deleter)
	{
		return new TReferenceControllerWithDeleter<ObjectType, typename TRemoveReference<DeleterType>::Type, Mode>(Object, Forward<DeleterType>(Deleter));
	}

	/** Creates an intrusive reference controller */
	template <ESPMode Mode, typename ObjectType, typename... ArgTypes>
	inline TIntrusiveReferenceController<ObjectType, Mode>* NewIntrusiveReferenceController(ArgTypes&&... Args)
	{
		return new TIntrusiveReferenceController<ObjectType, Mode>(Forward<ArgTypes>(Args)...);
	}


	/** Proxy structure for implicitly converting raw pointers to shared/weak pointers */
	// NOTE: The following is an Unreal extension to standard shared_ptr behavior
	template <class ObjectType>
	struct TRawPtrProxy
	{
		/** The object pointer */
		ObjectType* Object;

		/** Construct implicitly from a nullptr */
		FORCEINLINE TRawPtrProxy( TYPE_OF_NULLPTR )
			: Object( nullptr )
		{
		}

		/** Construct explicitly from an object */
		explicit FORCEINLINE TRawPtrProxy( ObjectType* InObject )
			: Object( InObject )
		{
		}
	};


	/** Proxy structure for implicitly converting raw pointers to shared/weak pointers */
	// NOTE: The following is an Unreal extension to standard shared_ptr behavior
	template <class ObjectType, typename DeleterType>
	struct TRawPtrProxyWithDeleter
	{
		/** The object pointer */
		ObjectType* Object;

		/** The deleter object */
		DeleterType Deleter;

		/** Construct implicitly from an object and a custom deleter */
		FORCEINLINE TRawPtrProxyWithDeleter( ObjectType* InObject, const DeleterType& InDeleter )
			: Object ( InObject )
			, Deleter( InObject, InDeleter )
		{
		}

		/** Construct implicitly from an object and a custom deleter */
		FORCEINLINE TRawPtrProxyWithDeleter( ObjectType* InObject, DeleterType&& InDeleter )
			: Object ( InObject )
			, Deleter( MoveTemp( InDeleter ) )
		{
		}
	};


	/**
	 * FSharedReferencer is a wrapper around a pointer to a reference controller that is used by either a
	 * TSharedRef or a TSharedPtr to keep track of a referenced object's lifetime
	 */
	template< ESPMode Mode >
	class FSharedReferencer
	{
	public:

		/** Constructor for an empty shared referencer object */
		FORCEINLINE FSharedReferencer()
			: ReferenceController( nullptr )
		{ }

		/** Constructor that counts a single reference to the specified object */
		inline explicit FSharedReferencer( TReferenceControllerBase<Mode>* InReferenceController )
			: ReferenceController( InReferenceController )
		{ }
		
		/** Copy constructor creates a new reference to the existing object */
		FORCEINLINE FSharedReferencer( FSharedReferencer const& InSharedReference )
			: ReferenceController( InSharedReference.ReferenceController )
		{
			// If the incoming reference had an object associated with it, then go ahead and increment the
			// shared reference count
			if( ReferenceController != nullptr )
			{
				ReferenceController->AddSharedReference();
			}
		}

		/** Move constructor creates no new references */
		FORCEINLINE FSharedReferencer( FSharedReferencer&& InSharedReference )
			: ReferenceController( InSharedReference.ReferenceController )
		{
			InSharedReference.ReferenceController = nullptr;
		}

		/** Creates a shared referencer object from a weak referencer object.  This will only result
		    in a valid object reference if the object already has at least one other shared referencer. */
		FSharedReferencer( FWeakReferencer< Mode > const& InWeakReference )
			: ReferenceController( InWeakReference.ReferenceController )
		{
			// If the incoming reference had an object associated with it, then go ahead and increment the
			// shared reference count
			if( ReferenceController != nullptr )
			{
				// Attempt to elevate a weak reference to a shared one.  For this to work, the object this
				// weak counter is associated with must already have at least one shared reference.  We'll
				// never revive a pointer that has already expired!
				if( !ReferenceController->ConditionallyAddSharedReference() )
				{
					ReferenceController = nullptr;
				}
			}
		}

		/** Creates a shared referencer object from a weak referencer object.  This will only result
		    in a valid object reference if the object already has at least one other shared referencer. */
		FSharedReferencer( FWeakReferencer< Mode >&& InWeakReference )
			: ReferenceController( InWeakReference.ReferenceController )
		{
			// If the incoming reference had an object associated with it, then go ahead and increment the
			// shared reference count
			if( ReferenceController != nullptr )
			{
				// Attempt to elevate a weak reference to a shared one.  For this to work, the object this
				// weak counter is associated with must already have at least one shared reference.  We'll
				// never revive a pointer that has already expired!
				if( !ReferenceController->ConditionallyAddSharedReference() )
				{
					ReferenceController = nullptr;
				}

				// Tell the reference counter object that we're no longer referencing the object with
				// this weak pointer
				InWeakReference.ReferenceController->ReleaseWeakReference();
				InWeakReference.ReferenceController = nullptr;
			}
		}

		/** Destructor. */
		FORCEINLINE ~FSharedReferencer()
		{
			if( ReferenceController != nullptr )
			{
				// Tell the reference counter object that we're no longer referencing the object with
				// this shared pointer
				ReferenceController->ReleaseSharedReference();
			}
		}

		/** Assignment operator adds a reference to the assigned object.  If this counter was previously
		    referencing an object, that reference will be released. */
		inline FSharedReferencer& operator=( FSharedReferencer const& InSharedReference )
		{
			// Make sure we're not be reassigned to ourself!
			auto NewReferenceController = InSharedReference.ReferenceController;
			if( NewReferenceController != ReferenceController )
			{
				// First, add a shared reference to the new object
				if( NewReferenceController != nullptr )
				{
					NewReferenceController->AddSharedReference();
				}

				// Release shared reference to the old object
				if( ReferenceController != nullptr )
				{
					ReferenceController->ReleaseSharedReference();
				}

				// Assume ownership of the assigned reference counter
				ReferenceController = NewReferenceController;
			}

			return *this;
		}

		/** Move assignment operator adds no references to the assigned object.  If this counter was previously
		    referencing an object, that reference will be released. */
		inline FSharedReferencer& operator=( FSharedReferencer&& InSharedReference )
		{
			// Make sure we're not be reassigned to ourself!
			auto NewReferenceController = InSharedReference.ReferenceController;
			auto OldReferenceController = ReferenceController;
			if( NewReferenceController != OldReferenceController )
			{
				// Assume ownership of the assigned reference counter
				InSharedReference.ReferenceController = nullptr;
				ReferenceController                   = NewReferenceController;

				// Release shared reference to the old object
				if( OldReferenceController != nullptr )
				{
					OldReferenceController->ReleaseSharedReference();
				}
			}

			return *this;
		}

		/**
		 * Tests to see whether or not this shared counter contains a valid reference
		 *
		 * @return  True if reference is valid
		 */
		FORCEINLINE const bool IsValid() const
		{
			return ReferenceController != nullptr;
		}

		/**
		 * Returns the number of shared references to this object (including this reference.)
		 *
		 * @return  Number of shared references to the object (including this reference.)
		 */
		FORCEINLINE const int32 GetSharedReferenceCount() const
		{
			return ReferenceController != nullptr ? ReferenceController->GetSharedReferenceCount() : 0;
		}

		/**
		 * Returns true if this is the only shared reference to this object.  Note that there may be
		 * outstanding weak references left.
		 *
		 * @return  True if there is only one shared reference to the object, and this is it!
		 */
		FORCEINLINE const bool IsUnique() const
		{
			return GetSharedReferenceCount() == 1;
		}

	private:

 		// Expose access to ReferenceController to FWeakReferencer
		template< ESPMode OtherMode > friend class FWeakReferencer;

	private:

		/** Pointer to the reference controller for the object a shared reference/pointer is referencing */
		TReferenceControllerBase<Mode>* ReferenceController;
	};


	/**
	 * FWeakReferencer is a wrapper around a pointer to a reference controller that is used
	 * by a TWeakPtr to keep track of a referenced object's lifetime.
	 */
	template< ESPMode Mode >
	class FWeakReferencer
	{
	public:

		/** Default constructor with empty counter */
		FORCEINLINE FWeakReferencer()
			: ReferenceController( nullptr )
		{ }

		/** Construct a weak referencer object from another weak referencer */
		FORCEINLINE FWeakReferencer( FWeakReferencer const& InWeakRefCountPointer )
			: ReferenceController( InWeakRefCountPointer.ReferenceController )
		{
			// If the weak referencer has a valid controller, then go ahead and add a weak reference to it!
			if( ReferenceController != nullptr )
			{
				ReferenceController->AddWeakReference();
			}
		}

		/** Construct a weak referencer object from an rvalue weak referencer */
		FORCEINLINE FWeakReferencer( FWeakReferencer&& InWeakRefCountPointer )
			: ReferenceController( InWeakRefCountPointer.ReferenceController )
		{
			InWeakRefCountPointer.ReferenceController = nullptr;
		}

		/** Construct a weak referencer object from a shared referencer object */
		FORCEINLINE FWeakReferencer( FSharedReferencer< Mode > const& InSharedRefCountPointer )
			: ReferenceController( InSharedRefCountPointer.ReferenceController )
		{
			// If the shared referencer had a valid controller, then go ahead and add a weak reference to it!
			if( ReferenceController != nullptr )
			{
				ReferenceController->AddWeakReference();
			}
		}

		/** Destructor. */
		FORCEINLINE ~FWeakReferencer()
		{
			if( ReferenceController != nullptr )
			{
				// Tell the reference counter object that we're no longer referencing the object with
				// this weak pointer
				ReferenceController->ReleaseWeakReference();
			}
		}
		
		/** Assignment operator from a weak referencer object.  If this counter was previously referencing an
		    object, that reference will be released. */
		FORCEINLINE FWeakReferencer& operator=( FWeakReferencer const& InWeakReference )
		{
			AssignReferenceController( InWeakReference.ReferenceController );

			return *this;
		}

		/** Assignment operator from an rvalue weak referencer object.  If this counter was previously referencing an
		    object, that reference will be released. */
		FORCEINLINE FWeakReferencer& operator=( FWeakReferencer&& InWeakReference )
		{
			auto OldReferenceController         = ReferenceController;
			ReferenceController                 = InWeakReference.ReferenceController;
			InWeakReference.ReferenceController = nullptr;
			if( OldReferenceController != nullptr )
			{
				OldReferenceController->ReleaseWeakReference();
			}

			return *this;
		}

		/** Assignment operator from a shared reference counter.  If this counter was previously referencing an
		   object, that reference will be released. */
		FORCEINLINE FWeakReferencer& operator=( FSharedReferencer< Mode > const& InSharedReference )
		{
			AssignReferenceController( InSharedReference.ReferenceController );

			return *this;
		}

		/**
		 * Tests to see whether or not this weak counter contains a valid reference
		 *
		 * @return  True if reference is valid
		 */
		FORCEINLINE const bool IsValid() const
		{
			return ReferenceController != nullptr && ReferenceController->GetSharedReferenceCount() > 0;
		}

	private:

		/** Assigns a new reference controller to this counter object, first adding a reference to it, then
		    releasing the previous object. */
		inline void AssignReferenceController( TReferenceControllerBase<Mode>* NewReferenceController )
		{
			// Only proceed if the new reference counter is different than our current
			if( NewReferenceController != ReferenceController )
			{
				// First, add a weak reference to the new object
				if( NewReferenceController != nullptr )
				{
					NewReferenceController->AddWeakReference();
				}

				// Release weak reference to the old object
				if( ReferenceController != nullptr )
				{
					ReferenceController->ReleaseWeakReference();
				}

				// Assume ownership of the assigned reference counter
				ReferenceController = NewReferenceController;
			}
		}

	private:

 		/** Expose access to ReferenceController to FSharedReferencer. */
		template< ESPMode OtherMode > friend class FSharedReferencer;

	private:

		/** Pointer to the reference controller for the object a TWeakPtr is referencing */
		TReferenceControllerBase<Mode>* ReferenceController;
	};


	/** Templated helper function (const) that creates a shared pointer from an object instance */
	template< class SharedPtrType, class ObjectType, class OtherType, ESPMode Mode >
	FORCEINLINE void EnableSharedFromThis( TSharedPtr< SharedPtrType, Mode > const* InSharedPtr, ObjectType const* InObject, TSharedFromThis< OtherType, Mode > const* InShareable )
	{
		if( InShareable != nullptr )
		{
			InShareable->UpdateWeakReferenceInternal( InSharedPtr, const_cast< ObjectType* >( InObject ) );
		}
	}


	/** Templated helper function that creates a shared pointer from an object instance */
	template< class SharedPtrType, class ObjectType, class OtherType, ESPMode Mode >
	FORCEINLINE void EnableSharedFromThis( TSharedPtr< SharedPtrType, Mode >* InSharedPtr, ObjectType const* InObject, TSharedFromThis< OtherType, Mode > const* InShareable )
	{
		if( InShareable != nullptr )
		{
			InShareable->UpdateWeakReferenceInternal( InSharedPtr, const_cast< ObjectType* >( InObject ) );
		}
	}


	/** Templated helper function (const) that creates a shared reference from an object instance */
	template< class SharedRefType, class ObjectType, class OtherType, ESPMode Mode >
	FORCEINLINE void EnableSharedFromThis( TSharedRef< SharedRefType, Mode > const* InSharedRef, ObjectType const* InObject, TSharedFromThis< OtherType, Mode > const* InShareable )
	{
		if( InShareable != nullptr )
		{
			InShareable->UpdateWeakReferenceInternal( InSharedRef, const_cast< ObjectType* >( InObject ) );
		}
	}


	/** Templated helper function that creates a shared reference from an object instance */
	template< class SharedRefType, class ObjectType, class OtherType, ESPMode Mode >
	FORCEINLINE void EnableSharedFromThis( TSharedRef< SharedRefType, Mode >* InSharedRef, ObjectType const* InObject, TSharedFromThis< OtherType, Mode > const* InShareable )
	{
		if( InShareable != nullptr )
		{
			InShareable->UpdateWeakReferenceInternal( InSharedRef, const_cast< ObjectType* >( InObject ) );
		}
	}


	/** Templated helper catch-all function, accomplice to the above helper functions */
	FORCEINLINE void EnableSharedFromThis( ... ) { }
}
