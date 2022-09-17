// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Trace/Config.h"
#include "Trace/Trace.h"

#if !defined(COUNTERSTRACE_ENABLED)
#if UE_TRACE_ENABLED && !UE_BUILD_SHIPPING
#define COUNTERSTRACE_ENABLED 1
#else
#define COUNTERSTRACE_ENABLED 0
#endif
#endif

enum ETraceCounterType
{
	TraceCounterType_Int,
	TraceCounterType_Float,
};

enum ETraceCounterDisplayHint
{
	TraceCounterDisplayHint_None,
	TraceCounterDisplayHint_Memory,
};

#if COUNTERSTRACE_ENABLED

UE_TRACE_CHANNEL_EXTERN(CountersChannel);

struct FCountersTrace
{
	CORE_API static uint16 OutputInitCounter(const TCHAR* CounterName, ETraceCounterType CounterType, ETraceCounterDisplayHint CounterDisplayHint);
	CORE_API static void OutputSetValue(uint16 CounterId, int64 Value);
	CORE_API static void OutputSetValue(uint16 CounterId, double Value);

	template<typename ValueType, ETraceCounterType CounterType>
	class TCounter
	{
	public:
		TCounter(const TCHAR* InCounterName, ETraceCounterDisplayHint InCounterDisplayHint)
			: Value(0)
			, CounterId(0)
			, CounterName(InCounterName)
			, CounterDisplayHint(InCounterDisplayHint)
		{
			CounterId = OutputInitCounter(InCounterName, CounterType, CounterDisplayHint);
		}

		const ValueType& Get() const
		{
			return Value;
		}

		void Set(ValueType InValue)
		{
			if (Value != InValue)
			{
				Value = InValue;
				OutputSetValue(CounterId, Value);
			}
		}

		void Add(ValueType InValue)
		{
			if (InValue != 0)
			{
				Value += InValue;
				OutputSetValue(CounterId, Value);
			}
		}

		void Subtract(ValueType InValue)
		{
			if (InValue != 0)
			{
				Value -= InValue;
				OutputSetValue(CounterId, Value);
			}
		}

		void Increment()
		{
			++Value;
			OutputSetValue(CounterId, Value);
		}
		
		void Decrement()
		{
			--Value;
			OutputSetValue(CounterId, Value);
		}
		
	private:
		ValueType Value;
		uint16 CounterId;
		const TCHAR* CounterName;
		ETraceCounterDisplayHint CounterDisplayHint;

		bool CheckCounterId()
		{
			CounterId = OutputInitCounter(CounterName, CounterType, CounterDisplayHint);
			return !!CounterId;
		}
	};

	using FCounterInt = TCounter<int64, TraceCounterType_Int>;
	using FCounterFloat = TCounter<double, TraceCounterType_Float>;
};

#define __TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, CounterType, CounterDisplayHint) \
	static FCountersTrace::CounterType PREPROCESSOR_JOIN(__TraceCounter, __LINE__)(CounterDisplayName, CounterDisplayHint);

#define TRACE_INT_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterInt, TraceCounterDisplayHint_None) \
	PREPROCESSOR_JOIN(__TraceCounter, __LINE__).Set(Value);

#define TRACE_FLOAT_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterFloat, TraceCounterDisplayHint_None) \
	PREPROCESSOR_JOIN(__TraceCounter, __LINE__).Set(Value);

#define TRACE_MEMORY_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterInt, TraceCounterDisplayHint_Memory) \
	PREPROCESSOR_JOIN(__TraceCounter, __LINE__).Set(Value);

#define TRACE_DECLARE_INT_COUNTER(CounterName, CounterDisplayName) \
	FCountersTrace::FCounterInt PREPROCESSOR_JOIN(__GTraceCounter, CounterName)(CounterDisplayName, TraceCounterDisplayHint_None);

#define TRACE_DECLARE_INT_COUNTER_EXTERN(CounterName) \
	extern FCountersTrace::FCounterInt PREPROCESSOR_JOIN(__GTraceCounter, CounterName);

#define TRACE_DECLARE_FLOAT_COUNTER(CounterName, CounterDisplayName) \
	FCountersTrace::FCounterFloat PREPROCESSOR_JOIN(__GTraceCounter, CounterName)(CounterDisplayName, TraceCounterDisplayHint_None);

#define TRACE_DECLARE_FLOAT_COUNTER_EXTERN(CounterName) \
	extern FCountersTrace::FCounterFloat PREPROCESSOR_JOIN(__GTraceCounter, CounterName);

#define TRACE_DECLARE_MEMORY_COUNTER(CounterName, CounterDisplayName) \
	FCountersTrace::FCounterInt PREPROCESSOR_JOIN(__GTraceCounter, CounterName)(CounterDisplayName, TraceCounterDisplayHint_Memory);

#define TRACE_DECLARE_MEMORY_COUNTER_EXTERN(CounterName) \
	TRACE_DECLARE_INT_COUNTER_EXTERN(CounterName)

#define TRACE_COUNTER_SET(CounterName, Value) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Set(Value);

#define TRACE_COUNTER_ADD(CounterName, Value) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Add(Value);

#define TRACE_COUNTER_SUBTRACT(CounterName, Value) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Subtract(Value);

#define TRACE_COUNTER_INCREMENT(CounterName) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Increment();

#define TRACE_COUNTER_DECREMENT(CounterName) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Decrement();

#else

#define TRACE_INT_VALUE(CounterDisplayName, Value)
#define TRACE_FLOAT_VALUE(CounterDisplayName, Value)
#define TRACE_MEMORY_VALUE(CounterDisplayName, Value)
#define TRACE_DECLARE_INT_COUNTER(CounterName, CounterDisplayName)
#define TRACE_DECLARE_INT_COUNTER_EXTERN(CounterName)
#define TRACE_DECLARE_FLOAT_COUNTER(CounterName, CounterDisplayName)
#define TRACE_DECLARE_FLOAT_COUNTER_EXTERN(CounterName)
#define TRACE_DECLARE_MEMORY_COUNTER(CounterName, CounterDisplayName)
#define TRACE_DECLARE_MEMORY_COUNTER_EXTERN(CounterName)
#define TRACE_COUNTER_SET(CounterName, Value)
#define TRACE_COUNTER_ADD(CounterName, Value)
#define TRACE_COUNTER_SUBTRACT(CounterName, Value)
#define TRACE_COUNTER_INCREMENT(CounterName)
#define TRACE_COUNTER_DECREMENT(CounterName)

#endif