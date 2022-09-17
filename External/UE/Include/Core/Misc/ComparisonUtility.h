// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/NameTypes.h"
#include "Containers/StringView.h"

namespace UE { namespace ComparisonUtility {

/** Compare the two names, correctly ordering any numeric suffixes they may have */
CORE_API int32 CompareWithNumericSuffix(FName A, FName B);

/** Compare the two strings, correctly ordering any numeric suffixes they may have */
CORE_API int32 CompareWithNumericSuffix(FStringView A, FStringView B);

} } // namespace UE::ComparisonUtility
