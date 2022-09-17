// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Containers/StringView.h"
#include "Internationalization/Text.h"

class IBreakIterator
{
public:
	virtual ~IBreakIterator() = default;

	virtual void SetString(const FText& InText)
	{
		SetString(CopyTemp(InText.ToString()));
	}
	virtual void SetString(const FString& InString)
	{
		SetString(CopyTemp(InString));
	}
	virtual void SetString(const TCHAR* const InString, const int32 InStringLength)
	{
		SetString(FString(InStringLength, InString));
	}
	virtual void SetString(FString&& InString) = 0;
	
	virtual void SetStringRef(const FText& InText)
	{
		SetStringRef(FStringView(InText.ToString()));
	}
	virtual void SetStringRef(const FString& InString)
	{
		SetStringRef(FStringView(InString));
	}
	virtual void SetStringRef(const TCHAR* const InString, const int32 InStringLength)
	{
		SetStringRef(FStringView(InString, InStringLength));
	}
	virtual void SetStringRef(const FString* InString)
	{
		check(InString);
		SetStringRef(FStringView(*InString));
	}
	virtual void SetStringRef(FStringView InString) = 0;

	virtual void ClearString()
	{
		SetStringRef(FStringView());
	}

	virtual int32 GetCurrentPosition() const = 0;

	virtual int32 ResetToBeginning() = 0;
	virtual int32 ResetToEnd() = 0;

	virtual int32 MoveToPrevious() = 0;
	virtual int32 MoveToNext() = 0;
	virtual int32 MoveToCandidateBefore(const int32 InIndex) = 0;
	virtual int32 MoveToCandidateAfter(const int32 InIndex) = 0;
};
