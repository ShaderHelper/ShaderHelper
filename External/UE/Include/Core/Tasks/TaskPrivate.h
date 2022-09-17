// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Async/Fundamental/Task.h"
#include "Async/Fundamental/Scheduler.h"
#include "Async/TaskTrace.h"
#include "Templates/Invoke.h"
#include "Templates/TypeCompatibleBytes.h"
#include "Misc/Timeout.h"
#include "Containers/ClosableMpscQueue.h"
#include "Containers/SpscQueue.h"
#include "Templates/UnrealTemplate.h"
#include "Templates/RefCounting.h"
#include "CoreTypes.h"
#include "Math/NumericLimits.h"
#include "Misc/SpinLock.h"
#include "Misc/ScopeLock.h"

#include <atomic>
#include <type_traits>

namespace UE { namespace Tasks
{
	using LowLevelTasks::ETaskPriority;

	class FPipe;

	namespace Private
	{
		// intrusive atomic reference-counting base class.
		class FRefCountedBase
		{
		public:
			explicit FRefCountedBase(uint32 InitRefCount = 0)
				: RefCount{ InitRefCount }
			{}

			virtual ~FRefCountedBase() = default;

			void AddRef()
			{
				RefCount.fetch_add(1, std::memory_order_relaxed);
			}

			void Release()
			{
				uint32 LocalRefCount = RefCount.fetch_sub(1, std::memory_order_release) - 1;
				if (LocalRefCount == 0)
				{
					std::atomic_thread_fence(std::memory_order_acquire);
					delete this;
				}
			}

		private:
			std::atomic<uint32> RefCount;
		};

		// A base class for high-level task implementation.
		// Must be dynamically allocated and used with TRefCountPtr.
		// @see Tasks::FTaskBase
		class CORE_API FTaskBase : public FRefCountedBase
		{
			UE_NONCOPYABLE(FTaskBase);

			// `ExecutionFlag` is set at the beginning of execution as the most significant bit of `NumLocks` and indicates a switch 
			// of `NumLocks` from "execution prerequisites" (a number of uncompleted prerequisites that block task execution) to 
			// "completion prerequisites" (a number of nested uncompleted tasks that block task completion)
			static constexpr uint32 ExecutionFlag = 0x80000000;

		public:
			FTaskBase()
				: FRefCountedBase(2) // for the initial reference (we don't increment it on passing to `TRefCountPtr`),
				// and for the reference passed to the scheduler - is released when scheduler's "runnable" lambda is destroyed (see `Init`)
			{}

			virtual ~FTaskBase() override
			{
				check(IsCompleted());
			}

			// a special internal task priority for "inline" task execution - a task is executed as soon as it's launched and has no 
			// pending dependencies, "inline", w/o scheduling
			static constexpr ETaskPriority InlineTaskPriority{ ETaskPriority::Count };

			// initialises the task but doesn't launches it
			template<typename TaskBodyType>
			void Init(const TCHAR* DebugName, TaskBodyType&& InTaskBody, LowLevelTasks::ETaskPriority Priority)
			{
				TaskBody = Forward<TaskBodyType>(InTaskBody);

				LowLevelTask.Init(DebugName, Priority,
					[
						this,
						// releasing scheduler's task reference can cause task's automatic destruction and so must be done after the low-level task
						// task is flagged as completed. The task is flagged as completed after the continuation is executed but before its destroyed.
						// `Deleter` is captured by value and is destroyed along with the continuation, calling the given functor on destruction
						Deleter = LowLevelTasks::TDeleter<FRefCountedBase, &FRefCountedBase::Release>{ this }
					] 
					{
						TryExecute();
					}
				);
			}

			// The task will be executed only when all prerequisites are completed. The task type must be a task handle that holds a pointer to
			// FTaskBase as its `Pimpl` member (see Tasks::TTaskBase).
			// Must not be called concurrently
			void AddPrerequisites(FTaskBase& Prerequisite)
			{
				checkf(NumLocks.load(std::memory_order_relaxed) >= NumInitialLocks && NumLocks.load(std::memory_order_relaxed) < ExecutionFlag, TEXT("Prerequisites can be added only before the task is launched"));

				// registering the task as a subsequent of the given prerequisite can cause its immediate launch by the prerequisite
				// (if the prerequisite has been completed on another thread), so we need to keep the task locked by assuming that the 
				// prerequisite can be added successfully, and release the lock if it wasn't
				uint32 PrevNumLocks = NumLocks.fetch_add(1, std::memory_order_acquire); // `acquire` to make it happen before the task is registered as a subsequent
				checkf(PrevNumLocks + 1 < ExecutionFlag, TEXT("Max number of nested tasks reached: %d"), ExecutionFlag);

				if (Prerequisite.AddSubsequent(*this))
				{
					Prerequisites.Enqueue(&Prerequisite);
					// keep it alive until this task's execution
					Prerequisite.AddRef();
				}
				else
				{
					// failed to add the prerequisite (too late), correct the number
					NumLocks.fetch_sub(1, std::memory_order_release); // `release` to make it happen after the task is registered as a subsequent
				}
			}

			// The task will be executed only when all prerequisites are completed. The task type must be a task handle that holds a pointer to
			// FTaskBase as its `Pimpl` member (see Tasks::TTaskBase).
			// Must not be called concurrently
			template<typename HigherLevelTaskType, decltype(std::declval<HigherLevelTaskType>().Pimpl)* = nullptr>
			void AddPrerequisites(const HigherLevelTaskType& Prerequisite)
			{
				AddPrerequisites(*Prerequisite.Pimpl);
			}

			// The task will be executed only when all prerequisites are completed.
			// Must not be called concurrently.
			// @param InPrerequisites - an iterable collection of tasks
			template<typename PrerequisiteCollectionType, decltype(std::declval<PrerequisiteCollectionType>().begin())* = nullptr>
			void AddPrerequisites(const PrerequisiteCollectionType& InPrerequisites)
			{
				checkf(NumLocks.load(std::memory_order_relaxed) >= NumInitialLocks && NumLocks.load(std::memory_order_relaxed) < ExecutionFlag, TEXT("Prerequisites can be added only before the task is launched"));

				// registering the task as a subsequent of the given prerequisite can cause its immediate launch by the prerequisite
				// (if the prerequisite has been completed on another thread), so we need to keep the task locked by assuming that the 
				// prerequisite can be added successfully, and release the lock if it wasn't
				uint32 PrevNumLocks = NumLocks.fetch_add(GetNum(InPrerequisites), std::memory_order_acquire); // `acquire` to make it happen before the task is registered as a subsequent
				checkf(PrevNumLocks + GetNum(InPrerequisites) < ExecutionFlag, TEXT("Max number of nested tasks reached: %d"), ExecutionFlag);

				uint32 NumCompletedPrerequisites = 0;
				for (auto& Prereq : InPrerequisites)
				{
					// prerequisites can be either `FTaskBase*` or its Pimpl handle
					FTaskBase* Prerequisite;
					using FPrerequisiteType = std::decay_t<decltype(*std::declval<PrerequisiteCollectionType>().begin())>;
					if constexpr (std::is_same_v<FPrerequisiteType, FTaskBase*>)
					{
						Prerequisite = Prereq;
					}
					else
					{
						Prerequisite = Prereq.Pimpl;
					}

					if (Prerequisite->AddSubsequent(*this))
					{
						Prerequisites.Enqueue(Prerequisite);
						// keep it alive until this task's execution
						Prerequisite->AddRef();
					}
					else
					{
						++NumCompletedPrerequisites;
					}
				}

				// unlock for prerequisites that weren't added
				NumLocks.fetch_sub(NumCompletedPrerequisites, std::memory_order_release); // `release` to make it happen after the task is registered as a subsequent
			}

			// the task unlocks all its subsequents on completion.
			// returns false if the task is already completed and the subsequent wasn't added
			bool AddSubsequent(FTaskBase& Subsequent)
			{
				return Subsequents.Enqueue(&Subsequent);
			}

			// A piped task is executed after the previous task from this pipe is completed. Tasks from the same pipe are not executed
			// concurrently (so don't require synchronization), but not necessarily on the same thread.
			// @See FPipe
			void SetPipe(FPipe& InPipe)
			{
				Pipe = &InPipe;
			}

			FPipe* GetPipe() const
			{
				return Pipe;
			}

			// Tries to schedule task execution. Returns false if the task has incomplete dependencies (prerequisites or is blocked by a pipe). 
			// In this case the task will be automatically scheduled when all dependencies are completed.
			bool TryLaunch()
			{
				TaskTrace::Launched(GetTraceId(), LowLevelTask.GetDebugName(), true, (ENamedThreads::Type)0xff);
				return TryUnlock();
			}

			bool IsCompleted() const
			{
				return Subsequents.IsClosed();
			}

			// Tries to pull out the task from the scheduler and execute it. Returns false if task execution is already started.
			bool TryRetractAndExecute(uint32 RecursionDepth = 0)
			{
				if (IsCompleted())
				{
					return true;
				}

				// avoid stack overflow. is not expected in a real-life cases but happens in stress tests
				if (RecursionDepth == 200)
				{
					return false;
				}
				++RecursionDepth;

				// prevent concurrent retraction from multiple threads. while it's desirable to allow this (so different threads retract and
				// execute prerequisites in parallel), multiple threads waiting for the same task with multiple not completed prerequisites is expected
				// to happen very rarely, and the implementation would be much more complicated potentially leading to a slower common-case
				// version
				if (!TryGetExecutionPermission())
				{
					return false;
				}

				auto IsLocked = [this]
				{
					return NumLocks.load(std::memory_order_acquire) != (Pipe == nullptr ? 1 : 0);
				};

				if (IsLocked())
				{
					// try to unlock the task. even if prerequisites retraction fails we still need to proceed to try to execute the task here in case
					// prerequisites were completed in parallel

					// prerequisites are "consumed" here even if their retraction fails. retraction can fail only if task execution has 
					// already started, and it can't become "retractable" again, so no need to keep them
					while (TOptional<FTaskBase*> Prerequisite = Prerequisites.Dequeue())
					{
						Prerequisite.GetValue()->TryRetractAndExecute(RecursionDepth);
						Prerequisite.GetValue()->Release();
					}
				}

				// the task can be still locked if either prerequisite retraction (above) failed, or the task was piped by another thread, or the task 
				// wasn't launched yet (Tasks::FTaskEvent can be used as a prerequisites before it's triggered)
				if (IsLocked())
				{
					RevokeExecutionPermission();
					// prerequisites could be completed in parallel after `IsLocked()` and before we revoked execution permission, so the worker who
					// unlocked the task won't be able to execute it. double check if the task is still locked and if not - try to execute it
					if (IsLocked() || !TryGetExecutionPermission()) // if it's the case, try to get back execution permission
					{
						return false;
					}
				}

				// the task is unlocked and we have execution permission
				DoExecute();
				// no need to cancel task execution by the scheduler, when the scheduler will execute its runnable, it will fail to get execution
				// permissions and will do nothing
				// LowLevelTask.TryCancel();

				if (IsCompleted()) // still can be hold back by nested tasks
				{
					return true;
				}

				// retract nested tasks. this can happen concurrently with `Close()` called by a nested task, that also consumes `Prerequisite`. 
				// `Prerequisites` is a single-producer/single-consumer queue so we need to put an aditional synchronisation for dequeueing
				{
					TScopeLock<FSpinLock> ScopeLock(PrerequisitesLock);

					// keep trying retracting all nested tasks even if some of them fail, so the current worker can contribute instead of being blocked
					bool bSucceeded = true;
					// prerequisites are "consumed" here even if their retraction fails. retraction can fail only if task execution has 
					// already started, and it can't become "retractable" again, so no need to keep them
					while (TOptional<FTaskBase*> Prerequisite = Prerequisites.Dequeue())
					{
						TScopeUnlock<FSpinLock> ScopeUnlock(PrerequisitesLock); // only `Prerequisites.Dequeue()` needs to be locked

						if (!Prerequisite.GetValue()->TryRetractAndExecute(RecursionDepth))
						{
							bSucceeded = false;
						}
						Prerequisite.GetValue()->Release();
					}

					if (!bSucceeded)
					{
						return false;
					}
				}

				// it happens that all nested tasks are completed and are in the process of completing the parent (this task) concurrently, but the flag
				// is not set yet. wait for it
				while (!IsCompleted())
				{
					FPlatformProcess::Yield();
				}

				return true;
			}

			// adds a nested task that must be completed before the parent (this) is completed
			void AddNested(FTaskBase& Nested)
			{
				uint32 PrevNumLocks = NumLocks.fetch_add(1, std::memory_order_relaxed); // in case we'll succeed in adding subsequent
				checkf(PrevNumLocks + 1 < TNumericLimits<uint32>::Max(), TEXT("Max number of nested tasks reached: %d"), TNumericLimits<uint32>::Max() - ExecutionFlag);
				checkf(PrevNumLocks > ExecutionFlag, TEXT("Internal error: nested tasks can be added only during parent's execution (%u)"), PrevNumLocks);

				if (Nested.AddSubsequent(*this))
				{
					Prerequisites.Enqueue(&Nested);
					// keep it alive until the task destruction
					Nested.AddRef();
				}
				else
				{
					NumLocks.fetch_sub(1, std::memory_order_relaxed);
				}
			}

			// waits for task's completion, with optional timeout. Tries to retract the task and execute it in-place, if failed - blocks until the task 
			// is completed by another thread. If timeout is zero, tries to retract the task and returns immedially after that.
			// @return true if the task is completed
			bool Wait(FTimespan Timeout = FTimespan::MaxValue())
			{
				TaskTrace::FWaitingScope WaitingScope(GetTraceId());
				TRACE_CPUPROFILER_EVENT_SCOPE(Tasks::Wait);

				if (TryRetractAndExecute())
				{
					return true;
				}

				// the event must be alive for the task and this function lifetime, we don't know which one will be finished first as waiting can time out
				// before the waiting task is completed
				FSharedEventRef CompletionEvent;

				TRefCountPtr<Private::FTaskBase> WaitingTask{ new Private::FTaskBase, /*bAddRef=*/ false };
				WaitingTask->Init(TEXT("Waiting Task"), [CompletionEvent] { CompletionEvent->Trigger(); }, Private::FTaskBase::InlineTaskPriority);
				WaitingTask->AddPrerequisites(*this);

				if (WaitingTask->TryLaunch())
				{	// was executed inline
					check(WaitingTask->IsCompleted());
					return true;
				}

				return CompletionEvent->Wait(Timeout);
			}

			// waits until the task is completed while executing other tasks
			void BusyWait()
			{
				TaskTrace::FWaitingScope WaitingScope(GetTraceId());
				TRACE_CPUPROFILER_EVENT_SCOPE(Tasks::BusyWait);
				
				if (!TryRetractAndExecute())
				{
					LowLevelTasks::BusyWaitUntil([this] { return IsCompleted(); });
				}
			}

			// waits until the task is completed or waiting timed out, while executing other tasks
			bool BusyWait(FTimespan InTimeout)
			{
				TaskTrace::FWaitingScope WaitingScope(GetTraceId());
				TRACE_CPUPROFILER_EVENT_SCOPE(Tasks::BusyWait);

				FTimeout Timeout{ InTimeout };
				
				if (TryRetractAndExecute())
				{
					return true;
				}

				LowLevelTasks::BusyWaitUntil([this, Timeout] { return IsCompleted() || Timeout; });
				return IsCompleted();
			}

			// waits until the task is completed or the condition returns true, while executing other tasks
			template<typename ConditionType>
			bool BusyWait(ConditionType&& Condition)
			{
				TaskTrace::FWaitingScope WaitingScope(GetTraceId());
				TRACE_CPUPROFILER_EVENT_SCOPE(Tasks::BusyWait);

				if (TryRetractAndExecute())
				{
					return true;
				}

				LowLevelTasks::BusyWaitUntil(
					[this, Condition = Forward<ConditionType>(Condition)]{ return IsCompleted() || Condition(); }
				);
				return IsCompleted();
			}

			TaskTrace::FId GetTraceId() const
			{
#if UE_TASK_TRACE_ENABLED
				return TraceId;
#else
				return TaskTrace::InvalidId;
#endif
			}

			// returns the task that is being currently exeucuted by this thread, if any
			static FTaskBase* GetCurrentTask();

		private:
			// sets the current task and returns the previous current task
			static FTaskBase* ExchangeCurrentTask(FTaskBase* Task);

			// A task can be locked for execution (by prerequisites or if it's not launched yet) or for completion (by nested tasks).
			// This method is called to unlock the task and so can result in its scheduling (and execution) or completion
			bool TryUnlock()
			{
				uint32 PrevNumLocks = NumLocks.fetch_sub(1, std::memory_order_acq_rel); // `acq_rel` to make it happen after task 
				// preparation and before launching it
				if (PrevNumLocks < ExecutionFlag)
				{
					checkf(PrevNumLocks != (GetPipe() == nullptr ? 1 : 0), TEXT("The task is not locked"));
					return TrySchedule(PrevNumLocks - 1);
				}

				checkf(PrevNumLocks != ExecutionFlag, TEXT("The task is not locked"));
				return TryComplete(PrevNumLocks - 1);
			}

			// tries to pass the task to the scheduler for eventual execution by checking if something holds it back.
			// tasks with inline priority can be executed right here.
			bool TrySchedule(uint32 LocalNumLocks)
			{
				const bool bReadyForPiping = LocalNumLocks <= 1; // the only lock that can remain is pipe's one
				if (!bReadyForPiping)
				{
					return false;
				}

				if (!TryPushIntoPipe(LocalNumLocks))
				{
					return false; // pipe is blocked
				}

				TaskTrace::Scheduled(GetTraceId());

				// "inline" tasks are not scheduled but executed as soon as they are unlocked
				if (LowLevelTask.GetPriority() == InlineTaskPriority)
				{
					// execute before cancelling the low-level task as successful cancellation can release the last reference and destroy the task
					// the low-level task wasn't scheduled, so successful execution and low-level task cancellation is guaranted
					TryExecute();
					verify(LowLevelTask.TryCancel());
					return true;
				}

				verify(LowLevelTasks::TryLaunch(LowLevelTask)); // schedule the task

				return true;
			}

			// the task is already executed but it's not completed yet. This method sets completion flag if there're no pending nested tasks.
			// The task can be deleted as the result of this call.
			bool TryComplete(uint32 LocalNumLocks)
			{
				if (LocalNumLocks == ExecutionFlag)
				{
					checkSlow(!IsCompleted());
					Close();
					return true;
				}

				return false;
			}

			// prepares the task for execution and executes its body if its execution hasn't been started yet
			bool TryExecute()
			{
				if (TryGetExecutionPermission())
				{
					DoExecute();
					return true;
				}

				return false; // that task is being retracted in parallel
			}

			bool TryGetExecutionPermission()
			{
				return bAvailableForExecution.exchange(false, std::memory_order_acq_rel);
			}

			void RevokeExecutionPermission()
			{
				checkSlow(!bAvailableForExecution.load(std::memory_order_relaxed));
				bAvailableForExecution.store(true, std::memory_order_release);
			}

			void DoExecute()
			{
				AddRef(); // for the reference hold by nested tasks, is released when the task is closed

				// release prerequisites refs as they are not needed anymore
				while (TOptional<FTaskBase*> Prerequisite = Prerequisites.Dequeue())
				{
					Prerequisite.GetValue()->Release();
				}

				checkSlow(Pipe == nullptr ? NumLocks.load(std::memory_order_relaxed) == 1 : NumLocks.load(std::memory_order_relaxed) == 0);
				NumLocks.store(ExecutionFlag + 1, std::memory_order_relaxed); // +1 to hold it locked during execution, so nested tasks don't
				// complete it before the execution finishes

				FTaskBase* PrevTask = ExchangeCurrentTask(this);
				{
					TaskTrace::FTaskTimingEventScope TaskEventScope(GetTraceId());
					StartPipeExecution();

					TaskBody();
					TaskBody.Reset();

					FinishPipeExecution();
				}
				ExchangeCurrentTask(PrevTask);
				
				uint32 LocalNumLocks = NumLocks.fetch_sub(1, std::memory_order_relaxed) - 1;

				TryComplete(LocalNumLocks);
			}

			// checks if the task is ready to be launched by trying to push it into the pipe.
			// can be called up to two times: first to push into the blocked pipe and then when the pipe is unblocked
			// `LocalNumLocks` is the value that was used to make a decision to push into the pipe, no need to read `NumLocks` again
			bool TryPushIntoPipe(uint32 LocalNumLocks)
			{
				if (Pipe == nullptr)
				{
					// the task is locked for a pipe initially even if eventually there's no pipe
					check(LocalNumLocks == 1);

					return true;
				}

				// on the first call we try to push the task into the pipe. if unsuccessful (the pipe is blocked), the second time the method is called
				// only when the pipe is unblocked, so we know that the task is free to be executed

				bool bFirstAttempt = LocalNumLocks == 1;
				if (bFirstAttempt)
				{
					FTaskBase* PrevPipedTask = PushIntoPipe();
					if (PrevPipedTask == nullptr) // we are free to go
					{
						NumLocks.store(0, std::memory_order_relaxed);
						return true;
					}

					Prerequisites.Enqueue(PrevPipedTask);
					// no need to AddRef as it's already sorted in `FPipe::PushIntoPipe`
					return false;
				}

				return true;
			}

			// is called when the task has no pending prerequisites. Returns the previous piped task if any
			FTaskBase* PushIntoPipe();

			void StartPipeExecution();
			void FinishPipeExecution();

			// closes task by unlocking its subsequents and flagging it as completed
			void Close();

		private:
			LowLevelTasks::FTask LowLevelTask;
			TUniqueFunction<void()> TaskBody;
			std::atomic<bool> bAvailableForExecution{ true };


			// the number of times that the task should be unlocked before it can be scheduled or completed
			// initial count is 1 for launching the task (it can't be scheduled before it's launched) and 1 for a potential blocked pipe. once NumLocks
			// reaches 0 the task is scheduled for execution.
			// NumLocks's the most significant bit (see `ExecutionFlag`) is set on task execution start, and indicates that now NumLocks is about 
			// how many times the task must be unlocked to be completed
			static constexpr uint32 NumInitialLocks = 1 + 1;
			std::atomic<uint32> NumLocks{ NumInitialLocks };

			// A single-producer/single-consumer container to store back links to "prerequsites" (either execution prerequisites or nested tasks 
			// that are completion prerequisites).
			// It's populated in three stages:
			// 1) by adding execution prerequisites, before the task is launched, single thread, when nobody else accesses it. doesn't need
			// synchronisation
			// 2) by piping, when the previous piped task is added as a prerequisite. after adding prerequisites. this can happen in parallel with
			// retraction that consumes prerequisites. multiple threads can try to retract the task but only one will be allowed to proceed, 
			// so single producer/single consumer
			// 3) by adding nested tasks. after piping. during task execution, thus retraction is not possible and won't touch it. single-threaded
			// `Prerequisites` can be consumed concurrently by retraction and nested tasks closing this task. This case is explicitly locked
			TSpscQueue<FTaskBase*> Prerequisites;
			FSpinLock PrerequisitesLock;

			TClosableMpscQueue<FTaskBase*> Subsequents;

			FPipe* Pipe{ nullptr };

#if UE_TASK_TRACE_ENABLED
			TaskTrace::FId TraceId = TaskTrace::GenerateTaskId();
#endif

		// some projects are still built on macOS before v.10.14 that doesn't have aligned new/delete operators
		public:
			inline void* operator new(size_t Size)
			{
				return FMemory::Malloc(Size, 64u);
			}

			inline void operator delete(void* Ptr)
			{
				FMemory::Free(Ptr);
			}
		};

		template<typename TaskCollectionType>
		bool TryRetractAndExecute(const TaskCollectionType& Tasks, FTimespan InTimeout = FTimespan::MaxValue())
		{
			FTimeout Timeout{ InTimeout };
			bool bResult = true;

			for (auto& Task : Tasks)
			{
				if (Task.IsValid() && !Task.Pimpl->TryRetractAndExecute())
				{
					bResult = false;
				}

				if (Timeout)
				{
					return false;
				}
			}

			return bResult;
		}

		// Extends FTaskBase by supporting execution result.
		template<typename ResultType>
		class TTaskWithResult : public FTaskBase
		{
			UE_NONCOPYABLE(TTaskWithResult);

		public:
			TTaskWithResult() = default;

			virtual ~TTaskWithResult() override
			{
				checkf(IsCompleted(), TEXT("Every task instance must be completed before it's destroyed"));
				DestructItem(ResultStorage.GetTypedPtr());
			}

			template<typename TaskBodyType>
			void Init(const TCHAR* DebugName, TaskBodyType&& TaskBody, LowLevelTasks::ETaskPriority Priority)
			{
				FTaskBase::Init(
					DebugName,
					[this, TaskBody = Forward<TaskBodyType>(TaskBody)]() mutable
					{
						new(&ResultStorage) ResultType{ Invoke(TaskBody) };
					},
					Priority
				);
			}

			bool TryLaunch()
			{
				return FTaskBase::TryLaunch();
			}

			ResultType& GetResult()
			{
				checkf(IsCompleted(), TEXT("The task must be completed to obtain its result"));
				return *ResultStorage.GetTypedPtr();
			}

		private:
			TTypeCompatibleBytes<ResultType> ResultStorage;
		};

		template<>
		class TTaskWithResult<void> : public FTaskBase
		{
		public:
			TTaskWithResult() = default; // instances can be created only on the heap by a ref-counting handler

			void GetResult()
			{
				checkf(IsCompleted(), TEXT("The task must be completed to obtain its result"));
			}
		};
	}
}}