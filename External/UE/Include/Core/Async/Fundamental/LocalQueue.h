// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreTypes.h"
#include "Math/RandomStream.h"
#include "Experimental/Containers/FAAArrayQueue.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Async/Fundamental/Task.h"

#include <atomic>

namespace LowLevelTasks
{
namespace LocalQueue_Impl
{
template<uint32 NumItems>
class TWorkStealingQueueBase2
{
	enum class ESlotState : uintptr_t
	{
		Free  = 0, //The slot is free and items can be put there
		Taken = 1, //The slot is in the proccess of beeing stolen 
	};

protected:
	//insert an item at the head position (this can only safe on a single thread, shared with Get) 
	inline bool Put(uintptr_t Item)
	{
		checkSlow(Item != uintptr_t(ESlotState::Free));
		checkSlow(Item != uintptr_t(ESlotState::Taken));

		uint32 Idx = (Head + 1) % NumItems;
		uintptr_t Slot = ItemSlots[Idx].Value.load(std::memory_order_acquire);

		if (Slot == uintptr_t(ESlotState::Free))
		{
			ItemSlots[Idx].Value.store(Item, std::memory_order_release);
			Head++;
			checkSlow(Head % NumItems == Idx);
			return true;		
		}
		return false;
	}

	//remove an item at the head position in FIFO order (this can only safe on a single thread, shared with Put) 
	inline bool Get(uintptr_t& Item)
	{
		uint32 Idx = Head % NumItems;
		uintptr_t Slot = ItemSlots[Idx].Value.load(std::memory_order_acquire);

		if (Slot > uintptr_t(ESlotState::Taken) && ItemSlots[Idx].Value.compare_exchange_strong(Slot, uintptr_t(ESlotState::Free), std::memory_order_acq_rel))
		{
			Head--;
			checkSlow((Head + 1) % NumItems == Idx);
			Item = Slot;
			return true;
		}
		return false;
	}

	//remove an item at the tail position in LIFO order (this can be done from any thread including the one that accesses the head)
	inline bool Steal(uintptr_t& Item)
	{
		do
		{
			uint32 IdxVer = Tail.load(std::memory_order_acquire);
			uint32 Idx = IdxVer % NumItems;
			uintptr_t Slot = ItemSlots[Idx].Value.load(std::memory_order_acquire);

			if (Slot == uintptr_t(ESlotState::Free))
			{
				return false;
			}
			else if (Slot != uintptr_t(ESlotState::Taken) && ItemSlots[Idx].Value.compare_exchange_weak(Slot, uintptr_t(ESlotState::Taken), std::memory_order_acq_rel))
			{
				if(IdxVer == Tail.load(std::memory_order_acquire))
				{
					uint32 Prev = Tail.fetch_add(1, std::memory_order_release);
					checkSlow(Prev % NumItems == Idx);
					ItemSlots[Idx].Value.store(uintptr_t(ESlotState::Free), std::memory_order_release);
					Item = Slot;
					return true;
				}
				ItemSlots[Idx].Value.store(Slot, std::memory_order_release);
			}
		} while(true);
	}

private:
	struct FAlignedElement
	{
		alignas(PLATFORM_CACHE_LINE_SIZE * 2) std::atomic<uintptr_t> Value = {};
	};

	alignas(PLATFORM_CACHE_LINE_SIZE * 2) uint32 Head { ~0u };
	alignas(PLATFORM_CACHE_LINE_SIZE * 2) std::atomic_uint Tail { 0 };
	alignas(PLATFORM_CACHE_LINE_SIZE * 2) FAlignedElement ItemSlots[NumItems] = {};
};

template<typename Type, uint32 NumItems>
class TWorkStealingQueue2 final : protected TWorkStealingQueueBase2<NumItems>
{
	using PointerType = Type*;

public:
	inline bool Put(PointerType Item)
	{
		return TWorkStealingQueueBase2<NumItems>::Put(reinterpret_cast<uintptr_t>(Item));
	}

	inline bool Get(PointerType& Item)
	{
		return TWorkStealingQueueBase2<NumItems>::Get(reinterpret_cast<uintptr_t&>(Item));
	}

	inline bool Steal(PointerType& Item)
	{
		return TWorkStealingQueueBase2<NumItems>::Steal(reinterpret_cast<uintptr_t&>(Item));
	}
};
}

enum class ELocalQueueType
{
	EBackground,
	EForeground,
	EBusyWait,
};

/********************************************************************************************************************************************
 * A LocalQueueRegistry is a collection of LockFree queues that store pointers to Items, there are ThreadLocal LocalQueues with LocalItems. *
 * LocalQueues can only be Enqueued and Dequeued by the current Thread they were installed on. But Items can be stolen from any Thread      *
 * There is a global OverflowQueue than is used when a LocalQueue goes out of scope to dump all the remaining Items in                      *
 * or when a Thread has no LocalQueue installed or when the LocalQueue is at capacity. A new LocalQueue is registers itself always.         *
 * A Dequeue Operation can only be done starting from a LocalQueue, than the GlobalQueue will be checked.                                   *
 * Finally Items might get Stolen from other LocalQueues that are registered with the LocalQueueRegistry.                                   *
 ********************************************************************************************************************************************/
template<uint32 NumLocalItems = 1024>
class TLocalQueueRegistry
{
public:
	class TLocalQueue;

	// FOutOfWork is used to track the time while a worker is waiting for work
	// this happens after a worker was unable to aquire any task from the queues and until it finds work again or it goes into drowsing state.
	class FOutOfWork
	{
	private:
		template<uint32>
		friend class TLocalQueueRegistry;

		std::atomic_int& NumWorkersLookingForWork;
		bool ActivelyLookingForWork = false;

#if CPUPROFILERTRACE_ENABLED
		bool bCpuBeginEventEmitted = false;
#endif

		static uint32 WorkerLookingForWorkTraceId;

		inline FOutOfWork(std::atomic_int& InNumWorkersLookingForWork) : NumWorkersLookingForWork(InNumWorkersLookingForWork)
		{
		}

	public:
		inline ~FOutOfWork()
		{
			Stop();
		}

		inline bool Start()
		{
			if (!ActivelyLookingForWork)
			{
#if CPUPROFILERTRACE_ENABLED
				if (CpuChannel)
				{
					if (WorkerLookingForWorkTraceId == 0)
					{
						WorkerLookingForWorkTraceId = FCpuProfilerTrace::OutputEventType("TaskWorkerIsLookingForWork");
					}
					FCpuProfilerTrace::OutputBeginEvent(WorkerLookingForWorkTraceId);
					bCpuBeginEventEmitted = true;
				}
#endif
				NumWorkersLookingForWork.fetch_add(1, std::memory_order_relaxed);
				ActivelyLookingForWork = true;
				return true;
			}
			return false;
		}

		inline bool Stop()
		{
			if (ActivelyLookingForWork)
			{
#if CPUPROFILERTRACE_ENABLED
				if (bCpuBeginEventEmitted)
				{
					FCpuProfilerTrace::OutputEndEvent();
				}
#endif
				NumWorkersLookingForWork.fetch_sub(1, std::memory_order_release);
				ActivelyLookingForWork = false;
				return true;
			}
			return false;
		}
	};

private:
	using FLocalQueueType	 = LocalQueue_Impl::TWorkStealingQueue2<FTask, NumLocalItems>;
	using FOverflowQueueType = FAAArrayQueue<FTask>;
	using DequeueHazard		 = typename FOverflowQueueType::DequeueHazard;

private:
	// FLocalQueueCollection is a read only collection of LocalQueues registerd with this Registry
	struct FLocalQueueCollection
	{
		TArray<TLocalQueue*, TInlineAllocator<32>> LocalQueues;
		TLocalQueue* RemovedQueue = nullptr;

		FLocalQueueCollection() = default;

		~FLocalQueueCollection()
		{
			//if the registry also request deletion of a queue (removal case)
			if (RemovedQueue)
			{
				RemovedQueue->~TLocalQueue();
				FMemory::Free(RemovedQueue);
			}
		}
	};
	using FStealHazard = THazardPointer<FLocalQueueCollection, true>;

public:
	class TLocalQueue
	{
		template<uint32>
		friend class TLocalQueueRegistry;

	public:
		TLocalQueue(TLocalQueueRegistry& InRegistry, ELocalQueueType QueueType, FSleepEvent* InSleepEvent) : Registry(&InRegistry), SleepEvent(InSleepEvent)
		{
			checkSlow(Registry);
			StealHazard = FStealHazard(Registry->QueueCollection, Registry->HazardsCollection);
			AffinityIndex = Registry->AddLocalQueue(StealHazard, this, QueueType);
			for (int32 PriorityIndex = 0; PriorityIndex < int32(ETaskPriority::Count); PriorityIndex++)
			{
				DequeueHazards[PriorityIndex] = Registry->OverflowQueues[PriorityIndex].getHeadHazard();
			}
			AffinityHazard = AffinityQueue.getHeadHazard();
		}

		static TLocalQueue* AllocateLocalQueue(TLocalQueueRegistry& InRegistry, ELocalQueueType QueueType, FSleepEvent* InSleepEvent = nullptr)
		{
			void* Memory = FMemory::Malloc(sizeof(TLocalQueue), 128u);
			return new (Memory) TLocalQueue(InRegistry, QueueType, InSleepEvent);
		}

		//delete a queue
		//WorkeOwned means that the queue will not be automatically deleted when removal succeded.
		//It is a special case where the memory for the local queues is alloated linearly by the Scheduler for improved stealing performance.
		static void DeleteLocalQueue(TLocalQueue* Queue, ELocalQueueType QueueType, bool WorkerOwned = false)
		{
			TLocalQueueRegistry* Registry = Queue->Registry;
			Queue->Registry = nullptr;

			for (int32 PriorityIndex = 0; PriorityIndex < int32(ETaskPriority::Count); PriorityIndex++)
			{
				while (true)
				{
					FTask* Item;
					if (!Queue->LocalQueues[PriorityIndex].Get(Item))
					{
						break;
					}
					Registry->OverflowQueues[PriorityIndex].enqueue(Item);
				}
			}
			Registry->DeleteLocalQueue(Queue->StealHazard, Queue, QueueType, WorkerOwned);
		}

		//add an item to the local queue and overflow into the global queue if full
		// returns true if we should wake a worker
		inline bool Enqueue(FTask* Item, uint32 PriorityIndex)
		{
			checkSlow(Registry);
			checkSlow(PriorityIndex < int32(ETaskPriority::Count));
			checkSlow(Item != nullptr);

			bool bBackgroundTask = Item->IsBackgroundTask();
			if (!LocalQueues[PriorityIndex].Put(Item))
			{
				Registry->OverflowQueues[PriorityIndex].enqueue(Item);
			}
			return Registry->LessThanHalfWorkersLookingForWork(bBackgroundTask);
		}

		inline FTask* DequeueLocal(bool GetBackGroundTasks, bool bDisableThrottleStealing)
		{
			int32 MaxPriority = GetBackGroundTasks ? int32(ETaskPriority::Count) : int32(ETaskPriority::ForegroundCount);
			for (int32 PriorityIndex = 0; PriorityIndex < MaxPriority; PriorityIndex++)
			{
				FTask* Item;
				if (LocalQueues[PriorityIndex].Get(Item))
				{
					return Item;
				}
			}
			return nullptr;
		}

		inline FTask* DequeueGlobal(bool GetBackGroundTasks, bool bDisableThrottleStealing)
		{
			if (bDisableThrottleStealing || (Registry->NumActiveWorkers[GetBackGroundTasks].load(std::memory_order_relaxed) >= (2 * Registry->NumWorkersLookingForWork[GetBackGroundTasks].load(std::memory_order_relaxed) - 1)) || ((Random.GetUnsignedInt() % 4) == 0))
			{
				int32 MaxPriority = GetBackGroundTasks ? int32(ETaskPriority::Count) : int32(ETaskPriority::ForegroundCount);
				for (int32 PriorityIndex = 0; PriorityIndex < MaxPriority; PriorityIndex++)
				{
					FTask* Item = Registry->OverflowQueues[PriorityIndex].dequeue(DequeueHazards[PriorityIndex]);
					if (Item)
					{
						return Item;
					}
				}
			}
			return nullptr;
		}

		inline FTask* DequeueSteal(bool GetBackGroundTasks, bool bDisableThrottleStealing)
		{
			if (bDisableThrottleStealing || (Registry->NumActiveWorkers[GetBackGroundTasks].load(std::memory_order_relaxed) >= (2 * Registry->NumWorkersLookingForWork[GetBackGroundTasks].load(std::memory_order_relaxed) - 1)) || ((Random.GetUnsignedInt() % 4) == 0))
			{
				if (CachedRandomIndex == InvalidIndex)
				{
					CachedRandomIndex = Random.GetUnsignedInt();
				}

				FTask* Result = Registry->StealItem(StealHazard, CachedRandomIndex, CachedPriorityIndex, GetBackGroundTasks);
				if (Result)
				{
					return Result;
				}
			}
			return nullptr;
		}

		inline FTask* DequeueAffinity(bool GetBackGroundTasks, bool bDisableThrottleStealing)
		{
			return AffinityQueue.dequeue(AffinityHazard);
		}

		uint32 GetAffinityIndex() const
		{
			return AffinityIndex;
		}

	private:
		inline void EnqueueAffinity(FTask* Item)
		{
			check(SleepEvent != nullptr);
			AffinityQueue.enqueue(Item);
			
			ESleepState SleepState = ESleepState::Sleeping;
			ESleepState DrowsingState = ESleepState::Drowsing;
			if (SleepEvent->SleepState.compare_exchange_strong(DrowsingState, ESleepState::Affinity, std::memory_order_acquire))
			{
			}
			else if (SleepEvent->SleepState.compare_exchange_strong(SleepState, ESleepState::Drowsing, std::memory_order_acquire))
			{
				SleepEvent->SleepEvent->Trigger();
			}
		}

	private:
		static constexpr uint32	InvalidIndex = ~0u;
		FLocalQueueType			LocalQueues[uint32(ETaskPriority::Count)];
		DequeueHazard			DequeueHazards[uint32(ETaskPriority::Count)];
		FOverflowQueueType		AffinityQueue;
		DequeueHazard			AffinityHazard;
		FStealHazard			StealHazard;
		TLocalQueueRegistry*	Registry;
		FSleepEvent*			SleepEvent;
		FRandomStream			Random;
		uint32					CachedRandomIndex = InvalidIndex;
		uint32					CachedPriorityIndex = 0;
		uint32					AffinityIndex = ~0;
	};

	TLocalQueueRegistry()
	{
		QueueCollection.store(new FLocalQueueCollection(), std::memory_order_relaxed);
	}

private:
	// add a queue to the Registry
	uint32 AddLocalQueue(FStealHazard& Hazard, TLocalQueue* QueueToAdd, ELocalQueueType QueueType)
	{
		if (QueueType != ELocalQueueType::EBusyWait)
		{
			NumActiveWorkers[QueueType == ELocalQueueType::EBackground].fetch_add(1, std::memory_order_relaxed);
		}

		FLocalQueueCollection* Copy = new FLocalQueueCollection();
		while(true)
		{
			FLocalQueueCollection* Previous = Hazard.Get();
			Copy->LocalQueues = Previous->LocalQueues;	
			Copy->LocalQueues.Add(QueueToAdd);
			if (!QueueCollection.compare_exchange_strong(Previous, Copy, std::memory_order_acq_rel, std::memory_order_relaxed))
			{
				continue;
			}
			HazardsCollection.Delete(Previous);
			Hazard.Retire();
			return Previous->LocalQueues.Num();
		}
	}

	// remove a queue from the Registry
	void DeleteLocalQueue(FStealHazard& Hazard, TLocalQueue* QueueToRemove, ELocalQueueType QueueType, bool WorkerOwned)
	{
		if (QueueType != ELocalQueueType::EBusyWait)
		{
			NumActiveWorkers[QueueType == ELocalQueueType::EBackground].fetch_sub(1, std::memory_order_relaxed);
		}

		FLocalQueueCollection* Copy = new FLocalQueueCollection();
		while(true)
		{
			FLocalQueueCollection* Previous = Hazard.Get();
			Copy->LocalQueues = Previous->LocalQueues;	
			verifySlow(Copy->LocalQueues.Remove(QueueToRemove) == 1);
			if (!QueueCollection.compare_exchange_strong(Previous, Copy, std::memory_order_acq_rel, std::memory_order_relaxed))
			{
				continue;
			}
			if (!WorkerOwned)
			{
				checkSlow(Previous->RemovedQueue == nullptr);
				Previous->RemovedQueue = QueueToRemove;
			}		
			HazardsCollection.Delete(Previous);
			Hazard.Retire();
			return;
		}
	}

	// StealItem tries to steal an Item from a Registered LocalQueue
	FTask* StealItem(FStealHazard& Hazard, uint32& CachedRandomIndex, uint32& CachedPriorityIndex, bool GetBackGroundTasks)
	{
		FLocalQueueCollection* Queues = Hazard.Get();
		uint32 NumQueues = Queues->LocalQueues.Num();
		uint32 MaxPriority = GetBackGroundTasks ? int32(ETaskPriority::Count) : int32(ETaskPriority::ForegroundCount);
		CachedRandomIndex = CachedRandomIndex % NumQueues;

		for(uint32 i = 0; i < NumQueues; i++)
		{
			TLocalQueue* LocalQueue = Queues->LocalQueues[CachedRandomIndex];
			for(uint32 PriorityIndex = 0; PriorityIndex < MaxPriority; PriorityIndex++)
			{	
				FTask* Item;
				if (LocalQueue->LocalQueues[CachedPriorityIndex].Steal(Item))
				{
					Hazard.Retire();
					return Item;
				}
				CachedPriorityIndex = ++CachedPriorityIndex < MaxPriority ? CachedPriorityIndex : 0;
			}
			CachedRandomIndex = ++CachedRandomIndex < NumQueues ? CachedRandomIndex : 0;
		}
		CachedPriorityIndex = 0;
		CachedRandomIndex = TLocalQueue::InvalidIndex;
		Hazard.Retire();
		return nullptr;
	}

public:
	// enqueue an Item directy into the Global OverflowQueue
	// returns true if we should wake a worker for stealing
	bool Enqueue(FTask* Item, uint32 PriorityIndex)
	{
		check(PriorityIndex < int32(ETaskPriority::Count));
		check(Item != nullptr);

		bool bBackgroundTask = Item->IsBackgroundTask();
		OverflowQueues[PriorityIndex].enqueue(Item);

		return LessThanHalfWorkersLookingForWork(bBackgroundTask);
	}

	inline bool EnqueueAffinity(FTask* Item, uint32 AffinityIndex)
	{
		FStealHazard Hazard(QueueCollection, HazardsCollection);
		FLocalQueueCollection* Queues = Hazard.Get();

		if (AffinityIndex < uint32(Queues->LocalQueues.Num()) && Queues->LocalQueues[AffinityIndex]->SleepEvent)
		{
			Queues->LocalQueues[AffinityIndex]->EnqueueAffinity(Item);
			return true;
		}
		return false;
	}

	// grab an Item directy from the Global OverflowQueue
	FTask* Dequeue()
	{
		for (int32 PriorityIndex = 0; PriorityIndex < int32(ETaskPriority::Count); PriorityIndex++)
		{
			FTask* Result = OverflowQueues[PriorityIndex].dequeue();
			if (Result)
			{
				return Result;
			}
		}
		return nullptr;
	}

	inline FOutOfWork GetOutOfWorkScope(ELocalQueueType QueueType)
	{
		return FOutOfWork(NumWorkersLookingForWork[QueueType == ELocalQueueType::EBackground]);
	}

private:
	inline bool LessThanHalfWorkersLookingForWork(bool bBackgroundTask) const
	{
		if (bBackgroundTask)
		{
			return NumWorkersLookingForWork[true].load(std::memory_order_acquire) * 2 <= NumActiveWorkers[true].load(std::memory_order_acquire);
		}
		else
		{
			return (NumWorkersLookingForWork[false].load(std::memory_order_acquire) + NumWorkersLookingForWork[true].load(std::memory_order_acquire)) * 2 <= (NumActiveWorkers[false].load(std::memory_order_acquire) + NumActiveWorkers[true].load(std::memory_order_acquire));
		}
	}

	FOverflowQueueType	  OverflowQueues[uint32(ETaskPriority::Count)];
	FHazardPointerCollection		  HazardsCollection;
	std::atomic<FLocalQueueCollection*>	QueueCollection;
	std::atomic_int NumWorkersLookingForWork[2] = { {0}, {0} };
	std::atomic_int NumActiveWorkers[2] = { {0}, {0} };
};

template<uint32 NumLocalItems>
uint32 TLocalQueueRegistry<NumLocalItems>::FOutOfWork::WorkerLookingForWorkTraceId = 0;
}