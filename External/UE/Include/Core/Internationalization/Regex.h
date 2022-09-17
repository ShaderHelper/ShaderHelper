// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"

class FRegexPatternImplementation;
class FRegexMatcherImplementation;

enum class ERegexPatternFlags
{
	None = 0,
	CaseInsensitive = (1 << 0),
};
ENUM_CLASS_FLAGS(ERegexPatternFlags);

/**
 * Implements a regular expression pattern.
 * @note DO NOT use this class as a file-level variable as its construction relies on the internationalization system being initialized!
 */
class CORE_API FRegexPattern
{
	friend class FRegexMatcher;

public:
	explicit FRegexPattern(const FString& SourceString, ERegexPatternFlags Flags = ERegexPatternFlags::None);

	FRegexPattern(const FRegexPattern&) = default;
	FRegexPattern& operator=(const FRegexPattern&) = default;

	FRegexPattern(FRegexPattern&&) = default;
	FRegexPattern& operator=(FRegexPattern&&) = default;

private:
	TSharedRef<FRegexPatternImplementation> Implementation;
};

/**
 * Implements a regular expression pattern matcher.
 * @note DO NOT use this class as a file-level variable as its construction relies on the internationalization system being initialized!
 */
class CORE_API FRegexMatcher
{
public:
	FRegexMatcher(const FRegexPattern& SourcePattern, const FString& Input);
	FRegexMatcher(FRegexPattern&& SourcePattern, const FString& Input);

	FRegexMatcher(const FRegexMatcher&) = delete;
	FRegexMatcher& operator=(const FRegexMatcher&) = delete;

	FRegexMatcher(FRegexMatcher&&) = default;
	FRegexMatcher& operator=(FRegexMatcher&&) = default;

	bool FindNext();

	int32 GetMatchBeginning();
	int32 GetMatchEnding();

	int32 GetCaptureGroupBeginning(const int32 Index);
	int32 GetCaptureGroupEnding(const int32 Index);
	FString GetCaptureGroup(const int32 Index);

	int32 GetBeginLimit();
	int32 GetEndLimit();
	void SetLimits(const int32 BeginIndex, const int32 EndIndex);

private:
	FRegexPattern Pattern;
	TSharedRef<FRegexMatcherImplementation> Implementation;
};
