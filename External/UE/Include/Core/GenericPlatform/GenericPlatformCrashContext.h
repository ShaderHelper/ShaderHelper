// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformStackWalk.h"
#include "Misc/Optional.h"
#include "Containers/UnrealString.h"

#ifndef WITH_ADDITIONAL_CRASH_CONTEXTS
#define WITH_ADDITIONAL_CRASH_CONTEXTS 1
#endif

struct FProgramCounterSymbolInfo;

/** Defines special exit codes used to diagnose abnormal terminations. The code values are arbitrary, but easily recongnizable in decimal. They are meant to be
    used with the out-of-process monitoring/analytics in order to figure out unexpected cases. */
enum ECrashExitCodes : int32
{
	/** Used by out-of-process monitor in analytics report, the application is still running, but out-of-process monitor was requested to exit before the application exit code could be read. */
	MonitoredApplicationStillRunning = 777001,

	/** Used by out-of-process monitor in analytics report, the application is not running anymore, but the out-of-process monitor could not read the Editor exit code (either is is not supported by the OS or is not available). */
	MonitoredApplicationExitCodeNotAvailable = 777002,

	/** Used by the application when the crash reporter crashed itself while reporting a crash.*/
	CrashReporterCrashed = 777003,

	/** Used by the application when the crash handler crashed itself (crash in the __except() clause for example).*/
	CrashHandlerCrashed = 777004,

	/** Used by the application to flag when it detects that its out-of-process application supposed to report the bugs died (ex if the Editor detects that CrashReportClientEditor is not running anymore as expected).*/
	OutOfProcessReporterExitedUnexpectedly = 777005,

	/** Application crashed during static initialization. It may or may not have been able to have sent a crash report. */
	CrashDuringStaticInit = 777006,

	/** Used as MonitorExceptCode in analytics to track how often the out-of-process CRC exits because of a failed check. */
	OutOfProcessReporterCheckFailed = 777007,

	/** The exception code used for ensure, in case a kernel driver callback happens at in a dispatch level where SEH (on windows) is disabled. */
	UnhandledEnsure = 777008,
};

/** Enumerates crash description versions. */
enum class ECrashDescVersions : int32
{
	/** Introduces a new crash description format. */
	VER_1_NewCrashFormat,

	/** Added misc properties (CPU,GPU,OS,etc), memory related stats and platform specific properties as generic payload. */
	VER_2_AddedNewProperties,

	/** Using crash context when available. */
	VER_3_CrashContext = 3,
};

/** Enumerates crash dump modes. */
enum class ECrashDumpMode : int32
{
	/** Default minidump settings. */
	Default = 0,

	/** Full memory crash minidump */
	FullDump = 1,

	/** Full memory crash minidump, even on ensures */
	FullDumpAlways = 2,
};

/** Portable stack frame */
struct FCrashStackFrame
{
	FString ModuleName;
	uint64 BaseAddress;
	uint64 Offset;

	FCrashStackFrame(FString ModuleNameIn, uint64 BaseAddressIn, uint64 OffsetIn)
		: ModuleName(MoveTemp(ModuleNameIn))
		, BaseAddress(BaseAddressIn)
		, Offset(OffsetIn)
	{
	}
};

/** Portable thread stack frame */
struct FThreadStackFrames {
	FString						ThreadName;
	uint32						ThreadId;
	TArray<FCrashStackFrame>	StackFrames;
};

enum class ECrashContextType
{
	Crash,
	Assert,
	Ensure,
	Stall,
	GPUCrash,
	Hang,
	OutOfMemory,
	AbnormalShutdown,

	Max
};

/** In development mode we can cause crashes in order to test reporting systems. */
enum class ECrashTrigger
{
	Debug = -1,
	Normal = 0
};

#define CR_MAX_ERROR_MESSAGE_CHARS 2048
#define CR_MAX_DIRECTORY_CHARS 256
#define CR_MAX_STACK_FRAMES 256
#define CR_MAX_THREAD_NAME_CHARS 64
#define CR_MAX_THREADS 512
#define CR_MAX_GENERIC_FIELD_CHARS 64
#define CR_MAX_COMMANDLINE_CHARS 1024
#define CR_MAX_RICHTEXT_FIELD_CHARS 512
#define CR_MAX_DYNAMIC_BUFFER_CHARS 1024*32

/**
 * Fixed size structure that holds session specific state.
 */
struct FSessionContext 
{
	bool 					bIsInternalBuild;
	bool 					bIsPerforceBuild;
	bool 					bIsSourceDistribution;
	bool 					bIsUERelease;
	bool					bIsOOM;
	bool					bIsExitRequested;
	uint32					ProcessId;
	int32 					LanguageLCID;
	int32 					NumberOfCores;
	int32 					NumberOfCoresIncludingHyperthreads;
	int32 					SecondsSinceStart;
	int32 					CrashDumpMode;
	int32					CrashType;
	int32					OOMAllocationAlignment;
	uint64					OOMAllocationSize;
	TCHAR 					GameName[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR					EngineMode[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR					EngineModeEx[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					ExecutableName[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR					BuildConfigurationName[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					DeploymentName[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					BaseDir[CR_MAX_DIRECTORY_CHARS];
	TCHAR 					RootDir[CR_MAX_DIRECTORY_CHARS];
	TCHAR 					EpicAccountId[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					LoginIdStr[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR					SymbolsLabel[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					OsVersion[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					OsSubVersion[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					CPUVendor[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					CPUBrand[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					PrimaryGPUBrand[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					UserName[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					DefaultLocale[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					CrashGUIDRoot[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					UserActivityHint[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					GameSessionID[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					CommandLine[CR_MAX_COMMANDLINE_CHARS];
	TCHAR 					CrashReportClientRichText[CR_MAX_RICHTEXT_FIELD_CHARS];
	TCHAR 					GameStateName[CR_MAX_GENERIC_FIELD_CHARS];
	TCHAR 					CrashConfigFilePath[CR_MAX_DIRECTORY_CHARS];
	char					PlatformName[CR_MAX_GENERIC_FIELD_CHARS];
	char					PlatformNameIni[CR_MAX_GENERIC_FIELD_CHARS];
	FPlatformMemoryStats	MemoryStats;
};

/** Additional user settings to be communicated to crash reporting client. */
struct FUserSettingsContext
{
	bool					bNoDialog = false;
	bool					bSendUnattendedBugReports = false;
	bool					bSendUsageData = false;
	bool					bImplicitSend = false;
	TCHAR					LogFilePath[CR_MAX_DIRECTORY_CHARS];
};

/**
 * Fixed size struct holds crash information and session specific state. It is designed
 * to shared between processes (e.g. Game and CrashReporterClient).
 */
struct FSharedCrashContext
{
	// Exception info
	TCHAR					ErrorMessage[CR_MAX_ERROR_MESSAGE_CHARS];
	uint32					ThreadIds[CR_MAX_THREADS];
	TCHAR					ThreadNames[CR_MAX_THREAD_NAME_CHARS * CR_MAX_THREADS];
	uint32					NumThreads;
	uint32					CrashingThreadId;
	ECrashContextType		CrashType;

	// Additional user settings.
	FUserSettingsContext	UserSettings;

	// Platform specific crash context (must be portable)
	void*					PlatformCrashContext;
	// Directory for dumped files
	TCHAR					CrashFilesDirectory[CR_MAX_DIRECTORY_CHARS];
	// Game/Engine information not possible to catch out of process
	FSessionContext			SessionContext;
	// Count and offset into dynamic buffer to comma separated plugin list
	uint32					EnabledPluginsNum;
	uint32					EnabledPluginsOffset;
	// Count and offset into dynamic buffer to comma separated key=value data for engine information
	uint32					EngineDataNum;
	uint32					EngineDataOffset;
	// Count and offset into dynamic buffer to comma separated key=value data for  game information
	uint32					GameDataNum;
	uint32					GameDataOffset;
	// Fixed size dynamic buffer
	TCHAR					DynamicData[CR_MAX_DYNAMIC_BUFFER_CHARS];

	// Program counter address where the error occurred.
	void*					ErrorProgramCounter;

	// Instruction address where the exception was raised that initiated crash reporting
	void*					ExceptionProgramCounter;
};

/**
 *	Contains a runtime crash's properties that are common for all platforms.
 *	This may change in the future.
 */
struct CORE_API FGenericCrashContext
{
public:

	static const ANSICHAR* const CrashContextRuntimeXMLNameA;
	static const TCHAR* const CrashContextRuntimeXMLNameW;

	static const ANSICHAR* const CrashConfigFileNameA;
	static const TCHAR* const CrashConfigFileNameW;
	static const TCHAR* const CrashConfigExtension;
	static const TCHAR* const ConfigSectionName;
	static const TCHAR* const CrashConfigPurgeDays;
	static const TCHAR* const CrashGUIDRootPrefix;

	static const TCHAR* const CrashContextExtension;
	static const TCHAR* const RuntimePropertiesTag;
	static const TCHAR* const PlatformPropertiesTag;
	static const TCHAR* const EngineDataTag;
	static const TCHAR* const GameDataTag;
	static const TCHAR* const EnabledPluginsTag;
	static const TCHAR* const UEMinidumpName;
	static const TCHAR* const NewLineTag;
	static constexpr int32 CrashGUIDLength = 128;

	static const TCHAR* const CrashTypeCrash;
	static const TCHAR* const CrashTypeAssert;
	static const TCHAR* const CrashTypeEnsure;
	static const TCHAR* const CrashTypeStall;
	static const TCHAR* const CrashTypeGPU;
	static const TCHAR* const CrashTypeHang;
	static const TCHAR* const CrashTypeAbnormalShutdown;

	static const TCHAR* const EngineModeExUnknown;
	static const TCHAR* const EngineModeExDirty;
	static const TCHAR* const EngineModeExVanilla;

	// A guid that identifies this particular execution. Allows multiple crash reports from the same run of the project to be tied together
	static const FGuid ExecutionGuid;

	/** Initializes crash context related platform specific data that can be impossible to obtain after a crash. */
	static void Initialize();

	/** Initialized crash context, using a crash context (e.g. shared from another process). */
	static void InitializeFromContext(const FSessionContext& Context, const TCHAR* EnabledPlugins, const TCHAR* EngineData, const TCHAR* GameData);

	/**
	 * @return true, if the generic crash context has been initialized.
	 */
	static bool IsInitalized()
	{
		return bIsInitialized;
	}

	/**
	 * @return true if walking the crashed call stack and writing the minidump is being handled out-of-process.
	 * @note The reporting itself (showing the crash UI and sending the report is always done out of process)
	 */
	static bool IsOutOfProcessCrashReporter()
	{
		return OutOfProcessCrashReporterPid != 0;
	}

	/**
	 * @return a non-zero value if crash reporter process is used to monitor the session, capture the call stack and write the minidump, otherwise, this is done inside the crashing process.
	 */
	static uint32 GetOutOfProcessCrashReporterProcessId()
	{
		return OutOfProcessCrashReporterPid;
	}

	/**
	 * Set whether or not the out-of-process crash reporter is running. A non-zero process id means that crash artifacts like the call stack and then minidump are
	 * built in a separated background process. The reporting itself, i.e. packaging and sending the crash artifacts is always done out of process.
	 * @note CrashReportClient (CrashReportClientEditor for the Editor) can be configured to wait for crash, capture the crashed process callstack, write the minidump, collect all crash artifacts
	 *       and send them (out-of-process reporting) or just collect and send them (in-process reporting because the crashing process creates all crash artifacts itself).
	 */
	static void SetOutOfProcessCrashReporterPid(uint32 ProcessId)
	{
		OutOfProcessCrashReporterPid = ProcessId;
	}

	/**
	 * Set the out of process crash reporter exit code if known. The out of process reporter is expected to run in background, waiting for a signal to handle a
	 * crashes/ensures/assert, but sometimes it crashes. If the engine detects that its associated out of process crash reporter died and if the child process exit
	 * code can be retrieved, it can be exposed through this function.
	 * @see GetOutOfProcessCrashReporterExitCode
	 */
	static void SetOutOfProcessCrashReporterExitCode(int32 ExitCode);

	/**
	 * Return the out-of-process crash reporter exit code if available. The exit code is available if crash reporter process died while the application it monitors was still running.
	 * Then engine periodically poll the health of the crash reporter process and try to read its exit code if it unexpectedly died.
	 * @note This function is useful to try diagnose why the crash reporter died (crashed/killed/asserted) and gather data for the analytics.
	 */
	static TOptional<int32> GetOutOfProcessCrashReporterExitCode();

	/** Default constructor. Optionally pass a process handle if building a crash context for a process other then current. */
	FGenericCrashContext(ECrashContextType InType, const TCHAR* ErrorMessage);

	virtual ~FGenericCrashContext() { }

	/** Get the file path to the temporary session context file that we create for the given process. */
	static FString GetTempSessionContextFilePath(uint64 ProcessID);

	/** Clean up expired context files that were left-over on the user disks (because the consumer crashed and/or failed to delete it). */
	static void CleanupTempSessionContextFiles(const FTimespan& ExpirationAge);

	/** Serializes all data to the buffer. */
	void SerializeContentToBuffer() const;

	/**
	 * @return the buffer containing serialized data.
	 */
	const FString& GetBuffer() const
	{
		return CommonBuffer;
	}

	/**
	 * @return a globally unique crash name.
	 */
	void GetUniqueCrashName(TCHAR* GUIDBuffer, int32 BufferSize) const;

	/**
	 * @return whether this crash is a full memory minidump
	 */
	const bool IsFullCrashDump() const;

	/** Serializes crash's informations to the specified filename. Should be overridden for platforms where using FFileHelper is not safe, all POSIX platforms. */
	virtual void SerializeAsXML( const TCHAR* Filename ) const;

	/** 
	 * Serializes session context to the given buffer. 
	 * NOTE: Assumes that the buffer already has a header and section open.
	 */
	static void SerializeSessionContext(FString& Buffer);
	
	template <typename Type>
	void AddCrashProperty(const TCHAR* PropertyName, const Type& Value) const
	{
		AddCrashPropertyInternal(CommonBuffer, PropertyName, Value);
	}

	/** Escapes and appends specified text to XML string */
	static void AppendEscapedXMLString( FString& OutBuffer, const TCHAR* Text );

	/** Unescapes a specified XML string, naive implementation. */
	static FString UnescapeXMLString( const FString& Text );

	/** Helper to get the standard string for the crash type based on crash event bool values. */
	static const TCHAR* GetCrashTypeString(ECrashContextType Type);

	/** Get the Game Name of the crash */
	static FString GetCrashGameName();

	/** Helper to get the crash report client config filepath saved by this instance and copied to each crash report folder. */
	static const TCHAR* GetCrashConfigFilePath();

	/** Helper to get the crash report client config folder used by GetCrashConfigFilePath(). */
	static const TCHAR* GetCrashConfigFolder();

	/** Helper to clean out old files in the crash report client config folder. */
	static void PurgeOldCrashConfig();

	/** Clears the engine data dictionary */
	static void ResetEngineData();

	/** Updates (or adds if not already present) arbitrary engine data to the crash context (will remove the key if passed an empty string) */
	static void SetEngineData(const FString& Key, const FString& Value);

	/** Clears the game data dictionary */
	static void ResetGameData();

	/** Updates (or adds if not already present) arbitrary game data to the crash context (will remove the key if passed an empty string) */
	static void SetGameData(const FString& Key, const FString& Value);

	/** Adds a plugin descriptor string to the enabled plugins list in the crash context */
	static void AddPlugin(const FString& PluginDesc);

	/** Flushes the logs. In the case of in memory logs is used on this configuration, dumps them to file. Returns the name of the file */
	static FString DumpLog(const FString& CrashFolderAbsolute);

	/** Collects additional crash context providers. See FAdditionalCrashContextStack. */
	static void DumpAdditionalContext(const TCHAR* CrashFolderAbsolute);

	/** Initializes a shared crash context from current state. Will not set all fields in Dst. */
	static void CopySharedCrashContext(FSharedCrashContext& Dst);

	/** We can't gather memory stats in crash handling function, so we gather them just before raising
	  * exception and use in crash reporting. 
	  */
	static void SetMemoryStats(const FPlatformMemoryStats& MemoryStats);

	/** Attempts to create the output report directory. */
	static bool CreateCrashReportDirectory(const TCHAR* CrashGUIDRoot, int32 CrashIndex, FString& OutCrashDirectoryAbsolute);

	/** Notify the crash context exit has been requested. */
	static void SetEngineExit(bool bIsRequestExit);

	/** Sets the process id to that has crashed. On supported platforms this will analyze the given process rather than current. Default is current process. */
	void SetCrashedProcess(const FProcHandle& Process) { ProcessHandle = Process; }

	/** Stores crashing thread id. */
	void SetCrashedThreadId(uint32 InId) { CrashedThreadId = InId; }

	/** Sets the number of stack frames to ignore when symbolicating from a minidump */
	void SetNumMinidumpFramesToIgnore(int32 InNumMinidumpFramesToIgnore);

	/**
	 * Generate raw call stack for crash report (image base + offset)
	 * @param ErrorProgramCounter The program counter of where the occur occurred in the callstack being captured
	 * @param Context Optional thread context information
	 */
	void CapturePortableCallStack(void* ErrorProgramCounter, void* Context);

	UE_DEPRECATED(5.0, "")
	void CapturePortableCallStack(int32 NumStackFramesToIgnore, void* Context);
	
	/** Sets the portable callstack to a specified stack */
	virtual void SetPortableCallStack(const uint64* StackFrames, int32 NumStackFrames);

	/** Gets the portable callstack to a specified stack and puts it into OutCallStack */
	virtual void GetPortableCallStack(const uint64* StackFrames, int32 NumStackFrames, TArray<FCrashStackFrame>& OutCallStack) const;

	/** Adds a portable callstack for a thread */
	virtual void AddPortableThreadCallStack(uint32 ThreadId, const TCHAR* ThreadName, const uint64* StackFrames, int32 NumStackFrames);

	/** Allows platform implementations to copy files to report directory. */
	virtual void CopyPlatformSpecificFiles(const TCHAR* OutputDirectory, void* Context);

	/** Cleanup platform specific files - called on startup, implemented per platform */
	static void CleanupPlatformSpecificFiles();

	/**
	 * @return the type of this crash
	 */
	ECrashContextType GetType() const { return Type; }

	/**
	 * @return whether a crash context type is continable
	 */
	static bool IsTypeContinuable(ECrashContextType Type)
	{
		switch (Type)
		{
		case ECrashContextType::Ensure:
			return true;
		case ECrashContextType::Stall:
			return true;
		default:
			return false;
		}
	}

	/**
	 * Set the current deployment name (ie. EpicApp)
	 */
	static void SetDeploymentName(const FString& EpicApp);

	/**
	 * Sets the type of crash triggered. Used to distinguish crashes caused for debugging purposes.
	 */
	static void SetCrashTrigger(ECrashTrigger Type);

protected:
	/**
	 * @OutStr - a stream of Thread XML elements containing info (e.g. callstack) specific to an active thread
	 * @return - whether the operation was successful
	 */
	virtual bool GetPlatformAllThreadContextsString(FString& OutStr) const { return false; }

	FProcHandle ProcessHandle;
	ECrashContextType Type;
	uint32 CrashedThreadId;
	const TCHAR* ErrorMessage;
	int NumMinidumpFramesToIgnore;
	TArray<FCrashStackFrame> CallStack;
	TArray<FThreadStackFrames> ThreadCallStacks;

	/** Allow platform implementations to provide a callstack property. Primarily used when non-native code triggers a crash. */
	virtual const TCHAR* GetCallstackProperty() const;

	/** Get arbitrary engine data from the crash context */
	static const FString* GetEngineData(const FString& Key);

	/** Get arbitrary game data from the crash context */
	static const FString* GetGameData(const FString& Key);

private:

	/** Serializes the session context section of the crash context to a temporary file. */
	static void SerializeTempCrashContextToFile();

	/** Serializes the current user setting struct to a buffer. */
	static void SerializeUserSettings(FString& Buffer);

	/** Writes a common property to the buffer. */
	static void AddCrashPropertyInternal(FString& Buffer, const TCHAR* PropertyName, const TCHAR* PropertyValue);

	/** Writes a common property to the buffer. */
	template <typename Type>
	static void AddCrashPropertyInternal(FString& Buffer, const TCHAR* PropertyName, const Type& Value)
	{
		AddCrashPropertyInternal(Buffer, PropertyName, *TTypeToString<Type>::ToString(Value));
	}

	/** Serializes platform specific properties to the buffer. */
	virtual void AddPlatformSpecificProperties() const;

	/** Add callstack information to the crash report xml */
	void AddPortableCallStack() const;

	/** Produces a hash based on the offsets of the portable callstack and adds it to the xml */
	void AddPortableCallStackHash() const;

	/** Writes header information to the buffer. */
	static void AddHeader(FString& Buffer);

	/** Writes footer to the buffer. */
	static void AddFooter(FString& Buffer);

	static void BeginSection(FString& Buffer, const TCHAR* SectionName);
	static void EndSection(FString& Buffer, const TCHAR* SectionName);

	/** Called once when GConfig is initialized. Opportunity to cache values from config. */
	static void InitializeFromConfig();

	/** Called to update any localized strings in the crash context */
	static void UpdateLocalizedStrings();

	/**	Whether the Initialize() has been called */
	static bool bIsInitialized;

	/** The ID of the external process reporting crashes if the platform supports it and was configured to use it, zero otherwise (0 is a reserved system process ID, invalid for the out of process reporter). */
	static uint32 OutOfProcessCrashReporterPid;

	/** The out of process crash reporter exit code, if available. The 32 MSB indicates if the exit code is set and the 32 LSB contains the exit code. The value can be read/write from different threads. */
	static volatile int64 OutOfProcessCrashReporterExitCode;

	/**	Static counter records how many crash contexts have been constructed */
	static int32 StaticCrashContextIndex;

	/** The buffer used to store the crash's properties. */
	mutable FString CommonBuffer;

	/**	Records which crash context we were using the StaticCrashContextIndex counter */
	int32 CrashContextIndex;

	// FNoncopyable
	FGenericCrashContext( const FGenericCrashContext& ) = delete;
	FGenericCrashContext& operator=(const FGenericCrashContext&) = delete;
};

struct CORE_API FGenericMemoryWarningContext
{};

namespace RecoveryService
{
	/** Generates a name for the disaster recovery service embedded in the CrashReporterClientEditor. */
	CORE_API FString GetRecoveryServerName();

	/** Generates a name for the disaster recovery session. */
	CORE_API FString MakeSessionName();

	/** Tokenize the session name into its components. */
	CORE_API bool TokenizeSessionName(const FString& SessionName, FString* OutServerName, int32* SeqNum, FString* ProjName, FDateTime* DateTime);
}

#if WITH_ADDITIONAL_CRASH_CONTEXTS

/**
 * Interface for callbacks to add context to the crash report.
 */
struct FCrashContextExtendedWriter
{
	/** Adds a named buffer to the report. Intended for larger payloads. */
	CORE_API virtual void AddBuffer(const TCHAR* Identifier, const uint8* Data, uint32 DataSize) = 0;
		
	/** Add a named buffer containing a string to the report. */
	CORE_API virtual void AddString(const TCHAR* Identifier, const TCHAR* DataStr) = 0;
};

/**
 * A thread local stack of callbacks that can be issued at time of the crash.
 */
struct FAdditionalCrashContextStack
{
	CORE_API static FAdditionalCrashContextStack& GetThreadContextProvider();
	CORE_API static void PushProvider(struct FScopedAdditionalCrashContextProvider* Provider);
	CORE_API static void PopProvider();

	static void ExecuteProviders(FCrashContextExtendedWriter& Writer);

private:
	enum { MaxStackDepth = 16 };
	FAdditionalCrashContextStack* Next;
	const FScopedAdditionalCrashContextProvider* Stack[MaxStackDepth];
	uint32 StackIndex = 0;

	FAdditionalCrashContextStack();
	~FAdditionalCrashContextStack();
	
	inline void PushProviderInternal(const FScopedAdditionalCrashContextProvider* Provider) 
	{
		check(StackIndex < MaxStackDepth);
		Stack[StackIndex++] = Provider;
	}

	inline void PopProviderInternal()
	{
		check(StackIndex > 0);
		Stack[--StackIndex] = nullptr;
	}
};

struct FScopedAdditionalCrashContextProvider
{
public:
	FScopedAdditionalCrashContextProvider(TUniqueFunction<void(FCrashContextExtendedWriter&)> InFunc)
		: Func(MoveTemp(InFunc))
	{
		FAdditionalCrashContextStack::PushProvider(this);
	}

	~FScopedAdditionalCrashContextProvider()
	{
		FAdditionalCrashContextStack::PopProvider();
	}

	void Execute(FCrashContextExtendedWriter& Writer) const
	{
		Func(Writer);
	}

private:
	TUniqueFunction<void(FCrashContextExtendedWriter&)> Func;
};

#define UE_ADD_CRASH_CONTEXT_SCOPE(FuncExpr) FScopedAdditionalCrashContextProvider ANONYMOUS_VARIABLE(AddCrashCtx)(FuncExpr)

#else

#define UE_ADD_CRASH_CONTEXT_SCOPE(FuncExpr) 

#endif // WITH_ADDITIONAL_CRASH_CONTEXTS

