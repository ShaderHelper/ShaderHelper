// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Framework/Text/SlateTextLayout.h"

DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<FSlateTextLayout>, FCreateSlateTextLayout, SWidget*, const FTextBlockStyle&);
