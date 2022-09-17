// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/ContainersFwd.h"
#include "UObject/UObjectHierarchyFwd.h"

// Basic types
class FName;
class FExec;
class FArchive;
class FOutputDevice;
class FFeedbackContext;
struct FDateTime;
struct FGuid;

// Math
UE_DECLARE_LWC_TYPE(Sphere, 3);
UE_DECLARE_LWC_TYPE(Box2,, FBox2D);
UE_DECLARE_LWC_TYPE(Box, 3);
struct FColor;
struct FLinearColor;
UE_DECLARE_LWC_TYPE(Matrix, 44);
UE_DECLARE_LWC_TYPE(Plane, 4);
UE_DECLARE_LWC_TYPE(Quat, 4);
UE_DECLARE_LWC_TYPE(Rotator, 3);
UE_DECLARE_LWC_TYPE(Transform, 3);
UE_DECLARE_LWC_TYPE(Vector2,, FVector2D);
UE_DECLARE_LWC_TYPE(Vector, 3);
UE_DECLARE_LWC_TYPE(Vector4);
struct FIntPoint;
struct FIntRect;

namespace UE
{
namespace Math
{
template<typename T, typename TExtent = T>
struct TBoxSphereBounds;
}
}
using FBoxSphereBounds3f = UE::Math::TBoxSphereBounds<float, float>;
using FBoxSphereBounds3d = UE::Math::TBoxSphereBounds<double, double>;
// FCompactBoxSphereBounds always stores float extents
using FCompactBoxSphereBounds3d = UE::Math::TBoxSphereBounds<double, float>;

using FBoxSphereBounds = FBoxSphereBounds3d;
using FCompactBoxSphereBounds = FCompactBoxSphereBounds3d;

// Misc
struct FResourceSizeEx;
class IConsoleVariable;
class FRunnableThread;
class FEvent;
class IPlatformFile;
class FMalloc;
class IFileHandle;
class FAutomationTestBase;
struct FGenericMemoryStats;
class FSHAHash;
class FScriptArray;
class FThreadSafeCounter;
enum class EModuleChangeReason;
struct FManifestContext;
class IConsoleObject;
class FConfigFile;
class FConfigSection;

// Text
class FText;
class FTextFilterString;
enum class ETextFilterTextComparisonMode : uint8;
enum class ETextFilterComparisonOperation : uint8;
