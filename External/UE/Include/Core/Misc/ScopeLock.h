// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/CriticalSection.h"

/**
 * Implements a scope lock.
 *
 * This is a utility class that handles scope level locking. It's very useful
 * to keep from causing deadlocks due to exceptions being caught and knowing
 * about the number of locks a given thread has on a resource. Example:
 *
 * <code>
 *	{
 *		// Synchronize thread access to the following data
 *		FScopeLock ScopeLock(SynchObject);
 *		// Access data that is shared among multiple threads
 *		...
 *		// When ScopeLock goes out of scope, other threads can access data
 *	}
 * </code>
 */
class FScopeLock
{
public:

	/**
	 * Constructor that performs a lock on the synchronization object
	 *
	 * @param InSynchObject The synchronization object to manage
	 */
	FScopeLock( FCriticalSection* InSynchObject )
		: SynchObject(InSynchObject)
	{
		check(SynchObject);
		SynchObject->Lock();
	}

	/** Destructor that performs a release on the synchronization object. */
	~FScopeLock()
	{
		Unlock();
	}

	void Unlock()
	{
		if(SynchObject)
		{
			SynchObject->Unlock();
			SynchObject = nullptr;
		}
	}

private:

	/** Default constructor (hidden on purpose). */
	FScopeLock();

	/** Copy constructor( hidden on purpose). */
	FScopeLock(const FScopeLock& InScopeLock);

	/** Assignment operator (hidden on purpose). */
	FScopeLock& operator=( FScopeLock& InScopeLock )
	{
		return *this;
	}

private:

	// Holds the synchronization object to aggregate and scope manage.
	FCriticalSection* SynchObject;
};

/**
 * Implements a scope unlock.
 *
 * This is a utility class that handles scope level unlocking. It's very useful
 * to allow access to a protected object when you are sure it can happen.
 * Example:
 *
 * <code>
 *	{
 *		// Access data that is shared among multiple threads
 *		FScopeUnlock ScopeUnlock(SynchObject);
 *		...
 *		// When ScopeUnlock goes out of scope, other threads can no longer access data
 *	}
 * </code>
 */
class FScopeUnlock
{
public:

	/**
	 * Constructor that performs a unlock on the synchronization object
	 *
	 * @param InSynchObject The synchronization object to manage, can be null.
	 */
	FScopeUnlock(FCriticalSection* InSynchObject)
		: SynchObject(InSynchObject)
	{
		if (InSynchObject)
		{
			InSynchObject->Unlock();
		}
	}

	/** Destructor that performs a lock on the synchronization object. */
	~FScopeUnlock()
	{
		if (SynchObject)
		{
			SynchObject->Lock();
		}
	}
private:

	/** Default constructor (hidden on purpose). */
	FScopeUnlock();

	/** Copy constructor( hidden on purpose). */
	FScopeUnlock(const FScopeUnlock& InScopeLock);

	/** Assignment operator (hidden on purpose). */
	FScopeUnlock& operator=(FScopeUnlock& InScopeLock)
	{
		return *this;
	}

private:

	// Holds the synchronization object to aggregate and scope manage.
	FCriticalSection* SynchObject;
};

namespace UE
{
	// RAII-style scope locking of a synchronisation primitive
	// `MutexType` is required to implement `Lock` and `Unlock` methods
	// Example:
	//	{
	//		TScopeLock<FCriticalSection> ScopeLock(CriticalSection);
	//		...
	//	}
	template<typename MutexType>
	class TScopeLock
	{
	public:
		UE_NONCOPYABLE(TScopeLock);

		TScopeLock(MutexType& InMutex)
			: Mutex(&InMutex)
		{
			check(Mutex);
			Mutex->Lock();
		}

		~TScopeLock()
		{
			Unlock();
		}

		void Unlock()
		{
			if (Mutex)
			{
				Mutex->Unlock();
				Mutex = nullptr;
			}
		}

	private:
		MutexType* Mutex;
	};

	// RAII-style scope unlocking of a synchronisation primitive
	// `MutexType` is required to implement `Lock` and `Unlock` methods
	// Example:
	//	{
	//		TScopeLock<FCriticalSection> ScopeLock(CriticalSection);
	//		for (FElementType& Element : ThreadUnsafeContainer)
	//		{
	//			TScopeUnlock<FCriticalSection> ScopeUnlock(CriticalSection);
	//			Process(Element);
	//		}
	//	}
	template<typename MutexType>
	class TScopeUnlock
	{
	public:
		UE_NONCOPYABLE(TScopeUnlock);

		TScopeUnlock(MutexType& InMutex)
			: Mutex(&InMutex)
		{
			check(Mutex);
			Mutex->Unlock();
		}

		~TScopeUnlock()
		{
			Mutex->Lock();
		}

	private:
		MutexType* Mutex;
	};
}