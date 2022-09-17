// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AsyncWork.h: Definition of queued work classes
=============================================================================*/

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Compression.h"
#include "HAL/ThreadSafeCounter.h"
#include "Stats/Stats.h"
#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "HAL/LowLevelMemTracker.h"
#include "Misc/IQueuedWork.h"
#include "Misc/QueuedThreadPool.h"
#include "ProfilingDebugging/TagTrace.h"
#include "Async/Fundamental/Scheduler.h"

/**
	FAutoDeleteAsyncTask - template task for jobs that delete themselves when complete

	Sample code:

	class ExampleAutoDeleteAsyncTask : public FNonAbandonableTask
	{
		friend class FAutoDeleteAsyncTask<ExampleAutoDeleteAsyncTask>;

		int32 ExampleData;

		ExampleAutoDeleteAsyncTask(int32 InExampleData)
		 : ExampleData(InExampleData)
		{
		}

		void DoWork()
		{
			... do the work here
		}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(ExampleAutoDeleteAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
		}
	};

	
	void Example()
	{
		// start an example job
		(new FAutoDeleteAsyncTask<ExampleAutoDeleteAsyncTask>(5)->StartBackgroundTask();

		// do an example job now, on this thread
		(new FAutoDeleteAsyncTask<ExampleAutoDeleteAsyncTask>(5)->StartSynchronousTask();
	}

**/
template<typename TTask>
class FAutoDeleteAsyncTask
	: private IQueuedWork
{
	/** User job embedded in this task */
	TTask Task;
	/** optional LLM tag */
	LLM(const UE::LLMPrivate::FTagData* InheritedLLMTag);
#if UE_MEMORY_TAGS_TRACE_ENABLED
	/** optional Trace tag */
	int32 InheritedTraceTag;
#endif

	/* Generic start function, not called directly
	 * @param bForceSynchronous if true, this job will be started synchronously, now, on this thread
	 **/
	void Start(bool bForceSynchronous, FQueuedThreadPool* InQueuedPool, EQueuedWorkPriority InPriority = EQueuedWorkPriority::Normal)
	{
		LLM(InheritedLLMTag = FLowLevelMemTracker::bIsDisabled ? nullptr : FLowLevelMemTracker::Get().GetActiveTagData(ELLMTracker::Default));
#if UE_MEMORY_TAGS_TRACE_ENABLED
		InheritedTraceTag = MemoryTrace_GetActiveTag();
#endif

		FPlatformMisc::MemoryBarrier();
		FQueuedThreadPool* QueuedPool = InQueuedPool;
		if (bForceSynchronous)
		{
			QueuedPool = 0;
		}
		if (QueuedPool)
		{
			QueuedPool->AddQueuedWork(this, InPriority);
		}
		else
		{
			// we aren't doing async stuff
			DoWork();
		}
	}

	/**
	 * Tells the user job to do the work, sometimes called synchronously, sometimes from the thread pool. Calls the event tracker.
	 **/
	void DoWork()
	{
		LLM_SCOPE(InheritedLLMTag);
		UE_MEMSCOPE(InheritedTraceTag);
		FScopeCycleCounter Scope(Task.GetStatId(), true);

		Task.DoWork();
		delete this;
	}

	/**
	* Always called from the thread pool. Just passes off to DoWork
	**/
	virtual void DoThreadedWork()
	{
		DoWork();
	}

	/**
	 * Always called from the thread pool. Called if the task is removed from queue before it has started which might happen at exit.
	 * If the user job can abandon, we do that, otherwise we force the work to be done now (doing nothing would not be safe).
	 */
	virtual void Abandon(void)
	{
		if (Task.CanAbandon())
		{
			Task.Abandon();
			delete this;
		}
		else
		{
			DoWork();
		}
	}

public:
	/** Forwarding constructor. */
	template<typename...T>
	explicit FAutoDeleteAsyncTask(T&&... Args) : Task(Forward<T>(Args)...)
	{
	}

	/** 
	* Run this task on this thread, now. Will end up destroying myself, so it is not safe to use this object after this call.
	**/
	void StartSynchronousTask()
	{
		Start(true, nullptr);
	}

	/** 
	* Run this task on the lo priority thread pool. It is not safe to use this object after this call.
	**/
	void StartBackgroundTask(FQueuedThreadPool* InQueuedPool = GThreadPool, EQueuedWorkPriority InPriority = EQueuedWorkPriority::Normal)
	{
		Start(false, InQueuedPool, InPriority);
	}
};


/**
	FAsyncTask - template task for jobs queued to thread pools

	Sample code:

	class ExampleAsyncTask : public FNonAbandonableTask
	{
		friend class FAsyncTask<ExampleAsyncTask>;

		int32 ExampleData;

		ExampleAsyncTask(int32 InExampleData)
		 : ExampleData(InExampleData)
		{
		}

		void DoWork()
		{
			... do the work here
		}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(ExampleAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
		}
	};

	void Example()
	{

		//start an example job

		FAsyncTask<ExampleAsyncTask>* MyTask = new FAsyncTask<ExampleAsyncTask>( 5 );
		MyTask->StartBackgroundTask();

		//--or --

		MyTask->StartSynchronousTask();

		//to just do it now on this thread
		//Check if the task is done :

		if (MyTask->IsDone())
		{
		}

		//Spinning on IsDone is not acceptable( see EnsureCompletion ), but it is ok to check once a frame.
		//Ensure the task is done, doing the task on the current thread if it has not been started, waiting until completion in all cases.

		MyTask->EnsureCompletion();
		delete Task;
	}
**/
template<typename TTask>
class FAsyncTask
	: private IQueuedWork
{
	/** User job embedded in this task */ 
	TTask Task;
	/** Thread safe counter that indicates WORK completion, no necessarily finalization of the job */
	FThreadSafeCounter	WorkNotFinishedCounter;
	/** If we aren't doing the work synchronously, this will hold the completion event */
	FEvent*				DoneEvent;
	/** Pool we are queued into, maintained by the calling thread */
	FQueuedThreadPool*	QueuedPool;
	/** Current priority */
	EQueuedWorkPriority Priority;
	/** Current flags */
	EQueuedWorkFlags Flags;
	/** Approximation of the peak memory (in bytes) this task could require during it's execution. */
	int64 RequiredMemory = -1;
	/** optional LLM tag */
	LLM(const UE::LLMPrivate::FTagData* InheritedLLMTag);
	/** Memory trace tag */
#if UE_MEMORY_TAGS_TRACE_ENABLED
	int32 InheritedTraceTag;
#endif

	/* Internal function to destroy the completion event
	**/
	void DestroyEvent()
	{
		FPlatformProcess::ReturnSynchEventToPool(DoneEvent);
		DoneEvent = nullptr;
	}

	EQueuedWorkFlags GetQueuedWorkFlags() const override
	{
		return Flags;
	}

	/* Generic start function, not called directly
		* @param bForceSynchronous if true, this job will be started synchronously, now, on this thread
	**/
	void Start(bool bForceSynchronous, FQueuedThreadPool* InQueuedPool, EQueuedWorkPriority InQueuedWorkPriority, EQueuedWorkFlags InQueuedWorkFlags, int64 InRequiredMemory)
	{
		FScopeCycleCounter Scope( Task.GetStatId(), true );
		DECLARE_SCOPE_CYCLE_COUNTER( TEXT( "FAsyncTask::Start" ), STAT_FAsyncTask_Start, STATGROUP_ThreadPoolAsyncTasks );
		LLM(InheritedLLMTag = FLowLevelMemTracker::bIsDisabled ? nullptr : FLowLevelMemTracker::Get().GetActiveTagData(ELLMTracker::Default));
#if UE_MEMORY_TAGS_TRACE_ENABLED
		InheritedTraceTag = MemoryTrace_GetActiveTag();
#endif
		RequiredMemory = InRequiredMemory;

		FPlatformMisc::MemoryBarrier();
		CheckIdle();  // can't start a job twice without it being completed first
		WorkNotFinishedCounter.Increment();
		QueuedPool = InQueuedPool;
		Priority = InQueuedWorkPriority;
		Flags = InQueuedWorkFlags;
		if (bForceSynchronous)
		{
			QueuedPool = 0;
		}
		if (QueuedPool)
		{
			if (!DoneEvent)
			{
				DoneEvent = FPlatformProcess::GetSynchEventFromPool(true);
			}
			DoneEvent->Reset();
			QueuedPool->AddQueuedWork(this, InQueuedWorkPriority);
		}
		else 
		{
			// we aren't doing async stuff
			DestroyEvent();
			DoWork();
		}
	}

	/** 
	* Tells the user job to do the work, sometimes called synchronously, sometimes from the thread pool. Calls the event tracker.
	**/
	void DoWork()
	{	
		LLM_SCOPE(InheritedLLMTag);
		UE_MEMSCOPE(InheritedTraceTag);
		FScopeCycleCounter Scope(Task.GetStatId(), true); 

		Task.DoWork();		
		check(WorkNotFinishedCounter.GetValue() == 1);
		WorkNotFinishedCounter.Decrement();
	}

	/** 
	* Triggers the work completion event, only called from a pool thread
	**/
	void FinishThreadedWork()
	{
		check(QueuedPool);
		if (DoneEvent)
		{
			FScopeCycleCounter Scope( Task.GetStatId(), true );
			DECLARE_SCOPE_CYCLE_COUNTER( TEXT( "FAsyncTask::FinishThreadedWork" ), STAT_FAsyncTask_FinishThreadedWork, STATGROUP_ThreadPoolAsyncTasks );		
			DoneEvent->Trigger();
		}
	}

	/** 
	* Performs the work, this is only called from a pool thread.
	**/
	virtual void DoThreadedWork() override
	{
		DoWork();
		FinishThreadedWork();
	}

	/**
	 * Always called from the thread pool. Called if the task is removed from queue before it has started which might happen at exit.
	 * If the user job can abandon, we do that, otherwise we force the work to be done now (doing nothing would not be safe).
	 */
	virtual void Abandon(void) override
	{
		if (Task.CanAbandon())
		{
			Task.Abandon();
			check(WorkNotFinishedCounter.GetValue() == 1);
			WorkNotFinishedCounter.Decrement();
		}
		else
		{
			DoWork();
		}
		FinishThreadedWork();
	}

	/** 
	* Internal call to assert that we are idle
	**/
	void CheckIdle() const
	{
		check(WorkNotFinishedCounter.GetValue() == 0);
		check(!QueuedPool);
	}

	/** 
	* Internal call to synchronize completion between threads, never called from a pool thread
	* @param bIsLatencySensitive specifies if waiting for the task should return as soon as possible even if this delays other tasks
	**/
	void SyncCompletion(bool bIsLatencySensitive)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FAsyncTask::SyncCompletion);

		FPlatformMisc::MemoryBarrier();
		if (QueuedPool)
		{
			FScopeCycleCounter Scope( Task.GetStatId() );
			DECLARE_SCOPE_CYCLE_COUNTER( TEXT( "FAsyncTask::SyncCompletion" ), STAT_FAsyncTask_SyncCompletion, STATGROUP_ThreadPoolAsyncTasks );

			if (LowLevelTasks::FScheduler::Get().IsWorkerThread() && !bIsLatencySensitive)
			{
				LowLevelTasks::BusyWaitUntil([this]() { return IsWorkDone(); });
			}

			check(DoneEvent); // if it is not done yet, we must have an event
			DoneEvent->Wait();
			QueuedPool = 0;
		}
		CheckIdle();
	}

	/** 
	* Internal call to initialize internal variables
	**/
	void Init()
	{
		DoneEvent = 0;
		QueuedPool = 0;
	}

public:
	FAsyncTask()
		: Task()
	{
		// This constructor shouldn't be necessary as the forwarding constructor should handle it, but
		// we are getting VC internal compiler errors on CIS when creating arrays of FAsyncTask.

		Init();
	}

	/** Forwarding constructor. */
	template <typename Arg0Type, typename... ArgTypes>
	FAsyncTask(Arg0Type&& Arg0, ArgTypes&&... Args)
		: Task(Forward<Arg0Type>(Arg0), Forward<ArgTypes>(Args)...)
	{
		Init();
	}

	/** Destructor, not legal when a task is in process */
	~FAsyncTask()
	{
		// destroying an unfinished task is a bug
		CheckIdle();
		DestroyEvent();
	}

	/* Retrieve embedded user job, not legal to call while a job is in process
	* @return reference to embedded user job 
	**/
	TTask &GetTask()
	{
		CheckIdle();  // can't modify a job without it being completed first
		return Task;
	}

	/* Retrieve embedded user job, not legal to call while a job is in process
	* @return reference to embedded user job 
	**/
	const TTask &GetTask() const
	{
		CheckIdle();  // could be safe, but I won't allow it anyway because the data could be changed while it is being read
		return Task;
	}

	/**
	 * Returns an approximation of the peak memory (in bytes) this task could require during it's execution.
	 **/
	int64 GetRequiredMemory() const override
	{
		return RequiredMemory;
	}

	/** 
	* Run this task on this thread
	* @param bDoNow if true then do the job now instead of at EnsureCompletion
	**/
	void StartSynchronousTask(EQueuedWorkPriority InQueuedWorkPriority = EQueuedWorkPriority::Normal, EQueuedWorkFlags InQueuedWorkFlags = EQueuedWorkFlags::None, int64 InRequiredMemory = -1)
	{
		Start(true, GThreadPool, InQueuedWorkPriority, InQueuedWorkFlags, InRequiredMemory);
	}

	/** 
	* Queue this task for processing by the background thread pool
	**/
	void StartBackgroundTask(FQueuedThreadPool* InQueuedPool = GThreadPool, EQueuedWorkPriority InQueuedWorkPriority = EQueuedWorkPriority::Normal, EQueuedWorkFlags InQueuedWorkFlags = EQueuedWorkFlags::None, int64 InRequiredMemory = -1)
	{
		Start(false, InQueuedPool, InQueuedWorkPriority, InQueuedWorkFlags, InRequiredMemory);
	}

	/** 
	* Wait until the job is complete
	* @param bDoWorkOnThisThreadIfNotStarted if true and the work has not been started, retract the async task and do it now on this thread
	* @param specifies if waiting for the task should return as soon as possible even if this delays other tasks
	**/
	void EnsureCompletion(bool bDoWorkOnThisThreadIfNotStarted = true, bool bIsLatencySensitive = false)
	{
		bool DoSyncCompletion = true;
		if (bDoWorkOnThisThreadIfNotStarted)
		{
			if (QueuedPool)
			{
				if (QueuedPool->RetractQueuedWork(this))
				{
					// we got the job back, so do the work now and no need to synchronize
					DoSyncCompletion = false;
					DoWork(); 
					FinishThreadedWork();
					QueuedPool = 0;
				}
			}
			else if (WorkNotFinishedCounter.GetValue())  // in the synchronous case, if we haven't done it yet, do it now
			{
				DoWork(); 
			}
		}
		if (DoSyncCompletion)
		{
			SyncCompletion(bIsLatencySensitive);
		}
		CheckIdle(); // Must have had bDoWorkOnThisThreadIfNotStarted == false and needed it to be true for a synchronous job
	}
	
	/**
	* If not already being processed, will be rescheduled on given thread pool and priority.
	* @return true if the reschedule was successful, false if was already being processed.
	**/
	bool Reschedule(FQueuedThreadPool* InQueuedPool = GThreadPool, EQueuedWorkPriority InQueuedWorkPriority = EQueuedWorkPriority::Normal)
	{
		if (QueuedPool)
		{
			if (QueuedPool->RetractQueuedWork(this))
			{
				QueuedPool = InQueuedPool;
				Priority = InQueuedWorkPriority;
				QueuedPool->AddQueuedWork(this, InQueuedWorkPriority);
				
				return true;
			}
		}
		return false;
	}

	/**
	* Cancel the task, if possible.
	* Note that this is different than abandoning (which is called by the thread pool at shutdown). 
	* @return true if the task was canceled and is safe to delete. If it wasn't canceled, it may be done, but that isn't checked here.
	**/
	bool Cancel()
	{
		if (QueuedPool)
		{
			if (QueuedPool->RetractQueuedWork(this))
			{
				check(WorkNotFinishedCounter.GetValue() == 1);
				WorkNotFinishedCounter.Decrement();
				FinishThreadedWork();
				QueuedPool = 0;
				return true;
			}
		}
		return false;
	}

	/**
	* Wait until the job is complete, up to a time limit
	* @param TimeLimitSeconds Must be positive, if you want to wait forever or poll, use a different call.
	* @return true if the task is completed
	**/
	bool WaitCompletionWithTimeout(float TimeLimitSeconds)
	{
		check(TimeLimitSeconds > 0.0f)
		FPlatformMisc::MemoryBarrier();
		if (QueuedPool)
		{
			FScopeCycleCounter Scope(Task.GetStatId());
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FAsyncTask::SyncCompletion"), STAT_FAsyncTask_SyncCompletion, STATGROUP_ThreadPoolAsyncTasks);

			uint32 Ms = uint32(TimeLimitSeconds * 1000.0f) + 1;
			check(Ms);

			check(DoneEvent); // if it is not done yet, we must have an event
			if (DoneEvent->Wait(Ms))
			{
				QueuedPool = 0;
				CheckIdle();
				return true;
			}
			return false;
		}
		CheckIdle();
		return true;
	}

	/** Returns true if the work and TASK has completed, false while it's still in progress. 
	 * prior to returning true, it synchronizes so the task can be destroyed or reused
	 */
	bool IsDone()
	{
		if (!IsWorkDone())
		{
			return false;
		}
		SyncCompletion(/*bIsLatencySensitive = */false);
		return true;
	}

	/** Returns true if the work has completed, false while it's still in progress. 
	 * This does not block and if true, you can use the results.
	 * But you can't destroy or reuse the task without IsDone() being true or EnsureCompletion()
	*/
	bool IsWorkDone() const
	{
		if (WorkNotFinishedCounter.GetValue())
		{
			return false;
		}
		return true;
	}

	/** Returns true if the work has not been started or has been completed. 
	 * NOT to be used for synchronization, but great for check()'s 
	 */
	bool IsIdle() const
	{
		return WorkNotFinishedCounter.GetValue() == 0 && QueuedPool == 0;
	}

	EQueuedWorkPriority GetPriority() const
	{
		return Priority;
	}
};

/**
 * Stub class to use a base class for tasks that cannot be abandoned
 */
class FNonAbandonableTask
{
public:
	bool CanAbandon()
	{
		return false;
	}
	void Abandon()
	{
	}
};
