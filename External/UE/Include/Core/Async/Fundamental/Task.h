// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Logging/LogMacros.h"
#include "TaskDelegate.h"
#include "HAL/Event.h"
#include "CoreTypes.h"
#include <atomic>

#define LOWLEVEL_TASK_SIZE PLATFORM_CACHE_LINE_SIZE

namespace LowLevelTasks
{
	DECLARE_LOG_CATEGORY_EXTERN(LowLevelTasks, Log, All);

	enum class ETaskPriority : int32
	{
		High,
		Normal,
		Default = Normal,
		ForegroundCount,
		BackgroundHigh = ForegroundCount,
		BackgroundNormal,
		BackgroundLow,
		Count,
		Inherit, //Inherit the TaskPriority from the launching Task or the Default Priority if not launched from a Task.
	};

	inline const TCHAR* ToString(ETaskPriority Priority)
	{
		if (Priority < ETaskPriority::High || Priority >= ETaskPriority::Count)
		{
			return nullptr;
		}

		const TCHAR* TaskPriorityToStr[] =
		{
			TEXT("High"),
			TEXT("Normal"),
			TEXT("BackgroundHigh"),
			TEXT("BackgroundNormal"),
			TEXT("BackgroundLow")
		};
		return TaskPriorityToStr[(int32)Priority];
	}

	inline bool ToTaskPriority(const TCHAR* PriorityStr, ETaskPriority& OutPriority)
	{
		if (FCString::Stricmp(PriorityStr, ToString(ETaskPriority::High)) == 0)
		{
			OutPriority = ETaskPriority::High;
			return true;
		}

		if (FCString::Stricmp(PriorityStr, ToString(ETaskPriority::Normal)) == 0)
		{
			OutPriority = ETaskPriority::Normal;
			return true;
		}

		if (FCString::Stricmp(PriorityStr, ToString(ETaskPriority::BackgroundHigh)) == 0)
		{
			OutPriority = ETaskPriority::BackgroundHigh;
			return true;
		}

		if (FCString::Stricmp(PriorityStr, ToString(ETaskPriority::BackgroundNormal)) == 0)
		{
			OutPriority = ETaskPriority::BackgroundNormal;
			return true;
		}

		if (FCString::Stricmp(PriorityStr, ToString(ETaskPriority::BackgroundLow)) == 0)
		{
			OutPriority = ETaskPriority::BackgroundLow;
			return true;
		}

		return false;
	}

	enum class ETaskState : int32
	{
		CanceledAndCompleted,	//means the task is completed with execution of it's continuation but the runnable was cancelled
		Completed,				//means the task is completed with execution or the default when we create a handle
		Ready,					//means the Task is ready to be launched
		Canceled,				//means the task was canceled and launched and therefore queued for execution by a worker (which already might be executing it's continuation)
		CanceledAndReady,		//means the task was canceled and is ready to be launched (it still is required to be launched)
		Scheduled,				//means the task is launched and therefore queued for execution by a worker
		Running,				//means the task is executing it's runnable and continuation by a worker 
		Count
	};
	
	//Generic implementation of a Deleter, it often comes up that one has to call a function to cleanup after a Task finished
	//this can be done by capturing a TDeleter like this in the lambda of the Continuation: [Deleter(LowLevelTasks::TDeleter<Type, &Type::DeleteFunction>(value))](){}
	template<typename Type, void (Type::*DeleteFunction)()>
	class TDeleter
	{
		Type* Value;

	public:
		inline TDeleter(Type* InValue) : Value(InValue)
		{
		}

		inline TDeleter(const TDeleter&) = delete;
		inline TDeleter(TDeleter&& Other) : Value(Other.Value)
		{
			Other.Value = nullptr;
		}

		inline Type* GetValue() const
		{
			return Value;
		}

		inline ~TDeleter() 
		{
			if(Value)
			{
				(Value->*DeleteFunction)();
			}
		}
	};

	//this class is just here to hide some variables away
	//because we don't want to become too close friends with the FScheduler
	class FTask;
	namespace Tasks_Impl
	{
	class FTaskBase
	{
		friend class ::LowLevelTasks::FTask;
		UE_NONCOPYABLE(FTaskBase); //means non movable

		class FPackedData
		{
			uintptr_t DebugName			: 57;
			uintptr_t Priority			: 3;
			uintptr_t State				: 3;
			uintptr_t bAllowBusyWaiting : 1;
			
		public:
			FPackedData() : FPackedData(nullptr,  ETaskPriority::Default, ETaskState::Completed, true)
			{
				static_assert(!PLATFORM_32BITS, "32bit Platforms are not supported");
				static_assert(uintptr_t(ETaskPriority::Count) <= (1ull << 3), "Not enough bits to store ETaskPriority");
				static_assert(uintptr_t(ETaskState::Count) <= (1ull << 3), "Not enough bits to store ETaskState");
			}

			FPackedData(const TCHAR* InDebugName, ETaskPriority InPriority, ETaskState InState, bool bInAllowBusyWaiting)
				: DebugName(reinterpret_cast<uintptr_t>(InDebugName))
				, Priority((uintptr_t)InPriority)
				, State((uintptr_t)InState)
				, bAllowBusyWaiting(bInAllowBusyWaiting)
			{
				checkSlow(reinterpret_cast<uintptr_t>(InDebugName) < (1ull << 57));
				checkSlow((uintptr_t)InPriority < (1ull << 3));
				checkSlow((uintptr_t)InState < (1ull << 3));
				static_assert(sizeof(FPackedData) == sizeof(uintptr_t), "Packed data needs to be pointer size");
			}

			FPackedData(FPackedData& Other, ETaskState State)
				: FPackedData(Other.GetDebugName(), Other.GetPriority(), State, Other.AllowBusyWaiting())
			{
			}

			const TCHAR* GetDebugName() const
			{
				return reinterpret_cast<const TCHAR*>(DebugName);
			}

			ETaskPriority GetPriority() const
			{
				return ETaskPriority(Priority);
			}

			ETaskState GetState() const
			{
				return ETaskState(State);
			}

			bool AllowBusyWaiting() const
			{
				return bAllowBusyWaiting;
			}
		};

	private:
		using FTaskDelegate = TTaskDelegate<void(bool), LOWLEVEL_TASK_SIZE - sizeof(FPackedData) - sizeof(void*)>;
		FTaskDelegate Runnable;
		mutable void* UserData = nullptr;
		std::atomic<FPackedData> PackedData { FPackedData() };

	private:
		FTaskBase() = default;

	private:
		inline bool IsCanceled() const { return PackedData.load(std::memory_order_relaxed).GetState() == ETaskState::Canceled; }
		inline bool IsScheduled() const { return PackedData.load(std::memory_order_relaxed).GetState() == ETaskState::Scheduled; }
	};
	}

	//minimal core task interface
	class FTask final : private Tasks_Impl::FTaskBase
	{		
		friend class FScheduler;
		UE_NONCOPYABLE(FTask); //means non movable

		static thread_local FTask* ActiveTask;

	public: //Public Interface
		//means the task is completed and this taskhandle can be recycled
		inline bool IsCompleted(std::memory_order MemoryOrder = std::memory_order_seq_cst) const
		{
			ETaskState State = PackedData.load(MemoryOrder).GetState();
			return State == ETaskState::CanceledAndCompleted || State == ETaskState::Completed;
		}
		
		//means the task was canceled but might still need to be launched 
		inline bool WasCanceled() 		
		{
			ETaskState State = PackedData.load(std::memory_order_relaxed).GetState();
			return State == ETaskState::CanceledAndReady || State == ETaskState::Canceled || State == ETaskState::CanceledAndCompleted;
		}

		//means the task is ready to be launched but might already been canceled 
		inline bool IsReady() const 
		{
			ETaskState State = PackedData.load(std::memory_order_relaxed).GetState();
			return State == ETaskState::Ready || State == ETaskState::CanceledAndReady; 
		}

#if PLATFORM_DESKTOP
		//get the currently active task if any
		CORE_API static const FTask* GetActiveTask();
#else
		FORCEINLINE static const FTask* GetActiveTask()
		{
			return ActiveTask;
		}
#endif

		//try to cancel the task if it has not been launched yet and ExecuteTaskOnSuccess is true the continuation will run immediately.
		inline bool TryCancel(bool ExecuteTaskOnSuccess = true);

		//try to execute the task if it has not been launched yet the task will execute immediately.
		inline bool TryExecute();

		template<typename TRunnable, typename TContinuation>
		inline void Init(const TCHAR* InDebugName, ETaskPriority InPriority, TRunnable&& InRunnable, TContinuation&& InContinuation, bool bAllowBusyWaiting = true);

		template<typename TRunnable>
		inline void Init(const TCHAR* InDebugName, ETaskPriority InPriority, TRunnable&& InRunnable, bool bAllowBusyWaiting = true);

		template<typename TRunnable, typename TContinuation>
		inline void Init(const TCHAR* InDebugName, TRunnable&& InRunnable, TContinuation&& InContinuation, bool bAllowBusyWaiting = true);

		template<typename TRunnable>
		inline void Init(const TCHAR* InDebugName, TRunnable&& InRunnable, bool bAllowBusyWaiting = true);

		inline const TCHAR* GetDebugName() const;
		inline ETaskPriority GetPriority() const;
		inline bool IsBackgroundTask() const;
		inline bool AllowBusyWaiting() const;

		struct FInitData
		{
			const TCHAR* DebugName;
			ETaskPriority Priority;
			bool bAllowBusyWaiting;
		};
		inline FInitData GetInitData() const;

		void* GetUserData() const { return UserData; }
		void SetUserData(void* NewUserData) const { UserData = NewUserData; }

	public:
		FTask() = default;
		inline ~FTask();

	private: //Interface of the Scheduler
		inline static bool PermitBackgroundWork()
		{
			return ActiveTask && ActiveTask->IsBackgroundTask();
		}

		inline bool TryPrepareLaunch();
		//after calling this function the task can be considered dead
		inline void ExecuteTask();
		inline void InheritParentData(ETaskPriority& Priority);
	};

   /******************
	* IMPLEMENTATION *
	******************/

	inline ETaskPriority FTask::GetPriority() const 
	{ 
		return PackedData.load(std::memory_order_relaxed).GetPriority(); 
	}

	inline void FTask::InheritParentData(ETaskPriority& Priority)
	{
		const FTask* LocalActiveTask = FTask::GetActiveTask();
		if (LocalActiveTask != nullptr)
		{
			if (Priority == ETaskPriority::Inherit)
			{
				Priority = LocalActiveTask->GetPriority();
			}
			UserData = LocalActiveTask->GetUserData();
		}
		else
		{
			if (Priority == ETaskPriority::Inherit)
			{
				Priority = ETaskPriority::Default;
			}
			UserData = nullptr;
		}
	}

	template<typename TRunnable, typename TContinuation>
	inline void FTask::Init(const TCHAR* InDebugName, ETaskPriority InPriority, TRunnable&& InRunnable, TContinuation&& InContinuation, bool bAllowBusyWaiting)
	{
		checkf(IsCompleted(), TEXT("State: %d"), PackedData.load(std::memory_order_relaxed).GetState());
		checkSlow(!Runnable.IsSet());
		Runnable = [LocalRunnable = Forward<TRunnable>(InRunnable), LocalContinuation = Forward<TContinuation>(InContinuation)](const bool NotCanceled)
		{
			if (NotCanceled)
			{
				LocalRunnable();
			}
			LocalContinuation();
		};
		InheritParentData(InPriority);
		PackedData.store(FPackedData(InDebugName, InPriority, ETaskState::Ready, bAllowBusyWaiting), std::memory_order_release);
	}

	template<typename TRunnable>
	inline void FTask::Init(const TCHAR* InDebugName, ETaskPriority InPriority, TRunnable&& InRunnable, bool bAllowBusyWaiting)
	{
		checkf(IsCompleted(), TEXT("State: %d"), PackedData.load(std::memory_order_relaxed).GetState());
		checkSlow(!Runnable.IsSet());
		Runnable = [LocalRunnable = Forward<TRunnable>(InRunnable)](const bool NotCanceled)
		{
			if (NotCanceled)
			{
				LocalRunnable();
			}
		};
		InheritParentData(InPriority);
		PackedData.store(FPackedData(InDebugName, InPriority, ETaskState::Ready, bAllowBusyWaiting), std::memory_order_release);
	}

	template<typename TRunnable, typename TContinuation>
	inline void FTask::Init(const TCHAR* InDebugName, TRunnable&& InRunnable, TContinuation&& InContinuation, bool bAllowBusyWaiting)
	{
		Init(InDebugName, ETaskPriority::Default, Forward<TRunnable>(InRunnable), Forward<TContinuation>(InContinuation), bAllowBusyWaiting);
	}

	template<typename TRunnable>
	inline void FTask::Init(const TCHAR* InDebugName, TRunnable&& InRunnable, bool bAllowBusyWaiting)
	{
		Init(InDebugName, ETaskPriority::Default, Forward<TRunnable>(InRunnable), bAllowBusyWaiting);
	}

	inline FTask::~FTask()
	{
		checkf(IsCompleted(), TEXT("State: %d"), PackedData.load(std::memory_order_relaxed).GetState());
	}

	inline bool FTask::TryPrepareLaunch()
	{
		FPackedData LocalPackedData = PackedData.load(std::memory_order_relaxed);
		FPackedData ReadyState = FPackedData(LocalPackedData, ETaskState::Ready);
		FPackedData CanceledAndReadyState = FPackedData(LocalPackedData, ETaskState::CanceledAndReady);
		return PackedData.compare_exchange_strong(ReadyState, FPackedData(LocalPackedData, ETaskState::Scheduled), std::memory_order_acquire, std::memory_order_relaxed)
			|| PackedData.compare_exchange_strong(CanceledAndReadyState, FPackedData(LocalPackedData, ETaskState::Canceled), std::memory_order_acquire, std::memory_order_relaxed);
	}

	inline bool FTask::TryCancel(bool ExecuteTaskOnSuccess)
	{
		FPackedData LocalPackedData = PackedData.load(std::memory_order_relaxed);
		FPackedData ReadyState(LocalPackedData, ETaskState::Ready);
		FPackedData ScheduledState(LocalPackedData, ETaskState::Scheduled);
		//we can use memory_order_relaxed in this case because, when a task is canceled it does not launch the task
		//to launch a canceled  task it has to go though TryPrepareLaunch which is doing the memory_order_acquire
		bool WasCanceled = PackedData.compare_exchange_strong(ReadyState, FPackedData(LocalPackedData, ETaskState::CanceledAndReady), std::memory_order_relaxed)
			|| PackedData.compare_exchange_strong(ScheduledState, FPackedData(LocalPackedData, ETaskState::Canceled), std::memory_order_relaxed);

		if(ExecuteTaskOnSuccess && WasCanceled && TryPrepareLaunch())
		{
			ExecuteTask();
			return true;
		}
		return WasCanceled;
	}

	inline bool FTask::TryExecute()
	{
		if(TryPrepareLaunch())
		{
			ExecuteTask();
			return true;
		}
		return false;
	}

	inline void FTask::ExecuteTask()
	{
		checkSlow(Runnable.IsSet());
		FTaskDelegate LocalRunnable;

		checkSlow(IsScheduled() || IsCanceled());
		FPackedData LocalPackedData = PackedData.load(std::memory_order_relaxed);
		FPackedData ScheduledState(LocalPackedData, ETaskState::Scheduled);
		const bool NotCanceled = PackedData.compare_exchange_strong(ScheduledState, FPackedData(LocalPackedData, ETaskState::Running), std::memory_order_relaxed);
		
		Runnable.CallAndMove(LocalRunnable, NotCanceled);

		checkSlow(!Runnable.IsSet());
		//do not access the task again after this call
		//as by defitition the task can be considered dead
		PackedData.store(FPackedData(LocalPackedData, NotCanceled ? ETaskState::Completed : ETaskState::CanceledAndCompleted), std::memory_order_seq_cst);
	}

	inline const TCHAR* FTask::GetDebugName() const
	{
		return PackedData.load(std::memory_order_relaxed).GetDebugName(); 
	}

	inline bool FTask::IsBackgroundTask() const
	{
		return PackedData.load(std::memory_order_relaxed).GetPriority() >= ETaskPriority::ForegroundCount;
	}

	inline bool FTask::AllowBusyWaiting() const
	{
		return PackedData.load(std::memory_order_relaxed).AllowBusyWaiting();
	}

	inline FTask::FInitData FTask::GetInitData() const
	{
		FPackedData LocalPackedData = PackedData.load(std::memory_order_relaxed);
		return { LocalPackedData.GetDebugName(), LocalPackedData.GetPriority(), LocalPackedData.AllowBusyWaiting() };
	}

	enum class ESleepState
	{
		Affinity,
		Running,
		Drowsing,
		Sleeping,
	};

	//the struct is naturally 64 bytes aligned, the extra alignment just 
	//re-enforces this assumption and will error if it changes in the future
	struct alignas(64) FSleepEvent
	{
		FEventRef SleepEvent;
		std::atomic<ESleepState> SleepState { ESleepState::Running };
		FSleepEvent* Next = nullptr;
	};
}