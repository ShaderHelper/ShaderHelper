// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	UnixPlatformProcess.h: Unix platform Process functions
==============================================================================================*/

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Containers/UnrealString.h"
#include "GenericPlatform/GenericPlatformProcess.h"

class Error;

/** Wrapper around Unix pid_t. Should not be copyable as changes in the process state won't be properly propagated to all copies. */
struct FProcState
{
	/** Default constructor. */
	FORCEINLINE FProcState()
		:	ProcessId(0)
		,	bIsRunning(false)
		,	bHasBeenWaitedFor(false)
		,	ReturnCode(-1)
		,	bFireAndForget(false)
	{
	}

	/** Initialization constructor. */
	explicit FProcState(pid_t InProcessId, bool bInFireAndForget);

	/** Destructor. */
	~FProcState();

	/** Getter for process id */
	FORCEINLINE pid_t GetProcessId() const
	{
		return ProcessId;
	}

	/**
	 * Returns whether this process is running.
	 *
	 * @return true if running
	 */
	bool	IsRunning();

	/**
	 * Returns child's return code (only valid to call if not running)
	 *
	 * @param ReturnCode set to return code if not NULL (use the value only if method returned true)
	 *
	 * @return return whether we have the return code (we don't if it crashed)
	 */
	bool	GetReturnCode(int32* ReturnCodePtr);

	/**
	 * Waits for the process to end.
	 * Has a side effect (stores child's return code).
	 */
	void	Wait();

protected:  // the below is not a public API!

	// FProcState should not be copyable since it breeds problems (e.g. one copy could have wait()ed, but another would not know it)

	/** Copy constructor - should not be publicly accessible */
	FORCEINLINE FProcState(const FProcState& Other)
		:	ProcessId(Other.ProcessId)
		,	bIsRunning(Other.bIsRunning)  // assume it is
		,	bHasBeenWaitedFor(Other.bHasBeenWaitedFor)
		,	ReturnCode(Other.ReturnCode)
		,	bFireAndForget(Other.bFireAndForget)
	{
		checkf(false, TEXT("FProcState should not be copied"));
	}

	/** Assignment operator - should not be publicly accessible */
	FORCEINLINE FProcState& operator=(const FProcState& Other)
	{
		checkf(false, TEXT("FProcState should not be copied"));
		return *this;
	}

	friend struct FUnixPlatformProcess;

	// -------------------------

	/** Process id */
	pid_t	ProcessId;

	/** Whether the process has finished or not (cached) */
	bool	bIsRunning;

	/** Whether the process's return code has been collected */
	bool	bHasBeenWaitedFor;

	/** Return code of the process (if negative, means that process did not finish gracefully but was killed/crashed*/
	int32	ReturnCode;

	/** Whether this child is fire-and-forget */
	bool	bFireAndForget;
};

/** FProcHandle can be copied (and thus passed by value). */
struct FProcHandle
{
	/** Child proc state set from CreateProc() call */
	FProcState* 		ProcInfo;

	/** Pid of external process opened with OpenProcess() call.
	  * Added to FProcHandle so we don't have to special case FProcState with process
	  * we can only check for running state, and even then the PID could be reused so
	  * we don't ever want to terminate, etc.
	  */
	pid_t				OpenedPid;

	FProcHandle()
	:	ProcInfo(nullptr), OpenedPid(-1)
	{
	}

	FProcHandle(FProcState* InHandle)
	:	ProcInfo(InHandle), OpenedPid(-1)
	{
	}

	FProcHandle(pid_t InProcPid)
	:	ProcInfo(nullptr), OpenedPid(InProcPid)
	{
	}

	/** Accessors. */
	FORCEINLINE pid_t Get() const
	{
		return ProcInfo ? ProcInfo->GetProcessId() : OpenedPid;
	}

	/** Resets the handle to invalid */
	FORCEINLINE void Reset()
	{
		ProcInfo = nullptr;
		OpenedPid = -1;
	}

	/** Checks the validity of handle */
	FORCEINLINE bool IsValid() const
	{
		return ProcInfo != nullptr || OpenedPid != -1;
	}

	// the below is not part of FProcHandle API and is specific to Unix implementation
	FORCEINLINE FProcState* GetProcessInfo() const
	{
		return ProcInfo;
	}
};

/** Wrapper around Unix file descriptors */
struct FPipeHandle
{
	FPipeHandle(int Fd)
		:	PipeDesc(Fd)
	{
	}

	~FPipeHandle();

	/**
	 * Reads until EOF.
	 */
	FString Read();

	/**
	 * Reads until EOF.
	 */
	bool ReadToArray(TArray<uint8> & Output);

	/**
	 * Returns raw file handle.
	 */
	int GetHandle() const
	{
		return PipeDesc;
	}

protected:

	int	PipeDesc;
};

/**
 * Unix implementation of the Process OS functions
 */
struct CORE_API FUnixPlatformProcess : public FGenericPlatformProcess
{
	struct FProcEnumInfo;

	/**
	 * Process enumerator.
	 */
	class CORE_API FProcEnumerator
	{
	public:
		// Constructor
		FProcEnumerator();
		FProcEnumerator(const FProcEnumerator&) = delete;
		FProcEnumerator& operator=(const FProcEnumerator&) = delete;

		// Destructor
		~FProcEnumerator();

		// Gets current process enumerator info.
		FProcEnumInfo GetCurrent() const;
		
		/**
		 * Moves current to the next process.
		 *
		 * @returns True if succeeded. False otherwise.
		 */
		bool MoveNext();
	private:
		// Private implementation data.
		struct FProcEnumData* Data;
	};

	/**
	 * Process enumeration info structure.
	 */
	struct CORE_API FProcEnumInfo
	{
		friend FUnixPlatformProcess::FProcEnumerator::FProcEnumerator();

		// Gets process PID.
		uint32 GetPID() const;

		// Gets parent process PID.
		uint32 GetParentPID() const;

		// Gets process name. I.e. exec name.
		FString GetName() const;

		// Gets process full image path. I.e. full path of the exec file.
		FString GetFullPath() const;

	private:
		// Private constructor.
		FProcEnumInfo(uint32 InPID);

		// Current process PID.
		uint32 PID;
	};

	static void* GetDllHandle( const TCHAR* Filename );
	static void FreeDllHandle( void* DllHandle );
	static void* GetDllExport( void* DllHandle, const TCHAR* ProcName );
	static const TCHAR* ComputerName();
	static const TCHAR* UserName(bool bOnlyAlphaNumeric = true);
	static const TCHAR* UserTempDir();
	static const TCHAR* UserDir();
	static const TCHAR* UserSettingsDir();
	static const TCHAR* ApplicationSettingsDir();
	static void SetCurrentWorkingDirectoryToBaseDir();
	static FString GetCurrentWorkingDirectory();
	static FString GenerateApplicationPath(const FString& AppName, EBuildConfiguration BuildConfiguration);
	static FString GetApplicationName( uint32 ProcessId );
	static bool SetProcessLimits(EProcessResource::Type Resource, uint64 Limit);
	static const TCHAR* ExecutablePath();
	static const TCHAR* ExecutableName(bool bRemoveExtension = true);
	static const TCHAR* GetModulePrefix();
	static const TCHAR* GetModuleExtension();
	static void ClosePipe( void* ReadPipe, void* WritePipe );
	static bool CreatePipe(void*& ReadPipe, void*& WritePipe, bool bWritePipeLocal = false);
	static FString ReadPipe( void* ReadPipe );
	static bool ReadPipeToArray(void* ReadPipe, TArray<uint8> & Output);
	static bool WritePipe(void* WritePipe, const FString& Message, FString* OutWritten = nullptr);
	static bool WritePipe(void* WritePipe, const uint8* Data, const int32 DataLength, int32* OutDataLength = nullptr);
	static class FRunnableThread* CreateRunnableThread();
	static const FString GetModulesDirectory();
	static bool CanLaunchURL(const TCHAR* URL);
	static void LaunchURL(const TCHAR* URL, const TCHAR* Parms, FString* Error);
	static FProcHandle CreateProc(const TCHAR* URL, const TCHAR* Parms, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden, uint32* OutProcessID, int32 PriorityModifier, const TCHAR* OptionalWorkingDirectory, void* PipeWriteChild, void* PipeReadChild = nullptr);
	static FProcHandle CreateProc(const TCHAR* URL, const TCHAR* Parms, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden, uint32* OutProcessID, int32 PriorityModifier, const TCHAR* OptionalWorkingDirectory, void* PipeWriteChild, void* PipeReadChild , void* PipeStdErrChild);
	static FProcHandle OpenProcess(uint32 ProcessID);
	static bool IsProcRunning( FProcHandle & ProcessHandle );
	static void WaitForProc( FProcHandle & ProcessHandle );
	static void CloseProc( FProcHandle & ProcessHandle );
	static void TerminateProc( FProcHandle & ProcessHandle, bool KillTree = false );
	static EWaitAndForkResult WaitAndFork();
	static uint32 GetCurrentProcessId();
	static uint32 GetCurrentCoreNumber();
	static bool GetProcReturnCode( FProcHandle & ProcHandle, int32* ReturnCode );
	static bool Daemonize();
	static bool IsApplicationRunning( uint32 ProcessId );
	static bool IsApplicationRunning( const TCHAR* ProcName );
	static bool ExecProcess(const TCHAR* URL, const TCHAR* Params, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr, const TCHAR* OptionalWorkingDirectory = NULL, bool bShouldEndWithParentProcess = false);
	static void ExploreFolder( const TCHAR* FilePath );
	static bool LaunchFileInDefaultExternalApplication( const TCHAR* FileName, const TCHAR* Parms = NULL, ELaunchVerb::Type Verb = ELaunchVerb::Open, bool bPromptToOpenOnFailure = true);
	static bool IsFirstInstance();

	/**
	 * @brief Releases locks that we held for IsFirstInstance check
	 */
	static void CeaseBeingFirstInstance();

	/**
	 * @brief Returns user home directory (i.e. $HOME).
	 *
	 * Like other directory functions, cannot return nullptr!
	 */
	static const TCHAR* UserHomeDir();
};
