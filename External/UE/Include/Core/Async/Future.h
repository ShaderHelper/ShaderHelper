// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Templates/UnrealTemplate.h"
#include "Templates/Function.h"
#include "Misc/Timespan.h"
#include "Templates/SharedPointer.h"
#include "Misc/DateTime.h"
#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ScopeLock.h"

/**
 * Base class for the internal state of asynchronous return values (futures).
 */
class FFutureState
{
public:

	/** Default constructor. */
	FFutureState()
		: CompletionEvent(FPlatformProcess::GetSynchEventFromPool(true))
		, Complete(false)
	{ }

	/**
	 * Create and initialize a new instance with a callback.
	 *
	 * @param InCompletionCallback A function that is called when the state is completed.
	 */
	FFutureState(TUniqueFunction<void()>&& InCompletionCallback)
		: CompletionCallback(MoveTemp(InCompletionCallback))
		, CompletionEvent(FPlatformProcess::GetSynchEventFromPool(true))
		, Complete(false)
	{ }

	/** Destructor. */
	~FFutureState()
	{
		FPlatformProcess::ReturnSynchEventToPool(CompletionEvent);
		CompletionEvent = nullptr;
	}

public:

	/**
	 * Checks whether the asynchronous result has been set.
	 *
	 * @return true if the result has been set, false otherwise.
	 * @see WaitFor
	 */
	bool IsComplete() const
	{
		return Complete;
	}

	/**
	 * Blocks the calling thread until the future result is available.
	 *
	 * @param Duration The maximum time span to wait for the future result.
	 * @return true if the result is available, false otherwise.
	 * @see IsComplete
	 */
	bool WaitFor(const FTimespan& Duration) const
	{
		if (CompletionEvent->Wait(Duration))
		{
			return true;
		}

		return false;
	}

	/** 
	 * Set a continuation to be called on completion of the promise
	 * @param Continuation 
	 */
	void SetContinuation(TUniqueFunction<void()>&& Continuation)
	{
		bool bShouldJustRun = IsComplete();
		if (!bShouldJustRun)
		{
			FScopeLock Lock(&Mutex);
			bShouldJustRun = IsComplete();
			if (!bShouldJustRun)
			{
				CompletionCallback = MoveTemp(Continuation);
			}
		}
		if (bShouldJustRun && Continuation)
		{
			Continuation();
		}
	}

protected:

	/** Notifies any waiting threads that the result is available. */
	void MarkComplete()
	{
		TUniqueFunction<void()> Continuation;
		{
			FScopeLock Lock(&Mutex);
			Continuation = MoveTemp(CompletionCallback);
			Complete = true;
		}
		CompletionEvent->Trigger();

		if (Continuation)
		{
			Continuation();
		}
	}

private:
	/** Mutex used to allow proper handling of continuations */
	mutable FCriticalSection Mutex;

	/** An optional callback function that is executed the state is completed. */
	TUniqueFunction<void()> CompletionCallback;

	/** Holds an event signaling that the result is available. */
	FEvent* CompletionEvent;

	/** Whether the asynchronous result is available. */
	TAtomic<bool> Complete; 
};


/**
 * Implements the internal state of asynchronous return values (futures).
 */
template<typename InternalResultType>
class TFutureState
	: public FFutureState
{
public:

	/** Default constructor. */
	TFutureState()
		: FFutureState()
	{ }

	~TFutureState()
	{
		if (IsComplete())
		{
			DestructItem(Result.GetTypedPtr());
		}
	}

	/**
	 * Create and initialize a new instance with a callback.
	 *
	 * @param CompletionCallback A function that is called when the state is completed.
	 */
	TFutureState(TUniqueFunction<void()>&& CompletionCallback)
		: FFutureState(MoveTemp(CompletionCallback))
	{ }

public:

	/**
	 * Gets the result (will block the calling thread until the result is available).
	 *
	 * @return The result value.
	 * @see EmplaceResult
	 */
	const InternalResultType& GetResult() const
	{
		while (!IsComplete())
		{
			WaitFor(FTimespan::MaxValue());
		}

		return *Result.GetTypedPtr();
	}

	/**
	 * Sets the result and notifies any waiting threads.
	 *
	 * @param InResult The result to set.
	 * @see GetResult
	 */
	template<typename... ArgTypes>
	void EmplaceResult(ArgTypes&&... Args)
	{
		check(!IsComplete());
		new(Result.GetTypedPtr()) InternalResultType(Forward<ArgTypes>(Args)...);
		MarkComplete();
	}

private:

	/** Holds the asynchronous result. */
	TTypeCompatibleBytes<InternalResultType> Result;
};

/* TFuture
*****************************************************************************/

/**
 * Abstract base template for futures and shared futures.
 */
template<typename InternalResultType>
class TFutureBase
{
public:

	/**
	 * Checks whether this future object has its value set.
	 *
	 * @return true if this future has a shared state and the value has been set, false otherwise.
	 * @see IsValid
	 */
	bool IsReady() const
	{
		return State.IsValid() ? State->IsComplete() : false;
	}

	/**
	 * Checks whether this future object has a valid state.
	 *
	 * @return true if the state is valid, false otherwise.
	 * @see IsReady
	 */
	bool IsValid() const
	{
		return State.IsValid();
	}

	/**
	 * Blocks the calling thread until the future result is available.
	 *
	 * Note that this method may block forever if the result is never set. Use
	 * the WaitFor or WaitUntil methods to specify a maximum timeout for the wait.
	 *
	 * @see WaitFor, WaitUntil
	 */
	void Wait() const
	{
		if (State.IsValid())
		{
			while (!WaitFor(FTimespan::MaxValue()));
		}
	}

	/**
	 * Blocks the calling thread until the future result is available or the specified duration is exceeded.
	 *
	 * @param Duration The maximum time span to wait for the future result.
	 * @return true if the result is available, false otherwise.
	 * @see Wait, WaitUntil
	 */
	bool WaitFor(const FTimespan& Duration) const
	{
		return State.IsValid() ? State->WaitFor(Duration) : false;
	}

	/**
	 * Blocks the calling thread until the future result is available or the specified time is hit.
	 *
	 * @param Time The time until to wait for the future result (in UTC).
	 * @return true if the result is available, false otherwise.
	 * @see Wait, WaitUntil
	 */
	bool WaitUntil(const FDateTime& Time) const
	{
		return WaitFor(Time - FDateTime::UtcNow());
	}

protected:

	typedef TSharedPtr<TFutureState<InternalResultType>, ESPMode::ThreadSafe> StateType;

	/** Default constructor. */
	TFutureBase() { }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InState The shared state to initialize with.
	 */
	TFutureBase(const StateType& InState)
		: State(InState)
	{ }

	/**
	 * Protected move construction
	 */
	TFutureBase(TFutureBase&&) = default;

	/**
	 * Protected move assignment
	 */
	TFutureBase& operator=(TFutureBase&&) = default;

	/**
	 * Protected copy construction
	 */
	TFutureBase(const TFutureBase&) = default;

	/**
	 * Protected copy assignment
	 */
	TFutureBase& operator=(const TFutureBase&) = default;

	/** Protected destructor. */
	~TFutureBase() { }

protected:

	/**
	 * Gets the shared state object.
	 *
	 * @return The shared state object.
	 */
	const StateType& GetState() const
	{
		// if you hit this assertion then your future has an invalid state.
		// this happens if you have an uninitialized future or if you moved
		// it to another instance.
		check(State.IsValid());

		return State;
	}

	/**
	 * Set a completion callback that will be called once the future completes
	 *	or immediately if already completed
	 *
	 * @param Continuation a continuation taking an argument of type TFuture<InternalResultType>
	 * @return nothing at the moment but could return another future to allow future chaining
	 */
	template<typename Func>
	auto Then(Func Continuation);

	/**
	 * Convenience wrapper for Then that
	 *	set a completion callback that will be called once the future completes
	 *	or immediately if already completed
	 * @param Continuation a continuation taking an argument of type InternalResultType
	 * @return nothing at the moment but could return another future to allow future chaining
	 */
	template<typename Func>
	auto Next(Func Continuation);

	/**
	 * Reset the future.
	 *	Reseting a future removes any continuation from its shared state and invalidates it.
	 *	Useful for discarding yet to be completed future cleanly.
	 */
	void Reset()
	{
		if (IsValid())
		{
			this->State->SetContinuation(nullptr);
			this->State.Reset();
		}
	}

private:

	/** Holds the future's state. */
	StateType State;
};


template<typename ResultType> class TSharedFuture;

/**
 * Template for unshared futures.
 */
template<typename ResultType>
class TFuture
	: public TFutureBase<ResultType>
{
	typedef TFutureBase<ResultType> BaseType;

public:

	/** Default constructor. */
	TFuture() { }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InState The shared state to initialize with.
	 */
	TFuture(const typename BaseType::StateType& InState)
		: BaseType(InState)
	{ }

	/**
	 * Move constructor.
	 */
	TFuture(TFuture&&) = default;

	/**
	 * Move assignment operator.
	 */
	TFuture& operator=(TFuture&& Other) = default;

	/** Destructor. */
	~TFuture() { }

public:

	/**
	 * Gets the future's result.
	 *
	 * @return The result.
	 */
	ResultType Get() const
	{
		return this->GetState()->GetResult();
	}

	/**
	 * Moves this future's state into a shared future.
	 *
	 * @return The shared future object.
	 */
	TSharedFuture<ResultType> Share()
	{
		return TSharedFuture<ResultType>(MoveTemp(*this));
	}

	/**
	 * Expose Then functionality
	 * @see TFutureBase 
	 */
	using BaseType::Then;

	/**
	 * Expose Next functionality
	 * @see TFutureBase
	 */
	using BaseType::Next;

	/**
	 * Expose Reset functionality
	 * @see TFutureBase
	 */
	using BaseType::Reset;

private:

	/** Hidden copy constructor (futures cannot be copied). */
	TFuture(const TFuture&);

	/** Hidden copy assignment (futures cannot be copied). */
	TFuture& operator=(const TFuture&);
};


/**
 * Template for unshared futures (specialization for reference types).
 */
template<typename ResultType>
class TFuture<ResultType&>
	: public TFutureBase<ResultType*>
{
	typedef TFutureBase<ResultType*> BaseType;

public:

	/** Default constructor. */
	TFuture() { }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InState The shared state to initialize with.
	 */
	TFuture(const typename BaseType::StateType& InState)
		: BaseType(InState)
	{ }

	/**
	 * Move constructor.
	 */
	TFuture(TFuture&&) = default;

	/**
	 * Move assignment operator.
	 */
	TFuture& operator=(TFuture&& Other) = default;

	/** Destructor. */
	~TFuture() { }

public:

	/**
	 * Gets the future's result.
	 *
	 * @return The result.
	 */
	ResultType& Get() const
	{
		return *this->GetState()->GetResult();
	}

	/**
	 * Moves this future's state into a shared future.
	 *
	 * @return The shared future object.
	 */
	TSharedFuture<ResultType&> Share()
	{
		return TSharedFuture<ResultType&>(MoveTemp(*this));
	}

	/**
	 * Expose Then functionality
	 * @see TFutureBase
	 */
	using BaseType::Then;

	/**
	 * Expose Next functionality
	 * @see TFutureBase
	 */
	using BaseType::Next;

	/**
	 * Expose Reset functionality
	 * @see TFutureBase
	 */
	using BaseType::Reset;

private:

	/** Hidden copy constructor (futures cannot be copied). */
	TFuture(const TFuture&);

	/** Hidden copy assignment (futures cannot be copied). */
	TFuture& operator=(const TFuture&);
};


/**
 * Template for unshared futures (specialization for void).
 */
template<>
class TFuture<void>
	: public TFutureBase<int>
{
	typedef TFutureBase<int> BaseType;

public:

	/** Default constructor. */
	TFuture() { }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InState The shared state to initialize with.
	 */
	TFuture(const BaseType::StateType& InState)
		: BaseType(InState)
	{ }

	/**
	 * Move constructor.
	 */
	TFuture(TFuture&&) = default;

	/**
	 * Move assignment operator.
	 */
	TFuture& operator=(TFuture&& Other) = default;

	/** Destructor. */
	~TFuture() { }

public:

	/**
	 * Gets the future's result.
	 *
	 * @return The result.
	 */
	void Get() const
	{
		GetState()->GetResult();
	}

	/**
	 * Moves this future's state into a shared future.
	 *
	 * @return The shared future object.
	 */
	TSharedFuture<void> Share();

	/**
	 * Expose Then functionality
	 * @see TFutureBase
	 */
	using BaseType::Then;

	/**
	 * Expose Next functionality
	 * @see TFutureBase
	 */
	using BaseType::Next;

	/**
	 * Expose Reset functionality
	 * @see TFutureBase
	 */
	using BaseType::Reset;

private:

	/** Hidden copy constructor (futures cannot be copied). */
	TFuture(const TFuture&);

	/** Hidden copy assignment (futures cannot be copied). */
	TFuture& operator=(const TFuture&);
};

/* TSharedFuture
*****************************************************************************/

/**
 * Template for shared futures.
 */
template<typename ResultType>
class TSharedFuture
	: public TFutureBase<ResultType>
{
	typedef TFutureBase<ResultType> BaseType;

public:

	/** Default constructor. */
	TSharedFuture() { }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InState The shared state to initialize from.
	 */
	TSharedFuture(const typename BaseType::StateType& InState)
		: BaseType(InState)
	{ }

	/**
	 * Creates and initializes a new instances from a future object.
	 *
	 * @param Future The future object to initialize from.
	 */
	TSharedFuture(TFuture<ResultType>&& Future)
		: BaseType(MoveTemp(Future))
	{ }

	/**
	 * Copy constructor.
	 */
	TSharedFuture(const TSharedFuture&) = default;

	/**
	 * Copy assignment operator.
	 */
	TSharedFuture& operator=(const TSharedFuture& Other) = default;

	/**
	 * Move constructor.
	 */
	TSharedFuture(TSharedFuture&&) = default;

	/**
	 * Move assignment operator.
	 */
	TSharedFuture& operator=(TSharedFuture&& Other) = default;

	/** Destructor. */
	~TSharedFuture() { }

public:

	/**
	 * Gets the future's result.
	 *
	 * @return The result.
	 */
	ResultType Get() const
	{
		return this->GetState()->GetResult();
	}
};


/**
 * Template for shared futures (specialization for reference types).
 */
template<typename ResultType>
class TSharedFuture<ResultType&>
	: public TFutureBase<ResultType*>
{
	typedef TFutureBase<ResultType*> BaseType;

public:

	/** Default constructor. */
	TSharedFuture() { }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InState The shared state to initialize from.
	 */
	TSharedFuture(const typename BaseType::StateType& InState)
		: BaseType(InState)
	{ }

	/**
	* Creates and initializes a new instances from a future object.
	*
	* @param Future The future object to initialize from.
	*/
	TSharedFuture(TFuture<ResultType>&& Future)
		: BaseType(MoveTemp(Future))
	{ }

	/**
	 * Copy constructor.
	 */
	TSharedFuture(const TSharedFuture&) = default;

	/** Copy assignment operator. */
	TSharedFuture& operator=(const TSharedFuture& Other) = default;

	/**
	 * Move constructor.
	 */
	TSharedFuture(TSharedFuture&&) = default;

	/**
	 * Move assignment operator.
	 */
	TSharedFuture& operator=(TSharedFuture&& Other) = default;

	/** Destructor. */
	~TSharedFuture() { }

public:

	/**
	 * Gets the future's result.
	 *
	 * @return The result.
	 */
	ResultType& Get() const
	{
		return *this->GetState()->GetResult();
	}
};


/**
 * Template for shared futures (specialization for void).
 */
template<>
class TSharedFuture<void>
	: public TFutureBase<int>
{
	typedef TFutureBase<int> BaseType;

public:

	/** Default constructor. */
	TSharedFuture() { }

	/**
	 * Creates and initializes a new instance from shared state.
	 *
	 * @param InState The shared state to initialize from.
	 */
	TSharedFuture(const BaseType::StateType& InState)
		: BaseType(InState)
	{ }

	/**
	 * Creates and initializes a new instances from a future object.
	 *
	 * @param Future The future object to initialize from.
	 */
	TSharedFuture(TFuture<void>&& Future)
		: BaseType(MoveTemp(Future))
	{ }

	/**
	 * Copy constructor.
	 */
	TSharedFuture(const TSharedFuture&) = default;

	/**
	 * Copy assignment operator.
	 */
	TSharedFuture& operator=(const TSharedFuture& Other) = default;

	/**
	 * Move constructor.
	 */
	TSharedFuture(TSharedFuture&&) = default;

	/**
	 * Move assignment operator.
	 */
	TSharedFuture& operator=(TSharedFuture&& Other) = default;

	/** Destructor. */
	~TSharedFuture() { }

public:

	/**
	 * Gets the future's result.
	 *
	 * @return The result.
	 */
	void Get() const
	{
		GetState()->GetResult();
	}
};


inline TSharedFuture<void> TFuture<void>::Share()
{
	return TSharedFuture<void>(MoveTemp(*this));
}


/* TPromise
*****************************************************************************/

template<typename InternalResultType>
class TPromiseBase
	: FNoncopyable
{
	typedef TSharedPtr<TFutureState<InternalResultType>, ESPMode::ThreadSafe> StateType;

public:

	/** Default constructor. */
	TPromiseBase()
		: State(MakeShared<TFutureState<InternalResultType>, ESPMode::ThreadSafe>())
	{ }

	/**
	 * Move constructor.
	 *
	 * @param Other The promise holding the shared state to move.
	 */
	TPromiseBase(TPromiseBase&& Other)
		: State(MoveTemp(Other.State))
	{
		Other.State.Reset();
	}

	/**
	 * Create and initialize a new instance with a callback.
	 *
	 * @param CompletionCallback A function that is called when the future state is completed.
	 */
	TPromiseBase(TUniqueFunction<void()>&& CompletionCallback)
		: State(MakeShared<TFutureState<InternalResultType>, ESPMode::ThreadSafe>(MoveTemp(CompletionCallback)))
	{ }

public:

	/** Move assignment operator. */
	TPromiseBase& operator=(TPromiseBase&& Other)
	{
		State = Other.State;
		Other.State.Reset();
		return *this;
	}

protected:

	/** Destructor. */
	~TPromiseBase()
	{
		if (State.IsValid())
		{
			// if you hit this assertion then your promise never had its result
			// value set. broken promises are considered programming errors.
			check(State->IsComplete());
		}
	}

	/**
	 * Gets the shared state object.
	 *
	 * @return The shared state object.
	 */
	const StateType& GetState()
	{
		// if you hit this assertion then your promise has an invalid state.
		// this happens if you move the promise to another instance.
		check(State.IsValid());

		return State;
	}

private:

	/** Holds the shared state object. */
	StateType State;
};


/**
 * Template for promises.
 */
template<typename ResultType>
class TPromise
	: public TPromiseBase<ResultType>
{
public:

	typedef TPromiseBase<ResultType> BaseType;

	/** Default constructor (creates a new shared state). */
	TPromise()
		: BaseType()
		, FutureRetrieved(false)
	{ }

	/**
	 * Move constructor.
	 *
	 * @param Other The promise holding the shared state to move.
	 */
	TPromise(TPromise&& Other)
		: BaseType(MoveTemp(Other))
		, FutureRetrieved(MoveTemp(Other.FutureRetrieved))
	{ }

	/**
	 * Create and initialize a new instance with a callback.
	 *
	 * @param CompletionCallback A function that is called when the future state is completed.
	 */
	TPromise(TUniqueFunction<void()>&& CompletionCallback)
		: BaseType(MoveTemp(CompletionCallback))
		, FutureRetrieved(false)
	{ }

public:

	/**
	 * Move assignment operator.
	 *
	 * @param Other The promise holding the shared state to move.
	 */
	TPromise& operator=(TPromise&& Other)
	{
		BaseType::operator=(MoveTemp(Other));
		FutureRetrieved = MoveTemp(Other.FutureRetrieved);

		return *this;
	}

public:

	/**
	 * Gets a TFuture object associated with the shared state of this promise.
	 *
	 * @return The TFuture object.
	 */
	TFuture<ResultType> GetFuture()
	{
		check(!FutureRetrieved);
		FutureRetrieved = true;

		return TFuture<ResultType>(this->GetState());
	}

	/**
	 * Sets the promised result.
	 *
	 * The result must be set only once. An assertion will
	 * be triggered if this method is called a second time.
	 *
	 * @param Result The result value to set.
	 */
	FORCEINLINE void SetValue(const ResultType& Result)
	{
		EmplaceValue(Result);
	}

	/**
	 * Sets the promised result (from rvalue).
	 *
	 * The result must be set only once. An assertion will
	 * be triggered if this method is called a second time.
	 *
	 * @param Result The result value to set.
	 */
	FORCEINLINE void SetValue(ResultType&& Result)
	{
		EmplaceValue(MoveTemp(Result));
	}

	/**
	 * Sets the promised result.
	 *
	 * The result must be set only once. An assertion will
	 * be triggered if this method is called a second time.
	 *
	 * @param Result The result value to set.
	 */
	template<typename... ArgTypes>
	void EmplaceValue(ArgTypes&&... Args)
	{
		this->GetState()->EmplaceResult(Forward<ArgTypes>(Args)...);
	}

private:

	/** Whether a future has already been retrieved from this promise. */
	bool FutureRetrieved;
};


/**
 * Template for promises (specialization for reference types).
 */
template<typename ResultType>
class TPromise<ResultType&>
	: public TPromiseBase<ResultType*>
{
	typedef TPromiseBase<ResultType*> BaseType;

public:

	/** Default constructor (creates a new shared state). */
	TPromise()
		: BaseType()
		, FutureRetrieved(false)
	{ }

	/**
	 * Move constructor.
	 *
	 * @param Other The promise holding the shared state to move.
	 */
	TPromise(TPromise&& Other)
		: BaseType(MoveTemp(Other))
		, FutureRetrieved(MoveTemp(Other.FutureRetrieved))
	{ }

	/**
	 * Create and initialize a new instance with a callback.
	 *
	 * @param CompletionCallback A function that is called when the future state is completed.
	 */
	TPromise(TUniqueFunction<void()>&& CompletionCallback)
		: BaseType(MoveTemp(CompletionCallback))
		, FutureRetrieved(false)
	{ }

public:

	/**
	 * Move assignment operator.
	 *
	 * @param Other The promise holding the shared state to move.
	 */
	TPromise& operator=(TPromise&& Other)
	{
		BaseType::operator=(MoveTemp(Other));
		FutureRetrieved = MoveTemp(Other.FutureRetrieved);

		return this;
	}

public:

	/**
	 * Gets a TFuture object associated with the shared state of this promise.
	 *
	 * @return The TFuture object.
	 */
	TFuture<ResultType&> GetFuture()
	{
		check(!FutureRetrieved);
		FutureRetrieved = true;

		return TFuture<ResultType&>(this->GetState());
	}

	/**
	 * Sets the promised result.
	 *
	 * The result must be set only once. An assertion will
	 * be triggered if this method is called a second time.
	 *
	 * @param Result The result value to set.
	 */
	FORCEINLINE void SetValue(ResultType& Result)
	{
		EmplaceValue(Result);
	}

	/**
	 * Sets the promised result.
	 *
	 * The result must be set only once. An assertion will
	 * be triggered if this method is called a second time.
	 *
	 * @param Result The result value to set.
	 */
	void EmplaceValue(ResultType& Result)
	{
		this->GetState()->EmplaceResult(&Result);
	}

private:

	/** Whether a future has already been retrieved from this promise. */
	bool FutureRetrieved;
};


/**
 * Template for promises (specialization for void results).
 */
template<>
class TPromise<void>
	: public TPromiseBase<int>
{
	typedef TPromiseBase<int> BaseType;

public:

	/** Default constructor (creates a new shared state). */
	TPromise()
		: BaseType()
		, FutureRetrieved(false)
	{ }

	/**
	 * Move constructor.
	 *
	 * @param Other The promise holding the shared state to move.
	 */
	TPromise(TPromise&& Other)
		: BaseType(MoveTemp(Other))
		, FutureRetrieved(false)
	{ }

	/**
	 * Create and initialize a new instance with a callback.
	 *
	 * @param CompletionCallback A function that is called when the future state is completed.
	 */
	TPromise(TUniqueFunction<void()>&& CompletionCallback)
		: BaseType(MoveTemp(CompletionCallback))
		, FutureRetrieved(false)
	{ }

public:

	/**
	 * Move assignment operator.
	 *
	 * @param Other The promise holding the shared state to move.
	 */
	TPromise& operator=(TPromise&& Other)
	{
		BaseType::operator=(MoveTemp(Other));
		FutureRetrieved = MoveTemp(Other.FutureRetrieved);

		return *this;
	}

public:

	/**
	 * Gets a TFuture object associated with the shared state of this promise.
	 *
	 * @return The TFuture object.
	 */
	TFuture<void> GetFuture()
	{
		check(!FutureRetrieved);
		FutureRetrieved = true;

		return TFuture<void>(GetState());
	}

	/**
	 * Sets the promised result.
	 *
	 * The result must be set only once. An assertion will
	 * be triggered if this method is called a second time.
	 */
	FORCEINLINE void SetValue()
	{
		EmplaceValue();
	}

	/**
	 * Sets the promised result.
	 *
	 * The result must be set only once. An assertion will
	 * be triggered if this method is called a second time.
	 *
	 * @param Result The result value to set.
	 */
	void EmplaceValue()
	{
		this->GetState()->EmplaceResult(0);
	}

private:

	/** Whether a future has already been retrieved from this promise. */
	bool FutureRetrieved;
};

/* TFuture::Then
*****************************************************************************/

namespace FutureDetail
{
	/**
	* Template for setting a promise value from a continuation.
	*/
	template<typename Func, typename ParamType, typename ResultType>
	inline void SetPromiseValue(TPromise<ResultType>& Promise, Func& Function, TFuture<ParamType>&& Param)
	{
		Promise.SetValue(Function(MoveTemp(Param)));
	}
	template<typename Func, typename ParamType>
	inline void SetPromiseValue(TPromise<void>& Promise, Func& Function, TFuture<ParamType>&& Param)
	{
		Function(MoveTemp(Param));
		Promise.SetValue();
	}
}

// Then implementation
template<typename InternalResultType>
template<typename Func>
auto TFutureBase<InternalResultType>::Then(Func Continuation) //-> TFuture<decltype(Continuation(MoveTemp(TFuture<InternalResultType>())))>
{
	check(IsValid());
	using ReturnValue = decltype(Continuation(MoveTemp(TFuture<InternalResultType>())));

	TPromise<ReturnValue> Promise;
	TFuture<ReturnValue> FutureResult = Promise.GetFuture();
	TUniqueFunction<void()> Callback = [PromiseCapture = MoveTemp(Promise), ContinuationCapture = MoveTemp(Continuation), StateCapture = this->State]() mutable
	{
		FutureDetail::SetPromiseValue(PromiseCapture, ContinuationCapture, TFuture<InternalResultType>(MoveTemp(StateCapture)));
	};

	// This invalidate this future.
	StateType MovedState = MoveTemp(this->State);
	MovedState->SetContinuation(MoveTemp(Callback));
	return FutureResult;
}

// Next implementation
template<typename InternalResultType>
template<typename Func>
auto TFutureBase<InternalResultType>::Next(Func Continuation) //-> TFuture<decltype(Continuation(Get()))>
{
	return this->Then([Continuation = MoveTemp(Continuation)](TFuture<InternalResultType> Self) mutable
	{
		return Continuation(Self.Get());
	});
}

/** Helper to create and immediately fulfill a promise */
template<typename ResultType, typename... ArgTypes>
TPromise<ResultType> MakeFulfilledPromise(ArgTypes&&... Args)
{
	TPromise<ResultType> Promise;
	Promise.EmplaceValue(Forward<ArgTypes>(Args)...);
	return Promise;
}
