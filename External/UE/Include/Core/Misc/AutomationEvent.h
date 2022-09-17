// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

enum class EAutomationEventType : uint8
{
	Info,
	Warning,
	Error
};

struct CORE_API FAutomationEvent
{
public:
	FAutomationEvent(EAutomationEventType InType, const FString& InMessage)
		: Type(InType)
		, Message(InMessage)
		, Context()
	{
	}

	FAutomationEvent(EAutomationEventType InType, const FString& InMessage, const FString& InContext)
		: Type(InType)
		, Message(InMessage)
		, Context(InContext)
	{
	}

public:

	EAutomationEventType Type;
	FString Message;
	FString Context;
	FGuid Artifact;
};

struct CORE_API FAutomationExecutionEntry
{
	FAutomationEvent Event;
	FString Filename;
	int32 LineNumber;
	FDateTime Timestamp;

	FAutomationExecutionEntry(FAutomationEvent InEvent)
		: Event(InEvent)
		, Filename()
		, LineNumber(-1)
		, Timestamp(FDateTime::UtcNow())
	{
	}

	FAutomationExecutionEntry(FAutomationEvent InEvent, FString InFilename, int32 InLineNumber)
		: Event(InEvent)
		, Filename(InFilename)
		, LineNumber(InLineNumber)
		, Timestamp(FDateTime::UtcNow())
	{
	}

	FString ToString() const;
};
