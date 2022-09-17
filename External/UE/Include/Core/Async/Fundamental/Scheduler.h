// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 
#include "Async/Fundamental/Task.h"
#include "HAL/Thread.h"
#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "LocalQueue.h"
#include "Containers/Array.h"
#include "Templates/UniquePtr.h"
#include "Templates/Function.h"

namespace LowLevelTasks
{
	enum class EQueuePreference
	{
		GlobalQueuePreference,
		LocalQueuePreference,
		DefaultPreference = LocalQueuePreference,
	};


	//implementation of a treiber stack
	//(https://en.wikipedia.org/wiki/Treiber_stack)
	template<typename NodeType>
	class TEventStack
	{
	private:
		struct FTopNode
		{
			uintptr_t Address  : 45; //all CPUs we care about use less than 48 bits for their addressing, and the lower 3 bits are unused due to alignment
			uintptr_t Revision : 19; //Tagging is used to avoid ABA (the wrap around is several minutes for this use-case (https://en.wikipedia.org/wiki/ABA_problem#Tagged_state_reference))
		};
		std::atomic<FTopNode> Top { FTopNode{0, 0} };

	public:
		NodeType* Pop()
		{
			FTopNode LocalTop = Top.load(std::memory_order_acquire);
			while (true) 
			{			
				if (LocalTop.Address == 0)
				{
					return nullptr;
				}
#if DO_CHECK
				int64 LastRevision = int64(LocalTop.Revision); 
#endif
				NodeType* Item = reinterpret_cast<NodeType*>(LocalTop.Address << 3);																 //acquire on failure because we read Item->Next next itteration
				if (Top.compare_exchange_weak(LocalTop, FTopNode { reinterpret_cast<uintptr_t>(Item->Next) >> 3, uintptr_t(LocalTop.Revision + 1) }, std::memory_order_relaxed, std::memory_order_acquire)) 
				{
					Item->Next = nullptr;
					return Item;
				}
#if DO_CHECK
				int64 NewRevision = int64(LocalTop.Revision) < LastRevision ? ((1ll << 19) + int64(LocalTop.Revision)) : int64(LocalTop.Revision);
				ensureMsgf((NewRevision - LastRevision) < (1ll << 18), TEXT("Dangerously close to the wraparound: %d, %d"), LastRevision, NewRevision);
#endif
			}
		}

		void Push(NodeType* Item)
		{
			checkSlow(Item != nullptr);
#if !USING_CODE_ANALYSIS //MS SA thowing warning C6011 on Item->Next access, even when it is validated or branched over
			checkSlow(reinterpret_cast<uintptr_t>(Item) < (1ull << 48));
			checkSlow((reinterpret_cast<uintptr_t>(Item) & 0x7) == 0);
#endif
			checkSlow(Item->Next == nullptr);

			FTopNode LocalTop = Top.load(std::memory_order_relaxed);
			while (true) 
			{
#if DO_CHECK
				int64 LastRevision = int64(LocalTop.Revision); 
#endif
				Item->Next = reinterpret_cast<NodeType*>(LocalTop.Address << 3);
				if (Top.compare_exchange_weak(LocalTop, FTopNode { reinterpret_cast<uintptr_t>(Item) >> 3, uintptr_t(LocalTop.Revision + 1) }, std::memory_order_release, std::memory_order_acquire))  
				{
					return;
				}
#if DO_CHECK
				int64 NewRevision = int64(LocalTop.Revision) < LastRevision ? ((1ll << 19) + int64(LocalTop.Revision)) : int64(LocalTop.Revision);
				ensureMsgf((NewRevision - LastRevision) < (1ll << 18), TEXT("Dangerously close to the wraparound: %d, %d"), LastRevision, NewRevision);
#endif
			}
		}
	};

	class FSchedulerTls
	{
	protected:
		using FQueueRegistry	= TLocalQueueRegistry<>;
		using FLocalQueueType	= FQueueRegistry::TLocalQueue;

		enum class EWorkerType
		{
			None,
			Background,
			Foreground,
		};

		static thread_local FSchedulerTls* ActiveScheduler;
		static thread_local FLocalQueueType* LocalQueue;
		static thread_local EWorkerType WorkerType;
		// number of busy-waiting calls in the call-stack
		static thread_local uint32 BusyWaitingDepth;

	public:
		CORE_API bool IsWorkerThread() const;

		// returns true if the current thread execution is in the context of busy-waiting
		CORE_API static bool IsBusyWaiting();

		// returns the AffinityIndex of the thread LocalQueue
		CORE_API static uint32 GetAffinityIndex();

	protected:
		inline static bool IsBackgroundWorker()
		{
			return WorkerType == EWorkerType::Background;
		}
	};

	class FScheduler final : public FSchedulerTls
	{
		UE_NONCOPYABLE(FScheduler);
		static constexpr uint32 WorkerSpinCycles = 53;

		static CORE_API FScheduler Singleton;

		// using 16 bytes here because it fits the vtable and one additional pointer
		using FConditional = TTaskDelegate<bool(), 16>;

	private: 
		//the FLocalQueueInstaller installs a LocalQueue into the current thread
		struct FLocalQueueInstaller
		{
			bool RegisteredLocalQueue = false;
			CORE_API FLocalQueueInstaller(FScheduler& Scheduler);
			CORE_API ~FLocalQueueInstaller();
		};

	public: // Public Interface of the Scheduler
		FORCEINLINE_DEBUGGABLE static FScheduler& Get();

		//start number of workers where 0 is the system default
		CORE_API void StartWorkers(uint32 NumForegroundWorkers = 0, uint32 NumBackgroundWorkers = 0, FThread::EForkable IsForkable = FThread::NonForkable, EThreadPriority InWorkerPriority = EThreadPriority::TPri_Normal, EThreadPriority InBackgroundPriority = EThreadPriority::TPri_BelowNormal, uint64 InWorkerAffinity = 0, uint64 InBackgroundAffinity = 0);
		CORE_API void StopWorkers(bool DrainGlobalQueue = true);
		CORE_API void RestartWorkers(uint32 NumForegroundWorkers = 0, uint32 NumBackgroundWorkers = 0, FThread::EForkable IsForkable = FThread::NonForkable, EThreadPriority WorkerPriority = EThreadPriority::TPri_Normal, EThreadPriority BackgroundPriority = EThreadPriority::TPri_BelowNormal, uint64 InWorkerAffinity = 0, uint64 InBackgroundAffinity = 0);

		//try to launch the task, the return value will specify if the task was in the ready state and has been launhced
		inline bool TryLaunch(FTask& Task, EQueuePreference QueuePreference = EQueuePreference::DefaultPreference, bool bWakeUpWorker = true);	

		//try to launch the task on a specific worker ID, the return value will specify if the task was in the ready state and has been launched
		inline bool TryLaunchAffinity(FTask& Task, uint32 AffinityIndex);	

		//tries to do some work until the Task is completed
		template<typename TaskType>
		inline void BusyWait(const TaskType& Task, bool ForceAllowBackgroundWork = false);

		//tries to do some work until the Conditional return true
		template<typename Conditional>
		inline void BusyWaitUntil(Conditional&& Cond, bool ForceAllowBackgroundWork = false);

		//tries to do some work until all the Tasks are completed
		//the template parameter can be any Type that has a const conversion operator to FTask
		template<typename TaskType>
		inline void BusyWait(const TArrayView<const TaskType>& Tasks, bool ForceAllowBackgroundWork = false);

		//number of instantiated workers
		inline uint32 GetNumWorkers() const;

		//get the Queue registry to register additonal WorkerQueues for this Scheduler
		inline FSchedulerTls::FQueueRegistry& GetQueueRegistry();

	public:
		FScheduler() = default;
		~FScheduler();

	private: 
		TUniquePtr<FThread> CreateWorker(bool bPermitBackgroundWork = false, FThread::EForkable IsForkable = FThread::NonForkable, FSleepEvent* ExternalWorkerEvent = nullptr, FSchedulerTls::FLocalQueueType* ExternalWorkerLocalQueue = nullptr, EThreadPriority Priority = EThreadPriority::TPri_Normal, uint64 InAffinity = 0);
		void WorkerMain(struct FSleepEvent* WorkerEvent, FSchedulerTls::FLocalQueueType* ExternalWorkerLocalQueue, uint32 WaitCycles, bool bPermitBackgroundWork);
		CORE_API void LaunchInternal(FTask& Task, EQueuePreference QueuePreference, bool bWakeUpWorker);
		CORE_API void BusyWaitInternal(const FConditional& Conditional, bool ForceAllowBackgroundWork);
		FORCENOINLINE bool TrySleeping(FSleepEvent* WorkerEvent, bool bStopOutOfWorkScope, bool Drowsing, bool bBackgroundWorker);
		inline bool WakeUpWorker(bool bBackgroundWorker);

		template<FTask* (FSchedulerTls::FLocalQueueType::*DequeueFunction)(bool, bool), bool bIsBusyWaiting>
		bool TryExecuteTaskFrom(FSchedulerTls::FLocalQueueType* Queue, FSchedulerTls::FQueueRegistry::FOutOfWork& OutOfWork, bool bPermitBackgroundWork, bool bDisableThrottleStealing);

	private:
		template<typename ElementType>
		using TAlignedArray = TArray<ElementType, TAlignedHeapAllocator<alignof(ElementType)>>;
		TEventStack<FSleepEvent> 						SleepEventStack[2];
		FSchedulerTls::FQueueRegistry 					QueueRegistry;
		FCriticalSection 								WorkerThreadsCS;
		TArray<TUniquePtr<FThread>>						WorkerThreads;
		TAlignedArray<FSchedulerTls::FLocalQueueType>	WorkerLocalQueues;
		TAlignedArray<FSleepEvent>						WorkerEvents;
		std::atomic_uint								ActiveWorkers { 0 };
		std::atomic_uint								NextWorkerId { 0 };
		uint64											WorkerAffinity = 0;
		uint64											BackgroundAffinity = 0;
		EThreadPriority									WorkerPriority = EThreadPriority::TPri_Normal;
		EThreadPriority									BackgroundPriority = EThreadPriority::TPri_BelowNormal;
		std::atomic_bool								TemporaryShutdown{ false };
	};

	FORCEINLINE_DEBUGGABLE bool TryLaunch(FTask& Task, EQueuePreference QueuePreference = EQueuePreference::DefaultPreference, bool bWakeUpWorker = true)
	{
		return FScheduler::Get().TryLaunch(Task, QueuePreference, bWakeUpWorker);
	}

	FORCEINLINE_DEBUGGABLE bool TryLaunchAffinity(FTask& Task, uint32 AffinityIndex)
	{
		return FScheduler::Get().TryLaunchAffinity(Task, AffinityIndex);
	}

	FORCEINLINE_DEBUGGABLE void BusyWaitForTask(const FTask& Task, bool ForceAllowBackgroundWork = false)
	{
		FScheduler::Get().BusyWait(Task, ForceAllowBackgroundWork);
	}

	template<typename Conditional>
	FORCEINLINE_DEBUGGABLE void BusyWaitUntil(Conditional&& Cond, bool ForceAllowBackgroundWork = false)
	{
		FScheduler::Get().BusyWaitUntil<Conditional>(Forward<Conditional>(Cond), ForceAllowBackgroundWork);
	}

	template<typename TaskType>
	FORCEINLINE_DEBUGGABLE void BusyWaitForTasks(const TArrayView<const TaskType>& Tasks, bool ForceAllowBackgroundWork = false)
	{
		FScheduler::Get().BusyWait<TaskType>(Tasks, ForceAllowBackgroundWork);
	}

   /******************
	* IMPLEMENTATION *
	******************/
	inline bool FScheduler::TryLaunch(FTask& Task, EQueuePreference QueuePreference, bool bWakeUpWorker)
	{
		if(Task.TryPrepareLaunch())
		{
			LaunchInternal(Task, QueuePreference, bWakeUpWorker);
			return true;
		}
		return false;
	}

	inline bool FScheduler::TryLaunchAffinity(FTask& Task, uint32 AffinityIndex)
	{
		if(Task.TryPrepareLaunch())
		{
			return QueueRegistry.EnqueueAffinity(&Task, AffinityIndex);
		}
		return false;
	}

	inline uint32 FScheduler::GetNumWorkers() const
	{
		return ActiveWorkers.load(std::memory_order_relaxed);
	}

	template<typename TaskType>
	inline void FScheduler::BusyWait(const TaskType& Task, bool ForceAllowBackgroundWork)
	{
		if(!Task.IsCompleted())
		{
			FLocalQueueInstaller Installer(*this);
			FScheduler::BusyWaitInternal([&Task](){ return Task.IsCompleted(); }, ForceAllowBackgroundWork);
		}
	}

	template<typename Conditional>
	inline void FScheduler::BusyWaitUntil(Conditional&& Cond, bool ForceAllowBackgroundWork)
	{
		static_assert(TIsInvocable<Conditional>::Value, "Conditional is not invocable");
		static_assert(TIsSame<decltype(Cond()), bool>::Value, "Conditional must return a boolean");
		
		if(!Cond())
		{
			FLocalQueueInstaller Installer(*this);
			FScheduler::BusyWaitInternal(Forward<Conditional>(Cond), ForceAllowBackgroundWork);
		}
	}

	template<typename TaskType>
	inline void FScheduler::BusyWait(const TArrayView<const TaskType>& Tasks, bool ForceAllowBackgroundWork)
	{
		auto AllTasksCompleted = [Index(0), &Tasks]() mutable
		{
			while (Index < Tasks.Num())
			{
				if (!Tasks[Index].IsCompleted())
				{
					return false;
				}
				Index++;
			}
			return true;
		};

		if (!AllTasksCompleted())
		{
			FLocalQueueInstaller Installer(*this);
			FScheduler::BusyWaitInternal([&AllTasksCompleted](){ return AllTasksCompleted(); }, ForceAllowBackgroundWork);
		}
	}

	inline bool FScheduler::TrySleeping(FSleepEvent* WorkerEvent, bool bStopOutOfWorkScope, bool bDrowsing, bool bBackgroundWorker)
	{
		ESleepState DrowsingState1 = ESleepState::Drowsing;
		ESleepState DrowsingState2 = ESleepState::Drowsing;
		ESleepState RunningState  = ESleepState::Running;
		ESleepState AffinityState  = ESleepState::Affinity;

		if(!bDrowsing && WorkerEvent->SleepState.compare_exchange_strong(DrowsingState1, ESleepState::Drowsing, std::memory_order_release)) //continue drowsing
		{
			verifySlow(bStopOutOfWorkScope);
			bDrowsing = true; // Alternative State one: ((Running -> Drowsing) -> Drowsing)
		}
		else if(WorkerEvent->SleepState.compare_exchange_strong(DrowsingState2, ESleepState::Sleeping, std::memory_order_release))
		{
			verifySlow(!bStopOutOfWorkScope);
			bDrowsing = false;
			WorkerEvent->SleepEvent->Wait(); // State two: ((Running -> Drowsing) -> Sleeping)
		}
		else if(WorkerEvent->SleepState.compare_exchange_strong(AffinityState, ESleepState::Drowsing, std::memory_order_release)) //continue drowsing
		{
			bDrowsing = true; // Alternative State one: ((Running -> Drowsing) -> Drowsing)
		}
		else if(WorkerEvent->SleepState.compare_exchange_strong(RunningState, ESleepState::Drowsing, std::memory_order_release))
		{
			bDrowsing = true;
			SleepEventStack[bBackgroundWorker].Push(WorkerEvent); // State one: (Running -> Drowsing)
		}
		else
		{
			checkf(false, TEXT("Worker was supposed to be running or drowsing: %d"), WorkerEvent->SleepState.load(std::memory_order_relaxed));
		}

		return bDrowsing;
	}

	inline bool FScheduler::WakeUpWorker(bool bBackgroundWorker)
	{
		while (FSleepEvent* WorkerEvent = SleepEventStack[bBackgroundWorker].Pop())
		{
			ESleepState SleepState = WorkerEvent->SleepState.exchange(ESleepState::Running, std::memory_order_acquire);
			if (SleepState == ESleepState::Sleeping)
			{
				WorkerEvent->SleepEvent->Trigger(); 
				return true; // Solving State two: (((Running -> Drowsing) -> Sleeping) -> Running)
			}
			else if (SleepState == ESleepState::Drowsing)
			{
				return true; // Solving State one: (Running -> Drowsing) -> Running  OR ((Running -> Drowsing) -> Drowsing) -> Running
			} 
		}
		return false;
	}

	inline FSchedulerTls::FQueueRegistry& FScheduler::GetQueueRegistry()
	{
		return QueueRegistry;
	}

	inline FScheduler& FScheduler::Get()
	{
		return Singleton;
	}

	inline FScheduler::~FScheduler()
	{
		StopWorkers();
	}
}

