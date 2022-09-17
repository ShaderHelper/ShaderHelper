// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Delegates/IDelegateInstance.h"
#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"
#include "Containers/MpscQueue.h"

#include <atomic>

/**
 * Ticker delegates return true to automatically reschedule at the same delay or false for one-shot.
 * You will not get more than one fire per "frame", which is just a FTSTicker::Tick call.
 * DeltaTime argument is the time since the last game frame, *not* since the last tick the
 * delegate received.
 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FTickerDelegate, float);

/**
 * Thread-safe ticker class. Fires delegates after a delay.
 */
class CORE_API FTSTicker
{
private:
	struct FElement;

public:
	using FDelegateHandle = TWeakPtr<FElement>;

	/** Singleton used for the ticker in Core / Launch. If you add a new ticker for a different subsystem, do not put that singleton here! **/
	static FTSTicker& GetCoreTicker();

	/**
	 * Add a new ticker with a given delay / interval
	 *
	 * Can be called concurrently. Resources used by the delegate must be released on the ticking thread.
	 *
	 * @param InDelegate Delegate to fire after the delay
	 * @param InDelay Delay until next fire; 0 means "next frame"
	 */
	FDelegateHandle AddTicker(const FTickerDelegate & InDelegate, float InDelay = 0.0f);

	/**
	* Add a new ticker with a given delay / interval.
	* 
	* Can be called concurrently.
	*
	* @param InName Name of this ticker for profiling
	* @param InDelay Delay until next fire; 0 means "next frame"
	* @param Function Function to execute. Should return true to fire after another InDelay time
	*/
	FDelegateHandle AddTicker(const TCHAR * InName, float InDelay, TFunction<bool(float)> Function);

	/**
	 * Removes a previously added ticker delegate.
	 *
	 * Can be called concurrently. If is caleld in the middle of the delegate execution, it blocks until the execution finishes, and thus guarantees
	 * that the delegate won't be executed after the call.
	 *
	 * @param Handle The handle of the ticker to remove.
	 */
	static void RemoveTicker(FDelegateHandle Handle);

	/**
	 * Fire all tickers who have passed their delay and reschedule the ones that return true
	 * 
	 * Note: This reschedule has timer skew, meaning we always wait a full Delay period after
	 * each invocation even if we missed the last interval by a bit. For instance, setting a
	 * delay of 0.1 seconds will not necessarily fire the delegate 10 times per second, depending
	 * on the Tick() rate. What this DOES guarantee is that a delegate will never be called MORE
	 * FREQUENTLY than the Delay interval, regardless of how far we miss the scheduled interval.
	 * 
	 * Must not be called concurrently.
	 *
	 * @param DeltaTime	time that has passed since the last tick call
	 */
	void Tick(float DeltaTime);

	/**
	 * Resets the instance to its default state. Must be called from the ticking thread.
	 */
	void Reset();

private:
	/** Internal structure to store a ticker delegate and related data **/
	struct FElement
	{
		/** Time that this delegate must not fire before **/
		double FireTime;
		/** Delay that this delegate was scheduled with. Kept here so that if the delegate returns true, we will reschedule it. **/
		float DelayTime;
		/** Delegate to fire **/
		FTickerDelegate Delegate;

		static constexpr uint64 DefaultState = 0;
		static constexpr uint64 RemovedState = 1;
		// The element can be in 4 states: "idle", "executing", "idle and removed" and "executing and removed"
		// Often right after `RemoveTicket()` call resources that are used by the delegate are destroyed. We must ensure that 
		// these resources are not accessed by the delegate after `RemoveTicket()` returns.
		// `State` is used to remove the element safely, if removal happens in the middle of the delegate execution, removal will not return
		// until the execution is finished.
		// `State` constists of two 32 bits parts: lower part is 1 if the delegate is removed, otherwise it's 0. the higher part contains thread id if the
		// delegate being executed, otherwise it's 0
		std::atomic<uint64> State{ DefaultState };

		/** Default ctor is only required to implement CurrentElement handling without making it a pointer. */
		CORE_API FElement();
		/** This is the ctor that the code will generally use. */
		CORE_API FElement(double InFireTime, float InDelayTime, const FTickerDelegate& InDelegate);

		/** Fire the delegate if it is fireable **/
		CORE_API bool Fire(float DeltaTime);
	};

	using FElementPtr = TSharedPtr<FElement>;

	// all added delegates are initially stored in a separate thread-safe queue and then in the next Tick are moved to the main not thread-safe
	// container
	TMpscQueue<FElementPtr> AddedElements;

	/** Current time of the ticker **/
	double CurrentTime{ 0.0 };
	/** Future delegates to fire **/
	TArray<FElementPtr> Elements;
};

/**
 * Base class for thread-safe ticker objects
 */
class CORE_API FTSTickerObjectBase
{
public:
	UE_NONCOPYABLE(FTSTickerObjectBase);

	/**
	 * Constructor
	 *
	 * @param InDelay Delay until next fire; 0 means "next frame"
	 * @param Ticker the ticker to register with. Defaults to FTSTicker::GetCoreTicker().
	*/
	FTSTickerObjectBase(float InDelay = 0.0f, FTSTicker& Ticker = FTSTicker::GetCoreTicker());

	/** Virtual destructor. */
	virtual ~FTSTickerObjectBase();

	/**
	 * Pure virtual that must be overloaded by the inheriting class.
	 *
	 * @param DeltaTime	time passed since the last call.
	 * @return true if should continue ticking
	 */
	virtual bool Tick(float DeltaTime) = 0;

private:
	/** Delegate for callbacks to Tick */
	FTSTicker::FDelegateHandle TickHandle;
};

/**
 * DEPRECATED
 * Not thread-safe ticker class. Fires delegates after a delay.
 * 
 * Note: Do not try to add the same delegate instance twice, as there is no way to remove only a single instance (see member RemoveTicker).
 * 
 * Migration guide:
 * As FTicker is deprecated and replaced by thread-safe FTSTicker class, if you see a deprecation warning please migrate your code. Change
 * `FTicker` to `FTSTicker` and change the type of the delegate handle returned by `AddTicker` methods from `FDelegateHandle` to
 * `FTSTicker::FDelegateHandle`
 */
class UE_DEPRECATED(5.0, "Please use thread-safe FTSTicker, see the migration guide in the code comment above") FTicker
{
public:

	/** Singleton used for the ticker in Core / Launch. If you add a new ticker for a different subsystem, do not put that singleton here! **/
	CORE_API static FTicker& GetCoreTicker();
	CORE_API static void TearDownCoreTicker();

	CORE_API FTicker();
	CORE_API ~FTicker();

	/**
	 * Add a new ticker with a given delay / interval
	 * 
	 * @param InDelegate Delegate to fire after the delay
	 * @param InDelay Delay until next fire; 0 means "next frame"
	 */
	CORE_API FDelegateHandle AddTicker(const FTickerDelegate& InDelegate, float InDelay = 0.0f);	

	/**
	* Add a new ticker with a given delay / interval.
	*
	* @param InName Name of this ticker for profiling
	* @param InDelay Delay until next fire; 0 means "next frame"
	* @param Function Function to execute. Should return true to fire after another InDelay time
	*/
	CORE_API FDelegateHandle AddTicker(const TCHAR* InName, float InDelay, TFunction<bool(float)> Function);

	/**
	 * Removes a previously added ticker delegate.
	 * 
	 * Note: will remove ALL tickers that use this handle, as there's no way to uniquely identify which one you are trying to remove.
	 *
	 * @param Handle The handle of the ticker to remove.
	 */
	CORE_API void RemoveTicker(FDelegateHandle Handle);

	/**
	 * Fire all tickers who have passed their delay and reschedule the ones that return true
	 * 
	 * Note: This reschedule has timer skew, meaning we always wait a full Delay period after 
	 * each invocation even if we missed the last interval by a bit. For instance, setting a 
	 * delay of 0.1 seconds will not necessarily fire the delegate 10 times per second, depending 
	 * on the Tick() rate. What this DOES guarantee is that a delegate will never be called MORE
	 * FREQUENTLY than the Delay interval, regardless of how far we miss the scheduled interval.
	 * 
	 * @param DeltaTime	time that has passed since the last tick call
	 */
	CORE_API void Tick(float DeltaTime);


protected:
	/**
	 * Element of the priority queue
	 */
	struct FElement
	{
		/** Time that this delegate must not fire before **/
		double FireTime;
		/** Delay that this delegate was scheduled with. Kept here so that if the delegate returns true, we will reschedule it. **/
		float DelayTime;
		/** Delegate to fire **/
		FTickerDelegate Delegate;

		/** Default ctor is only required to implement CurrentElement handling without making it a pointer. */
		CORE_API FElement();
		/** This is the ctor that the code will generally use. */
		CORE_API FElement(double InFireTime, float InDelayTime, const FTickerDelegate& InDelegate);
		
		/** Fire the delegate if it is fireable **/
		CORE_API bool Fire(float DeltaTime);
	};

	/** Current time of the ticker **/
	double CurrentTime;
	/** Future delegates to fire **/
	TArray<FElement, TInlineAllocator<1>> Elements;
	/** List of delegates that have been considered during Tick. Either they been fired or are not yet ready.*/
	TArray<FElement, TInlineAllocator<1>> TickedElements;
	/** Current element being ticked (only valid during tick). */
	FElement CurrentElement;
	/** State to track whether CurrentElement is valid. */
	bool bInTick;
	/** State to track whether the CurrentElement removed itself during its own delegate execution. */
	bool bCurrentElementRemoved;
};

/**
 * Base class for ticker objects
 */
class UE_DEPRECATED(5.0, "Please use thread-safe FTSTickerObjectBase, see the migration guide in the code comment above FTSTicker") FTickerObjectBase
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS // FTicker

public:

	/**
	 * Constructor
	 *
	 * @param InDelay Delay until next fire; 0 means "next frame"
	 * @param Ticker the ticker to register with. Defaults to FTicker::GetCoreTicker().
	*/
#if ( PLATFORM_WINDOWS && defined(__clang__) )
	CORE_API FTickerObjectBase()							// @todo clang: non-default argument constructors needed to prevent an ICE in clang
		: FTickerObjectBase(0.0f, FTicker::GetCoreTicker())	// https://llvm.org/bugs/show_bug.cgi?id=28137
	{
	}

	CORE_API FTickerObjectBase(float InDelay)
		: FTickerObjectBase(InDelay, FTicker::GetCoreTicker())
	{
	}

	CORE_API FTickerObjectBase(float InDelay, FTicker& Ticker);
#else
	CORE_API FTickerObjectBase(float InDelay = 0.0f, FTicker& Ticker = FTicker::GetCoreTicker());
#endif

	/** Virtual destructor. */
	CORE_API virtual ~FTickerObjectBase();

	/**
	 * Pure virtual that must be overloaded by the inheriting class.
	 *
	 * @param DeltaTime	time passed since the last call.
	 * @return true if should continue ticking
	 */
	virtual bool Tick(float DeltaTime) = 0;

private:
	// noncopyable idiom
	FTickerObjectBase(const FTickerObjectBase&);
	FTickerObjectBase& operator=(const FTickerObjectBase&);

	/** Ticker to register with */
	FTicker& Ticker;
	/** Delegate for callbacks to Tick */
	FDelegateHandle TickHandle;

	PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
