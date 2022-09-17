// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "HAL/Event.h"
#include <atomic>

namespace UE
{

class FLazyEvent
{
public:
	FLazyEvent(const FLazyEvent&) = delete;
	FLazyEvent& operator=(const FLazyEvent&) = delete;

	CORE_API explicit FLazyEvent(EEventMode EventMode);
	CORE_API ~FLazyEvent();

	CORE_API void Trigger();
	CORE_API void Reset();
	CORE_API void Wait();
	CORE_API bool Wait(uint32 WaitTime);

private:
	std::atomic<FEvent*> AtomicEvent;
};

} // UE
