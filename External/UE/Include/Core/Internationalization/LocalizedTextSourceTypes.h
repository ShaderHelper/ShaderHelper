// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/EnumClassFlags.h"
#include "Templates/SharedPointer.h"

/**
 * Categories of localized text.
 * @note This enum is mirrored in NoExportTypes.h for UHT.
 */
enum class ELocalizedTextSourceCategory : uint8
{
	Game,
	Engine,
	Editor,
};

/**
 * Result codes from calling QueryLocalizedResourceResult.
 */
enum class EQueryLocalizedResourceResult : uint8
{
	/** Indicates the query found a matching entry and added its result */
	Found,
	/** Indicates that the query failed to find a matching entry */
	NotFound,
	/** Indicates that the query failed as this text source doesn't support queries */
	NotImplemented,
};

/**
 * Load flags used in localization initialization.
 */
enum class ELocalizationLoadFlags : uint8
{
	/** Load no data */
	None = 0,

	/** Load native data */
	Native = 1<<0,

	/** Load editor localization data */
	Editor = 1<<1,

	/** Load game localization data */
	Game = 1<<2,

	/** Load engine localization data */
	Engine = 1<<3,

	/** Load additional (eg, plugin) localization data */
	Additional = 1<<4,

	/** Force localized game data to be loaded, even when running in the editor */
	ForceLocalizedGame = 1<<5,
};
ENUM_CLASS_FLAGS(ELocalizationLoadFlags);

/**
 * Pre-defined priorities for ILocalizedTextSource.
 */
struct ELocalizedTextSourcePriority
{
	enum Enum
	{
		Lowest = -1000,
		Low = -100,
		Normal = 0,
		High = 100,
		Highest = 1000,
	};
};

typedef TSharedRef<FString, ESPMode::ThreadSafe> FTextDisplayStringRef;
typedef TSharedPtr<FString, ESPMode::ThreadSafe> FTextDisplayStringPtr;
typedef TSharedRef<const FString, ESPMode::ThreadSafe> FTextConstDisplayStringRef;
typedef TSharedPtr<const FString, ESPMode::ThreadSafe> FTextConstDisplayStringPtr;

inline FTextDisplayStringRef MakeTextDisplayString(FString&& InDisplayString)
{
	return MakeShared<FString, ESPMode::ThreadSafe>(MoveTemp(InDisplayString));
}
