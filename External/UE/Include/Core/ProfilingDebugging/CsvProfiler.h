// Copyright Epic Games, Inc. All Rights Reserved.

/**
*
* A lightweight multi-threaded CSV profiler which can be used for profiling in Test/Shipping builds
*/

#pragma once

#include "CoreTypes.h"
#include "Containers/Queue.h"
#include "UObject/NameTypes.h"
#include "Templates/UniquePtr.h"
#include "Async/Future.h"
#include "Async/TaskGraphInterfaces.h"
#include "Misc/EnumClassFlags.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "ProfilingDebugging/CsvProfilerConfig.h"
#include "ProfilingDebugging/CsvProfilerTrace.h"

#include <atomic>

#if CSV_PROFILER

// Helpers
#define CSV_CATEGORY_INDEX(CategoryName)						(_GCsvCategory_##CategoryName.Index)
#define CSV_CATEGORY_INDEX_GLOBAL								(0)
#define CSV_STAT_FNAME(StatName)								(_GCsvStat_##StatName.Name)

// Inline stats (no up front definition)
#define CSV_SCOPED_TIMING_STAT(Category,StatName) \
	TRACE_CSV_PROFILER_INLINE_STAT(#StatName, CSV_CATEGORY_INDEX(Category)); \
	FScopedCsvStat _ScopedCsvStat_ ## StatName (#StatName, CSV_CATEGORY_INDEX(Category));
#define CSV_SCOPED_TIMING_STAT_GLOBAL(StatName) \
	TRACE_CSV_PROFILER_INLINE_STAT(#StatName, CSV_CATEGORY_INDEX_GLOBAL); \
	FScopedCsvStat _ScopedCsvStat_ ## StatName (#StatName, CSV_CATEGORY_INDEX_GLOBAL);
#define CSV_SCOPED_TIMING_STAT_EXCLUSIVE(StatName) \
	TRACE_CSV_PROFILER_INLINE_STAT_EXCLUSIVE(#StatName); \
	FScopedCsvStatExclusive _ScopedCsvStatExclusive_ ## StatName (#StatName);
#define CSV_SCOPED_TIMING_STAT_EXCLUSIVE_CONDITIONAL(StatName,Condition) \
	TRACE_CSV_PROFILER_INLINE_STAT_EXCLUSIVE(#StatName); \
	FScopedCsvStatExclusiveConditional _ScopedCsvStatExclusive_ ## StatName (#StatName,Condition);

#define CSV_SCOPED_WAIT(WaitTime)							FScopedCsvWaitConditional _ScopedCsvWait(WaitTime>0 && FCsvProfiler::IsWaitTrackingEnabledOnCurrentThread());
#define CSV_SCOPED_WAIT_CONDITIONAL(Condition)				FScopedCsvWaitConditional _ScopedCsvWait(Condition);

#define CSV_SCOPED_SET_WAIT_STAT(StatName) \
	TRACE_CSV_PROFILER_INLINE_STAT_EXCLUSIVE("EventWait/"#StatName); \
	FScopedCsvSetWaitStat _ScopedCsvSetWaitStat ## StatName("EventWait/"#StatName);

#define CSV_SCOPED_SET_WAIT_STAT_IGNORE()						FScopedCsvSetWaitStat _ScopedCsvSetWaitStat ## StatName();

#define CSV_CUSTOM_STAT(Category,StatName,Value,Op) \
	TRACE_CSV_PROFILER_INLINE_STAT(#StatName, CSV_CATEGORY_INDEX(Category)); \
	FCsvProfiler::RecordCustomStat(#StatName, CSV_CATEGORY_INDEX(Category), Value, Op);
#define CSV_CUSTOM_STAT_GLOBAL(StatName,Value,Op) \
	TRACE_CSV_PROFILER_INLINE_STAT(#StatName, CSV_CATEGORY_INDEX_GLOBAL); \
	FCsvProfiler::RecordCustomStat(#StatName, CSV_CATEGORY_INDEX_GLOBAL, Value, Op); 

// Stats declared up front
#define CSV_DEFINE_STAT(Category,StatName)						FCsvDeclaredStat _GCsvStat_##StatName((TCHAR*)TEXT(#StatName), CSV_CATEGORY_INDEX(Category));
#define CSV_DEFINE_STAT_GLOBAL(StatName)						FCsvDeclaredStat _GCsvStat_##StatName((TCHAR*)TEXT(#StatName), CSV_CATEGORY_INDEX_GLOBAL);
#define CSV_DECLARE_STAT_EXTERN(Category,StatName)				extern FCsvDeclaredStat _GCsvStat_##StatName
#define CSV_CUSTOM_STAT_DEFINED(StatName,Value,Op)				FCsvProfiler::RecordCustomStat(_GCsvStat_##StatName.Name, _GCsvStat_##StatName.CategoryIndex, Value, Op);

// Categories
#define CSV_DEFINE_CATEGORY(CategoryName,bDefaultValue)			FCsvCategory _GCsvCategory_##CategoryName(TEXT(#CategoryName),bDefaultValue)
#define CSV_DECLARE_CATEGORY_EXTERN(CategoryName)				extern FCsvCategory _GCsvCategory_##CategoryName

#define CSV_DEFINE_CATEGORY_MODULE(Module_API,CategoryName,bDefaultValue)	FCsvCategory Module_API _GCsvCategory_##CategoryName(TEXT(#CategoryName),bDefaultValue)
#define CSV_DECLARE_CATEGORY_MODULE_EXTERN(Module_API,CategoryName)			extern Module_API FCsvCategory _GCsvCategory_##CategoryName

// Events
#define CSV_EVENT(Category, Format, ...) \
	FCsvProfiler::RecordEventf( CSV_CATEGORY_INDEX(Category), Format, ##__VA_ARGS__ ); \
	TRACE_BOOKMARK(TEXT(PREPROCESSOR_TO_STRING(Category)) TEXT("/") Format, ##__VA_ARGS__)

#define CSV_EVENT_GLOBAL(Format, ...) \
	FCsvProfiler::RecordEventf( CSV_CATEGORY_INDEX_GLOBAL, Format, ##__VA_ARGS__ ); \
	TRACE_BOOKMARK(Format, ##__VA_ARGS__)

// Metadata
#define CSV_METADATA(Key,Value)									FCsvProfiler::SetMetadata( Key, Value )

#else
  #define CSV_CATEGORY_INDEX(CategoryName)						
  #define CSV_CATEGORY_INDEX_GLOBAL								
  #define CSV_STAT_FNAME(StatName)								
  #define CSV_SCOPED_TIMING_STAT(Category,StatName)				
  #define CSV_SCOPED_TIMING_STAT_GLOBAL(StatName)					
  #define CSV_SCOPED_TIMING_STAT_EXCLUSIVE(StatName)
  #define CSV_SCOPED_TIMING_STAT_EXCLUSIVE_CONDITIONAL(StatName,Condition)
  #define CSV_SCOPED_WAIT(WaitTime)
  #define CSV_SCOPED_WAIT_CONDITIONAL(Condition)
  #define CSV_SCOPED_SET_WAIT_STAT(StatName)
  #define CSV_SCOPED_SET_WAIT_STAT_IGNORE()
  #define CSV_CUSTOM_STAT(Category,StatName,Value,Op)				
  #define CSV_CUSTOM_STAT_GLOBAL(StatName,Value,Op) 				
  #define CSV_DEFINE_STAT(Category,StatName)						
  #define CSV_DEFINE_STAT_GLOBAL(StatName)						
  #define CSV_DECLARE_STAT_EXTERN(Category,StatName)				
  #define CSV_CUSTOM_STAT_DEFINED(StatName,Value,Op)				
  #define CSV_DEFINE_CATEGORY(CategoryName,bDefaultValue)			
  #define CSV_DECLARE_CATEGORY_EXTERN(CategoryName)				
  #define CSV_DEFINE_CATEGORY_MODULE(Module_API,CategoryName,bDefaultValue)	
  #define CSV_DECLARE_CATEGORY_MODULE_EXTERN(Module_API,CategoryName)			
  #define CSV_EVENT(Category, Format, ...) 						
  #define CSV_EVENT_GLOBAL(Format, ...)
  #define CSV_METADATA(Key,Value)
#endif


#if CSV_PROFILER

class FCsvProfilerFrame;
class FCsvProfilerThreadData;
class FCsvProfilerProcessingThread;
class FName;

enum class ECsvCustomStatOp : uint8
{
	Set,
	Min,
	Max,
	Accumulate,
};

enum class ECsvCommandType : uint8
{
	Start,
	Stop,
	Count
};

struct FCsvCategory;

struct FCsvDeclaredStat
{
	FCsvDeclaredStat(TCHAR* InNameString, uint32 InCategoryIndex) 
		: Name(InNameString)
		, CategoryIndex(InCategoryIndex) 
	{
		TRACE_CSV_PROFILER_DECLARED_STAT(Name, InCategoryIndex);
	}

	FName Name;
	uint32 CategoryIndex;
};

enum class ECsvProfilerFlags
{
	None = 0,
	WriteCompletionFile = 1,
	CompressOutput = 2
};
ENUM_CLASS_FLAGS(ECsvProfilerFlags);

struct FCsvCaptureCommand
{
	FCsvCaptureCommand()
		: CommandType(ECsvCommandType::Count)
		, FrameRequested(-1)
		, Value(-1)
	{}

	FCsvCaptureCommand(ECsvCommandType InCommandType, uint32 InFrameRequested, uint32 InValue, const FString& InDestinationFolder, const FString& InFilename, ECsvProfilerFlags InFlags)
		: CommandType(InCommandType)
		, FrameRequested(InFrameRequested)
		, Value(InValue)
		, DestinationFolder(InDestinationFolder)
		, Filename(InFilename)
		, Flags(InFlags)
	{}

	FCsvCaptureCommand(ECsvCommandType InCommandType, uint32 InFrameRequested, TPromise<FString>* InCompletion, TSharedFuture<FString> InFuture)
		: CommandType(InCommandType)
		, FrameRequested(InFrameRequested)
		, Completion(MoveTemp(InCompletion))
		, Future(InFuture)
	{}

	ECsvCommandType CommandType;
	uint32 FrameRequested;
	uint32 Value;
	FString DestinationFolder;
	FString Filename;
	ECsvProfilerFlags Flags;
	TPromise<FString>* Completion;
	TSharedFuture<FString> Future;
};

/**
* FCsvProfiler class. This manages recording and reporting all for CSV stats
*/
class FCsvProfiler
{
	friend class FCsvProfilerProcessingThread;
	friend class FCsvProfilerThreadData;
	friend struct FCsvCategory;
public:
	FCsvProfiler();
	~FCsvProfiler();
	static CORE_API FCsvProfiler* Get();

	CORE_API void Init();

	/** Begin static interface (used by macros)*/
	/** Push/pop events */
	CORE_API static void BeginStat(const char* StatName, uint32 CategoryIndex);
	CORE_API static void BeginStat(const FName& StatName, uint32 CategoryIndex);
	CORE_API static void EndStat(const char* StatName, uint32 CategoryIndex);
	CORE_API static void EndStat(const FName& StatName, uint32 CategoryIndex);

	CORE_API static void BeginExclusiveStat(const char * StatName);
	CORE_API static void EndExclusiveStat(const char * StatName);

	CORE_API static void RecordCustomStat(const char * StatName, uint32 CategoryIndex, float Value, const ECsvCustomStatOp CustomStatOp);
	CORE_API static void RecordCustomStat(const FName& StatName, uint32 CategoryIndex, float Value, const ECsvCustomStatOp CustomStatOp);
	CORE_API static void RecordCustomStat(const char* StatName, uint32 CategoryIndex, double Value, const ECsvCustomStatOp CustomStatOp);
	CORE_API static void RecordCustomStat(const FName& StatName, uint32 CategoryIndex, double Value, const ECsvCustomStatOp CustomStatOp);
	CORE_API static void RecordCustomStat(const char * StatName, uint32 CategoryIndex, int32 Value, const ECsvCustomStatOp CustomStatOp);
	CORE_API static void RecordCustomStat(const FName& StatName, uint32 CategoryIndex, int32 Value, const ECsvCustomStatOp CustomStatOp);

	CORE_API static void RecordEvent(int32 CategoryIndex, const FString& EventText);
	CORE_API static void RecordEventAtTimestamp(int32 CategoryIndex, const FString& EventText, uint64 Cycles64);

	CORE_API static void SetMetadata(const TCHAR* Key, const TCHAR* Value);
	
	static CORE_API int32 RegisterCategory(const FString& Name, bool bEnableByDefault, bool bIsGlobal);
	static CORE_API int32 GetCategoryIndex(const FString& Name);

	template <typename FmtType, typename... Types>
	FORCEINLINE static void RecordEventf(int32 CategoryIndex, const FmtType& Fmt, Types... Args)
	{
		static_assert(TIsArrayOrRefOfType<FmtType, TCHAR>::Value, "Formatting string must be a TCHAR array.");
		static_assert(TAnd<TIsValidVariadicFunctionArg<Types>...>::Value, "Invalid argument(s) passed to FCsvProfiler::RecordEventf");
		RecordEventfInternal(CategoryIndex, Fmt, Args...);
	}

	CORE_API static void BeginSetWaitStat(const char * StatName);
	CORE_API static void EndSetWaitStat();

	CORE_API static void BeginWait();
	CORE_API static void EndWait();

	/** Singleton interface */
	CORE_API bool IsCapturing();
	CORE_API bool IsCapturing_Renderthread();
	CORE_API bool IsWritingFile();

	CORE_API int32 GetCaptureFrameNumber();
	CORE_API int32 GetNumFrameToCaptureOnEvent();

	CORE_API bool EnableCategoryByString(const FString& CategoryName) const;
	CORE_API void EnableCategoryByIndex(uint32 CategoryIndex, bool bEnable) const;
	CORE_API bool IsCategoryEnabled(uint32 CategoryIndex) const;

	/** Per-frame update */
	CORE_API void BeginFrame();
	CORE_API void EndFrame();

	/** Begin Capture */
	CORE_API void BeginCapture(int InNumFramesToCapture = -1,
		const FString& InDestinationFolder = FString(),
		const FString& InFilename = FString(),
		ECsvProfilerFlags InFlags = ECsvProfilerFlags::None);

	/**
	 * End Capture
	 * EventToSignal is optional. If provided, the CSV profiler will signal the event when the async file write is complete.
	 * The returned TFuture can be waited on to retrieve the filename of the csv file that was written to disk.
	 */
	CORE_API TSharedFuture<FString> EndCapture(FGraphEventRef EventToSignal = nullptr);

	/** Called at the end of the first frame after forking */
	CORE_API void OnEndFramePostFork();

	/** Renderthread begin/end frame */
	CORE_API void BeginFrameRT();
	CORE_API void EndFrameRT();

	CORE_API void SetDeviceProfileName(FString InDeviceProfileName);

	CORE_API FString GetOutputFilename() const { return OutputFilename; }

	CORE_API static bool IsWaitTrackingEnabledOnCurrentThread();

	CORE_API void GetFrameExecCommands(TArray<FString>& OutFrameCommands) const;

	DECLARE_MULTICAST_DELEGATE(FOnCSVProfileStart);
	FOnCSVProfileStart& OnCSVProfileStart() { return OnCSVProfileStartDelegate; }

	DECLARE_MULTICAST_DELEGATE(FOnCSVProfileEnd);
	FOnCSVProfileEnd& OnCSVProfileEnd() { return OnCSVProfileEndDelegate; }
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCSVProfileFinished, const FString& /*Filename */);
	FOnCSVProfileFinished& OnCSVProfileFinished() { return OnCSVProfileFinishedDelegate; }

	CORE_API void SetRenderThreadId(uint32 InRenderThreadId)
	{
		RenderThreadId = InRenderThreadId;
	}

	CORE_API void SetRHIThreadId(uint32 InRHIThreadId)
	{
		RHIThreadId = InRHIThreadId;
	}

private:
	CORE_API static void VARARGS RecordEventfInternal(int32 CategoryIndex, const TCHAR* Fmt, ...);

	void SetMetadataInternal(const TCHAR* Key, const TCHAR* Value, bool bSanitize=true);

	void FinalizeCsvFile();

	float ProcessStatData();

	int32 NumFramesToCapture;
	int32 CaptureFrameNumber;
	int32 CaptureOnEventFrameCount;

	bool bInsertEndFrameAtFrameStart;

	uint64 LastEndFrameTimestamp;
	uint32 CaptureEndFrameCount;

	FString OutputFilename;
	TQueue<FCsvCaptureCommand> CommandQueue;
	FCsvProfilerProcessingThread* ProcessingThread;

	FEvent* FileWriteBlockingEvent;
	FThreadSafeCounter IsShuttingDown;

	TMap<FString, FString> MetadataMap;
	TQueue<TMap<FString, FString>> MetadataQueue;
	FCriticalSection MetadataCS;

	class FCsvStreamWriter* CsvWriter;

	ECsvProfilerFlags CurrentFlags;

	FOnCSVProfileStart OnCSVProfileStartDelegate;
	FOnCSVProfileEnd OnCSVProfileEndDelegate;
	
	FOnCSVProfileFinished OnCSVProfileFinishedDelegate;

	std::atomic<uint32> RenderThreadId{ 0 };
	std::atomic<uint32> RHIThreadId{ 0 };
};

class FScopedCsvStat
{
public:
	FScopedCsvStat(const char * InStatName, uint32 InCategoryIndex)
		: StatName(InStatName)
		, CategoryIndex(InCategoryIndex)
	{
		FCsvProfiler::BeginStat(StatName, CategoryIndex);
	}

	~FScopedCsvStat()
	{
		FCsvProfiler::EndStat(StatName, CategoryIndex);
	}
	const char * StatName;
	uint32 CategoryIndex;
};

class FScopedCsvStatExclusive 
{
public:
	FScopedCsvStatExclusive(const char * InStatName)
		: StatName(InStatName)
	{
		FCsvProfiler::BeginExclusiveStat(StatName);
	}

	~FScopedCsvStatExclusive()
	{
		FCsvProfiler::EndExclusiveStat(StatName);
	}
	const char * StatName;
};

class FScopedCsvStatExclusiveConditional
{
public:
	FScopedCsvStatExclusiveConditional(const char * InStatName, bool bInCondition)
		: StatName(InStatName)
		, bCondition(bInCondition)
	{
		if (bCondition)
		{
			FCsvProfiler::BeginExclusiveStat(StatName);
		}
	}

	~FScopedCsvStatExclusiveConditional()
	{
		if (bCondition)
		{
			FCsvProfiler::EndExclusiveStat(StatName);
		}
	}
	const char * StatName;
	bool bCondition;
};

class FScopedCsvWaitConditional
{
public:
	FScopedCsvWaitConditional(bool bInCondition)
		: bCondition(bInCondition)
	{
		if (bCondition)
		{
			FCsvProfiler::BeginWait();
		}
	}

	~FScopedCsvWaitConditional()
	{
		if (bCondition)
		{
			FCsvProfiler::EndWait();
		}
	}
	bool bCondition;
};


class FScopedCsvSetWaitStat
{
public:
	FScopedCsvSetWaitStat(const char * InStatName = nullptr)
		: StatName(InStatName)
	{
		FCsvProfiler::BeginSetWaitStat(StatName);
	}

	~FScopedCsvSetWaitStat()
	{
		FCsvProfiler::EndSetWaitStat();
	}
	const char * StatName;
};

struct CORE_API FCsvCategory
{
	FCsvCategory() : Index(-1) {}
	FCsvCategory(const TCHAR* CategoryString, bool bDefaultValue, bool bIsGlobal = false)
	{
		Name = CategoryString;
		Index = FCsvProfiler::RegisterCategory(Name, bDefaultValue, bIsGlobal);
	}

	uint32 Index;
	FString Name;
};


CSV_DECLARE_CATEGORY_MODULE_EXTERN(CORE_API, Exclusive);


#endif //CSV_PROFILER