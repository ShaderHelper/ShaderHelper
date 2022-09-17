// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Math/Color.h"
#include "UObject/NameTypes.h"
#include "Delegates/Delegate.h"
#include "HAL/PlatformTime.h"
#include "HAL/ThreadSingleton.h"
#include "StatsCommon.h"
#include "ProfilingDebugging/UMemoryDefines.h"
#include "Stats/Stats2.h"

/** 
 *	Learn about the Stats System at docs.unrealengine.com
 */

struct TStatId;
enum class EStatFlags : uint8;

// used by the profiler
enum EStatType
{
	STATTYPE_CycleCounter,
	STATTYPE_AccumulatorFLOAT,
	STATTYPE_AccumulatorDWORD,
	STATTYPE_CounterFLOAT,
	STATTYPE_CounterDWORD,
	STATTYPE_MemoryCounter,
	STATTYPE_Error
};

/*----------------------------------------------------------------------------
	Stats helpers
----------------------------------------------------------------------------*/

#if STATS
	#define STAT(x) x
#else
	#define STAT(x)
#endif

#ifndef USE_LIGHTWEIGHT_STATS_FOR_HITCH_DETECTION
#define USE_LIGHTWEIGHT_STATS_FOR_HITCH_DETECTION 1
#endif

#if STATS

/**
 * This is a utility class for counting the number of cycles during the
 * lifetime of the object. It updates the per thread values for this
 * stat.
 */
class FScopeCycleCounter : public FCycleCounter
{
public:
	/**
	 * Pushes the specified stat onto the hierarchy for this thread. Starts
	 * the timing of the cycles used
	 */
	FORCEINLINE_STATS FScopeCycleCounter( TStatId StatId, EStatFlags StatFlags, bool bAlways = false
#if CPUPROFILERTRACE_ENABLED
		// Optional verbose description added for CPU profiler, without affecting the stat name.  Should be invariant
		// across frames, to avoid bloating memory in the name map used in the CPU profiler.
		, const TCHAR* OptionalVerboseDescription = nullptr
#endif
	)
	{
		Start( StatId, StatFlags, bAlways
#if CPUPROFILERTRACE_ENABLED
			, OptionalVerboseDescription
#endif
			);
	}

	FORCEINLINE_STATS FScopeCycleCounter(TStatId StatId, bool bAlways = false)
		: FScopeCycleCounter(StatId, EStatFlags::None, bAlways)
	{}

	/**
	 * Updates the stat with the time spent
	 */
	FORCEINLINE_STATS ~FScopeCycleCounter()
	{
		Stop();
	}

};

FORCEINLINE void StatsMasterEnableAdd(int32 Value = 1)
{
	FThreadStats::MasterEnableAdd(Value);
}
FORCEINLINE void StatsMasterEnableSubtract(int32 Value = 1)
{
	FThreadStats::MasterEnableSubtract(Value);
}

#else	//STATS
#if ENABLE_STATNAMEDEVENTS

struct FStatStringWrapper
{
	
};

#if	PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING
typedef ANSICHAR PROFILER_CHAR;
#else
typedef WIDECHAR PROFILER_CHAR;
#endif // PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING

struct TStatId
{
	const PROFILER_CHAR* StatString;

	FORCEINLINE TStatId()
	: StatString(nullptr)
	{
	}

	FORCEINLINE TStatId(const PROFILER_CHAR* InString)
	: StatString(InString)
	{
	}
	
	FORCEINLINE bool IsValidStat() const
	{
		return StatString != nullptr;
	}

	FORCEINLINE bool operator==(TStatId Other) const
	{
		return StatString == Other.StatString;
	}

	FORCEINLINE bool operator!=(TStatId Other) const
	{
		return StatString != Other.StatString;
	}
};

#if USE_LIGHTWEIGHT_STATS_FOR_HITCH_DETECTION && USE_HITCH_DETECTION
extern CORE_API TSAN_ATOMIC(bool) GHitchDetected;

class FLightweightStatScope
{
	const PROFILER_CHAR* StatString;
public:
	FORCEINLINE FLightweightStatScope(const PROFILER_CHAR* InStat)
	{
		StatString = GHitchDetected ? nullptr : InStat;
	}

	FORCEINLINE ~FLightweightStatScope()
	{
		if (GHitchDetected && StatString)
		{
			ReportHitch();
		}
	}

	CORE_API void ReportHitch();
};

#endif

class FScopeCycleCounter
{
public:
	FORCEINLINE FScopeCycleCounter(TStatId InStatId, EStatFlags StatFlags, bool bAlways = false)
		: 
#if USE_LIGHTWEIGHT_STATS_FOR_HITCH_DETECTION && USE_HITCH_DETECTION
		StatScope(InStatId.StatString),
#endif
		
		bPop(false)
	{
		if (GCycleStatsShouldEmitNamedEvents && InStatId.IsValidStat())
		{
			bPop = true;
			FPlatformMisc::BeginNamedEvent(FColor(0), InStatId.StatString);
		}
	}

	FORCEINLINE FScopeCycleCounter(TStatId InStatId, bool bAlways = false)
		: FScopeCycleCounter(InStatId, EStatFlags::None, bAlways)
	{
	}

	FORCEINLINE ~FScopeCycleCounter()
	{
		if (bPop)
		{
			FPlatformMisc::EndNamedEvent();
		}
	}
private:
#if USE_LIGHTWEIGHT_STATS_FOR_HITCH_DETECTION && USE_HITCH_DETECTION
	FLightweightStatScope StatScope;
#endif
	bool bPop;
};

#else	//ENABLE_STATNAMEDEVENTS
struct TStatId {};

class FScopeCycleCounter
{
public:
	FORCEINLINE_STATS FScopeCycleCounter(TStatId, EStatFlags, bool bAlways = false)
	{
	}
	FORCEINLINE_STATS FScopeCycleCounter(TStatId, bool bAlways = false)
	{
	}
};
#endif

FORCEINLINE void StatsMasterEnableAdd(int32 Value = 1)
{
}
FORCEINLINE void StatsMasterEnableSubtract(int32 Value = 1)
{
}



#if ENABLE_STATNAMEDEVENTS

#if PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING
#define ANSI_TO_PROFILING(x) x
#else
#define ANSI_TO_PROFILING(x) TEXT(x)
#endif

#define SCOPE_CYCLE_COUNTER_TO_TRACE(StatString, StatName, Condition) \
	TRACE_CPUPROFILER_EVENT_DECLARE(StatString, PREPROCESSOR_JOIN(PREPROCESSOR_JOIN(__Decl_, StatName), __LINE__), CpuChannel, Condition && GCycleStatsShouldEmitNamedEvents>0); \
	TRACE_CPUPROFILER_EVENT_SCOPE_USE(PREPROCESSOR_JOIN(PREPROCESSOR_JOIN(__Decl_, StatName), __LINE__), PREPROCESSOR_JOIN(PREPROCESSOR_JOIN(__Scope_, StatName), __LINE__), CpuChannel, Condition && GCycleStatsShouldEmitNamedEvents>0);

#define DECLARE_SCOPE_CYCLE_COUNTER(CounterName,Stat,GroupId) \
	FScopeCycleCounter StatNamedEventsScope_##Stat(TStatId(ANSI_TO_PROFILING(#Stat))); \
	SCOPE_CYCLE_COUNTER_TO_TRACE(CounterName, Stat, true);

#define QUICK_SCOPE_CYCLE_COUNTER(Stat) \
	FScopeCycleCounter StatNamedEventsScope_##Stat(TStatId(ANSI_TO_PROFILING(#Stat))); \
	SCOPE_CYCLE_COUNTER_TO_TRACE(#Stat, Stat, true);

#define SCOPE_CYCLE_COUNTER(Stat) \
	FScopeCycleCounter StatNamedEventsScope_##Stat(TStatId(ANSI_TO_PROFILING(#Stat))); \
	SCOPE_CYCLE_COUNTER_TO_TRACE(#Stat, Stat, true);

#define CONDITIONAL_SCOPE_CYCLE_COUNTER(Stat,bCondition) \
	FScopeCycleCounter StatNamedEventsScope_##Stat(bCondition ? ANSI_TO_PROFILING(#Stat) : nullptr); \
	SCOPE_CYCLE_COUNTER_TO_TRACE(#Stat, Stat, bCondition);

#define SCOPE_CYCLE_COUNTER_VERBOSE(Stat, VerboseDescription) \
	FScopeCycleCounter StatNamedEventsScope_##Stat(TStatId(ANSI_TO_PROFILING(#Stat))); \
	SCOPE_CYCLE_COUNTER_TO_TRACE(#Stat, Stat, true);

#define RETURN_QUICK_DECLARE_CYCLE_STAT(StatId,GroupId) return TStatId(ANSI_TO_PROFILING(#StatId));

#define GET_STATID(Stat) (TStatId(ANSI_TO_PROFILING(#Stat)))


#elif USE_LIGHTWEIGHT_STATS_FOR_HITCH_DETECTION && USE_HITCH_DETECTION
extern CORE_API TSAN_ATOMIC(bool) GHitchDetected;

class FLightweightStatScope
{
	const TCHAR* StatString;
public:
	FORCEINLINE FLightweightStatScope(const TCHAR* InStat)
	{
		StatString = GHitchDetected ? nullptr : InStat;
	}

	FORCEINLINE ~FLightweightStatScope()
	{
		if (GHitchDetected && StatString)
		{
			ReportHitch();
		}
	}

	CORE_API void ReportHitch();
};

#define DECLARE_SCOPE_CYCLE_COUNTER(CounterName,Stat,GroupId) \
	FLightweightStatScope LightweightStatScope_##Stat(TEXT(#Stat));

#define QUICK_SCOPE_CYCLE_COUNTER(Stat) \
	FLightweightStatScope LightweightStatScope_##Stat(TEXT(#Stat));

#define SCOPE_CYCLE_COUNTER(Stat) \
	FLightweightStatScope LightweightStatScope_##Stat(TEXT(#Stat));

#define CONDITIONAL_SCOPE_CYCLE_COUNTER(Stat,bCondition) \
	FLightweightStatScope LightweightStatScope_##Stat(bCondition ? TEXT(#Stat) : nullptr);

#define SCOPE_CYCLE_COUNTER_VERBOSE(Stat,VerboseDescription) \
	FLightweightStatScope LightweightStatScope_##Stat(TEXT(#Stat));

#define RETURN_QUICK_DECLARE_CYCLE_STAT(StatId,GroupId) return TStatId();
#define GET_STATID(Stat) (TStatId())

#else
#define SCOPE_CYCLE_COUNTER(Stat)
#define QUICK_SCOPE_CYCLE_COUNTER(Stat)
#define DECLARE_SCOPE_CYCLE_COUNTER(CounterName,StatId,GroupId)
#define CONDITIONAL_SCOPE_CYCLE_COUNTER(Stat,bCondition)
#define SCOPE_CYCLE_COUNTER_VERBOSE(Stat,VerboseDescription)
#define RETURN_QUICK_DECLARE_CYCLE_STAT(StatId,GroupId) return TStatId();
#define GET_STATID(Stat) (TStatId())

#endif

#define SCOPE_SECONDS_ACCUMULATOR(Stat)
#define SCOPE_MS_ACCUMULATOR(Stat)
#define DEFINE_STAT(Stat)
#define QUICK_USE_CYCLE_STAT(StatId,GroupId) TStatId()
#define DECLARE_CYCLE_STAT(CounterName,StatId,GroupId)
#define DECLARE_CYCLE_STAT_WITH_FLAGS(CounterName,StatId,GroupId,StatFlags)
#define DECLARE_FLOAT_COUNTER_STAT(CounterName,StatId,GroupId)
#define DECLARE_DWORD_COUNTER_STAT(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_ACCUMULATOR_STAT(CounterName,StatId,GroupId)
#define DECLARE_DWORD_ACCUMULATOR_STAT(CounterName,StatId,GroupId)
#define DECLARE_FNAME_STAT(CounterName,StatId,GroupId, API)
#define DECLARE_PTR_STAT(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT_POOL(CounterName,StatId,GroupId,Pool)
#define DECLARE_CYCLE_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_CYCLE_STAT_WITH_FLAGS_EXTERN(CounterName,StatId,GroupId,StatFlags, API)
#define DECLARE_FLOAT_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_DWORD_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_FLOAT_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_FNAME_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_PTR_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_MEMORY_STAT_EXTERN(CounterName,StatId,GroupId, API)
#define DECLARE_MEMORY_STAT_POOL_EXTERN(CounterName,StatId,GroupId,Pool, API)

#define DECLARE_STATS_GROUP(GroupDesc,GroupId,GroupCat)
#define DECLARE_STATS_GROUP_VERBOSE(GroupDesc,GroupId,GroupCat)
#define DECLARE_STATS_GROUP_MAYBE_COMPILED_OUT(GroupDesc,GroupId,GroupCat,CompileIn)

#define SET_CYCLE_COUNTER(Stat,Cycles)
#define INC_DWORD_STAT(StatId)
#define INC_FLOAT_STAT_BY(StatId,Amount)
#define INC_DWORD_STAT_BY(StatId,Amount)
#define INC_DWORD_STAT_FNAME_BY(StatId,Amount)
#define INC_MEMORY_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT(StatId)
#define DEC_FLOAT_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT_FNAME_BY(StatId,Amount)
#define DEC_MEMORY_STAT_BY(StatId,Amount)
#define SET_MEMORY_STAT(StatId,Value)
#define SET_DWORD_STAT(StatId,Value)
#define SET_FLOAT_STAT(StatId,Value)
#define STAT_ADD_CUSTOMMESSAGE_NAME(StatId,Value)
#define STAT_ADD_CUSTOMMESSAGE_PTR(StatId,Value)

#define SET_CYCLE_COUNTER_FName(Stat,Cycles)
#define INC_DWORD_STAT_FName(Stat)
#define INC_FLOAT_STAT_BY_FName(Stat, Amount)
#define INC_DWORD_STAT_BY_FName(Stat, Amount)
#define INC_MEMORY_STAT_BY_FName(Stat, Amount)
#define DEC_DWORD_STAT_FName(Stat)
#define DEC_FLOAT_STAT_BY_FName(Stat,Amount)
#define DEC_DWORD_STAT_BY_FName(Stat,Amount)
#define DEC_MEMORY_STAT_BY_FName(Stat,Amount)
#define SET_MEMORY_STAT_FName(Stat,Value)
#define SET_DWORD_STAT_FName(Stat,Value)
#define SET_FLOAT_STAT_FName(Stat,Value)

#define GET_STATFNAME(Stat) (FName())
#define GET_STATDESCRIPTION(Stat) (nullptr)

#endif


/** Helper class used to generate dynamic stat ids. */
struct FDynamicStats
{
	/**
	* Create a new stat id and registers it with the stats system.
	* This is the only way to create dynamic stat ids at runtime.
	* Can be used only with FScopeCycleCounters.
	*
	* Store the created stat id.
	* Expensive method, avoid calling that method every frame.
	*
	* Example:
	*	FDynamicStats::CreateStatId<STAT_GROUP_TO_FStatGroup( STATGROUP_UObjects )>( FString::Printf(TEXT("MyDynamicStat_%i"),Index) )
	*/
	template< typename TStatGroup >
	static TStatId CreateStatId( const FString& StatNameOrDescription )
	{
#if	STATS
		return CreateStatIdInternal<TStatGroup>( FName( *StatNameOrDescription ), EStatDataType::ST_int64, true);
#endif // STATS

		return TStatId();
	}

	template< typename TStatGroup >
	static TStatId CreateStatIdInt64(const FString& StatNameOrDescription, bool bIsAccumulator = false)
	{
#if	STATS
		return CreateStatIdInternal<TStatGroup>(FName(*StatNameOrDescription), EStatDataType::ST_int64, false, !bIsAccumulator);
#endif // STATS

		return TStatId();
	}

	template< typename TStatGroup >
	static TStatId CreateStatIdDouble(const FString& StatNameOrDescription, bool bIsAccumulator=false)
	{
#if	STATS
		return CreateStatIdInternal<TStatGroup>(FName(*StatNameOrDescription), EStatDataType::ST_double, false, !bIsAccumulator);
#endif // STATS
		return TStatId();
	}

	template< typename TStatGroup >
	static TStatId CreateStatId(const FName StatNameOrDescription, bool IsTimer = true)
	{
#if	STATS
		return CreateStatIdInternal<TStatGroup>(StatNameOrDescription, EStatDataType::ST_int64, IsTimer);
#endif // STATS
		return TStatId();
	}

	template< typename TStatGroup >
	static TStatId CreateMemoryStatId(const FString& StatNameOrDescription, FPlatformMemory::EMemoryCounterRegion MemRegion=FPlatformMemory::MCR_Physical)
	{
#if	STATS
		return CreateMemoryStatId<TStatGroup>(FName(*StatNameOrDescription), MemRegion);
#endif // STATS

		return TStatId();
	}

	template< typename TStatGroup >
	static TStatId CreateMemoryStatId(const FName StatNameOrDescription, FPlatformMemory::EMemoryCounterRegion MemRegion=FPlatformMemory::MCR_Physical)
	{
#if	STATS
		FStartupMessages::Get().AddMetadata(StatNameOrDescription, *StatNameOrDescription.ToString(),
			TStatGroup::GetGroupName(),
			TStatGroup::GetGroupCategory(),
			TStatGroup::GetDescription(),
			false, EStatDataType::ST_int64, false, false, MemRegion);

		TStatId StatID = IStatGroupEnableManager::Get().GetHighPerformanceEnableForStat(StatNameOrDescription,
			TStatGroup::GetGroupName(),
			TStatGroup::GetGroupCategory(),
			TStatGroup::DefaultEnable,
			false, EStatDataType::ST_int64, *StatNameOrDescription.ToString(), false, false, MemRegion);

		return StatID;
#endif // STATS

		return TStatId();
	}

#if	STATS
private: // private since this can only be declared if STATS is defined, due to EStatDataType in signature
	template< typename TStatGroup >
	static TStatId CreateStatIdInternal(const FName StatNameOrDescription, EStatDataType::Type Type, bool IsTimer, bool bClearEveryFrame=true)
	{

		FStartupMessages::Get().AddMetadata(StatNameOrDescription, nullptr,
			TStatGroup::GetGroupName(),
			TStatGroup::GetGroupCategory(),
			TStatGroup::GetDescription(),
			bClearEveryFrame, Type, IsTimer, false);

		TStatId StatID = IStatGroupEnableManager::Get().GetHighPerformanceEnableForStat(StatNameOrDescription,
			TStatGroup::GetGroupName(),
			TStatGroup::GetGroupCategory(),
			TStatGroup::DefaultEnable,
			bClearEveryFrame, Type, nullptr, IsTimer, false);

		return StatID;
	}
#endif // STATS
};
