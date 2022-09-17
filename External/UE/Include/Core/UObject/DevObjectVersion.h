// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "UObject/NameTypes.h"
#include "Misc/Guid.h"
#include "Serialization/CustomVersion.h"

class CORE_API FDevVersionRegistration :  public FCustomVersionRegistration
{
public:
	/** @param InFriendlyName must be a string literal */
	template<int N>
	FDevVersionRegistration(FGuid InKey, int32 Version, const TCHAR(&InFriendlyName)[N], CustomVersionValidatorFunc InValidatorFunc = nullptr)
		: FCustomVersionRegistration(InKey, Version, InFriendlyName, InValidatorFunc)
	{
		RecordDevVersion(InKey);
	}

	/** Dumps all registered versions to log */
	static void DumpVersionsToLog();
private:
	static void RecordDevVersion(FGuid Key);
};
