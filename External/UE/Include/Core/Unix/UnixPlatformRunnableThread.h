// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	UnixRunnableThread.h: Unix platform threading functions
==============================================================================================*/

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/UnrealString.h"
#include "Containers/StringConv.h"
#include "Logging/LogMacros.h"
#include "Runtime/Core/Private/HAL/PThreadRunnableThread.h"
#include <sys/resource.h>

class Error;

/**
* Unix implementation of the Process OS functions
**/
class FRunnableThreadUnix : public FRunnableThreadPThread
{
	struct EConstants
	{
		enum
		{
			UnixThreadNameLimit = 15,			// the limit for thread name is just 15 chars :( http://man7.org/linux/man-pages/man3/pthread_setname_np.3.html

			CrashHandlerStackSize = SIGSTKSZ + 192 * 1024,	// should be at least SIGSTKSIZE, plus 192K because we do logging and symbolication in crash handler
			CrashHandlerStackSizeMin = SIGSTKSZ + 8 * 1024  // minimum allowed stack size
		};
	};

	/** Each thread needs a separate stack for the signal handler, so possible stack overflows in the thread are handled */
	void* ThreadCrashHandlingStack;

	/** Address of stack guard page - if nullptr, the page wasn't set */
	void* StackGuardPageAddress;

	/** Baseline priority (nice value). See explanation in SetPriority(). */
	int BaselineNiceValue;

	/** Whether the value of BaselineNiceValue has been obtained through getpriority(). See explanation in SetPriority(). */
	bool bGotBaselineNiceValue;

public:

	/** Separate stack for the signal handler (so possible stack overflows don't go unnoticed), for the main thread specifically. */
	static void *MainThreadSignalHandlerStack;

	static void *AllocCrashHandlerStack();
	static void FreeCrashHandlerStack(void *StackBuffer);
	static uint64 GetCrashHandlerStackSize();

	FRunnableThreadUnix()
		:	FRunnableThreadPThread()
		,	ThreadCrashHandlingStack(nullptr)
		,	StackGuardPageAddress(nullptr)
		,	BaselineNiceValue(0)
		,	bGotBaselineNiceValue(false)
	{
	}

	~FRunnableThreadUnix()
	{
		// Call the parent destructor body before the parent does it - see comment on that function for explanation why.
		FRunnableThreadPThread_DestructorBody();
	}

	/**
	 * Sets up an alt stack for signal (including crash) handling on this thread.
	 *
	 * This includes guard page at the end of the stack to make running out of stack more obvious.
	 * Should be run in the context of the thread.
	 *
	 * @param StackBuffer pointer to the beginning of the stack buffer (note: on x86_64 will be the bottom of the stack, not its beginning)
	 * @param StackSize size of the stack buffer
	 * @param OutStackGuardPageAddress pointer to the variable that will receive the address of the guard page. Can be null. Will not be set if guard page wasn't successfully set.
	 *
	 * @return true if setting the alt stack succeeded. Inability to set guard page will not affect success of the operation.
	 */
	static bool SetupSignalHandlerStack(void* StackBuffer, const size_t StackBufferSize, void** OutStackGuardPageAddress)
	{
		if (!StackBuffer)
		{
			return false;
		}

		// Added by CL 11188846 for ASan, TSan, and UBSan
		// #jira UE-62784 UE-62803 UE-62804
		if (FORCE_ANSI_ALLOCATOR)
		{
			return true;
		}

		// find an address close to begin of the stack and protect it
		uint64 StackGuardPage = reinterpret_cast<uint64>(StackBuffer);

		// align by page
		const uint64 PageSize = sysconf(_SC_PAGESIZE);
		const uint64 Remainder = StackGuardPage % PageSize;
		if (Remainder != 0)
		{
			StackGuardPage += (PageSize - Remainder);
			checkf(StackGuardPage % PageSize == 0, TEXT("StackGuardPage is not aligned on page size"));
		}

		checkf(StackGuardPage + PageSize - reinterpret_cast<uint64>(StackBuffer) < StackBufferSize,
			TEXT("Stack size is too small for the extra guard page!"));

		void* StackGuardPageAddr = reinterpret_cast<void*>(StackGuardPage);
		if (FPlatformMemory::PageProtect(StackGuardPageAddr, PageSize, true, false))
		{
			if (OutStackGuardPageAddress)
			{
				*OutStackGuardPageAddress = StackGuardPageAddr;
			}
		}
		else
		{
			// cannot use UE_LOG - can run into deadlocks in output device code
			fprintf(stderr, "Unable to set a guard page on the alt stack\n");
		}

		// set up the buffer to be used as stack
		stack_t SignalHandlerStack;
		FMemory::Memzero(SignalHandlerStack);
		SignalHandlerStack.ss_sp = StackBuffer;
		SignalHandlerStack.ss_size = StackBufferSize;

		bool bSuccess = (sigaltstack(&SignalHandlerStack, nullptr) == 0);
		if (!bSuccess)
		{
			int ErrNo = errno;
			// cannot use UE_LOG - can run into deadlocks in output device code
			fprintf(stderr, "Unable to set alternate stack for crash handler, sigaltstack() failed with errno=%d (%s)\n", ErrNo, strerror(ErrNo));
		}

		return bSuccess;
	}

protected:

	/** on Unix, this translates to ranges of setpriority(). Note that not all range may be available*/
	virtual int32 TranslateThreadPriority(EThreadPriority Priority)
	{
		// In general, the range is -20 to 19 (negative is highest, positive is lowest)
		int32 NiceLevel = 0;
		switch (Priority)
		{
			case TPri_TimeCritical:
				NiceLevel = -20;
				break;

			case TPri_Highest:
				NiceLevel = -15;
				break;

			case TPri_AboveNormal:
				NiceLevel = -10;
				break;

			case TPri_Normal:
				NiceLevel = 0;
				break;

			case TPri_SlightlyBelowNormal:
				NiceLevel = 3;
				break;

			case TPri_BelowNormal:
				NiceLevel = 5;
				break;

			case TPri_Lowest:
				NiceLevel = 10;		// 19 is a total starvation
				break;

			default:
				UE_LOG(LogHAL, Fatal, TEXT("Unknown Priority passed to FRunnableThreadPThread::TranslateThreadPriority()"));
				return 0;
		}

		// note: a non-privileged process can only go as low as RLIMIT_NICE
		return NiceLevel;
	}

	virtual void SetThreadPriority(EThreadPriority NewPriority) override
	{
		// always set priority to avoid second guessing
		ThreadPriority = NewPriority;
		SetThreadPriority(Thread, NewPriority);
	}

	virtual void SetThreadPriority(pthread_t InThread, EThreadPriority NewPriority)
	{
		// NOTE: InThread is ignored, but we can use ThreadID that maps to SYS_ttid
		int32 Prio = TranslateThreadPriority(NewPriority);

		// Unix implements thread priorities the same way as process priorities, while on Windows they are relative to process priority.
		// We want Windows behavior, since sometimes we set the whole process to a lower priority and would like its threads to avoid raising it
		// (even if RTLIMIT_NICE allows it) - example is ShaderCompileWorker that need to run in the background.
		//
		// Thusly we remember the baseline value that the process has at the moment of first priority change and set thread priority relative to it.
		// This is of course subject to race conditions and other problems (e.g. in case main thread changes its priority after the fact), but it's the best we have.

		if (!bGotBaselineNiceValue)
		{
			// checking errno is necessary since -1 is a valid priority to return from getpriority()
			errno = 0;
			int CurrentPriority = getpriority(PRIO_PROCESS, getpid());
			// if getting priority wasn't successful, don't change the baseline value (will be 0 - i.e. normal - by default)
			if (CurrentPriority != -1 || errno == 0)
			{
				BaselineNiceValue = CurrentPriority;
				bGotBaselineNiceValue = true;
			}
		}

		int ModifiedPriority = FMath::Clamp(BaselineNiceValue + Prio, -20, 19);
		if (setpriority(PRIO_PROCESS, ThreadID, ModifiedPriority) != 0)
		{
			// Unfortunately this is going to be a frequent occurence given that by default Unix doesn't allow raising priorities.
			// Don't issue a warning here
		}
	}

private:

	/**
	 * Allows a platform subclass to setup anything needed on the thread before running the Run function
	 */
	virtual void PreRun() override
	{
		FString SizeLimitedThreadName = ThreadName;

		if (SizeLimitedThreadName.Len() > EConstants::UnixThreadNameLimit)
		{
			// first, attempt to cut out common and meaningless substrings
			SizeLimitedThreadName = SizeLimitedThreadName.Replace(TEXT("Thread"), TEXT(""));
			SizeLimitedThreadName = SizeLimitedThreadName.Replace(TEXT("Runnable"), TEXT(""));

			// if still larger
			if (SizeLimitedThreadName.Len() > EConstants::UnixThreadNameLimit)
			{
				FString Temp = SizeLimitedThreadName;

				// cut out the middle and replace with a substitute
				const TCHAR Dash[] = TEXT("-");
				const int32 DashLen = UE_ARRAY_COUNT(Dash) - 1;
				int NumToLeave = (EConstants::UnixThreadNameLimit - DashLen) / 2;

				SizeLimitedThreadName = Temp.Left(EConstants::UnixThreadNameLimit - (NumToLeave + DashLen));
				SizeLimitedThreadName += Dash;
				SizeLimitedThreadName += Temp.Right(NumToLeave);

				check(SizeLimitedThreadName.Len() <= EConstants::UnixThreadNameLimit);
			}
		}

		int ErrCode = pthread_setname_np(Thread, TCHAR_TO_ANSI(*SizeLimitedThreadName));
		if (ErrCode != 0)
		{
			UE_LOG(LogHAL, Warning, TEXT("pthread_setname_np(, '%s') failed with error %d (%s)."), *ThreadName, ErrCode, ANSI_TO_TCHAR(strerror(ErrCode)));
		}

		// set the alternate stack for handling crashes due to stack overflow
		check(ThreadCrashHandlingStack == nullptr);
		ThreadCrashHandlingStack = AllocCrashHandlerStack();
		SetupSignalHandlerStack(ThreadCrashHandlingStack, FRunnableThreadUnix::GetCrashHandlerStackSize(), &StackGuardPageAddress);
	}

	virtual void PostRun() override
	{
		if (StackGuardPageAddress != nullptr)
		{
			// we protected one page only
			const uint64 PageSize = sysconf(_SC_PAGESIZE);

			if (!FPlatformMemory::PageProtect(StackGuardPageAddress, PageSize, true, true))
			{
				UE_LOG(LogCore, Error, TEXT("Unable to remove a guard page from the alt stack"));
			}

			StackGuardPageAddress = nullptr;
		}

		if (ThreadCrashHandlingStack != nullptr)
		{
			FreeCrashHandlerStack(ThreadCrashHandlingStack);
			ThreadCrashHandlingStack = nullptr;
		}
	}

	/**
	 * Allows platforms to adjust stack size
	 */
	virtual uint32 AdjustStackSize(uint32 InStackSize)
	{
		InStackSize = FRunnableThreadPThread::AdjustStackSize(InStackSize);

		// If it's set, make sure it's at least 128 KB or stack allocations (e.g. in Logf) may fail
		if (InStackSize && InStackSize < 128 * 1024)
		{
			InStackSize = 128 * 1024;
		}

		return InStackSize;
	}
};
