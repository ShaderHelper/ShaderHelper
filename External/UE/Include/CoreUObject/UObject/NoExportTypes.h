// Copyright Epic Games, Inc. All Rights Reserved.

// Reflection mirrors of C++ structs defined in Core or CoreUObject, those modules are not parsed by the Unreal Header Tool.
// The documentation comments here are only for use in the editor tooltips, and is ignored for the API docs.
// More complete documentation will be found in the files that have the full class definition, listed below.

#pragma once

#if CPP

// Include the real definitions of the noexport classes below to allow the generated cpp file to compile.

#include "PixelFormat.h"

#include "Misc/FallbackStruct.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"
#include "Misc/Timespan.h"

#include "UObject/SoftObjectPath.h"
#include "UObject/PropertyAccessUtil.h"

#include "Math/InterpCurvePoint.h"
#include "Math/UnitConversion.h"
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Vector2D.h"
#include "Math/TwoVectors.h"
#include "Math/Plane.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "Math/IntPoint.h"
#include "Math/IntVector.h"
#include "Math/Color.h"
#include "Math/Box.h"
#include "Math/Box2D.h"
#include "Math/BoxSphereBounds.h"
#include "Math/OrientedBox.h"
#include "Math/Matrix.h"
#include "Math/ScalarRegister.h"
#include "Math/RandomStream.h"
#include "Math/RangeBound.h"
#include "Math/Interval.h"

#include "Internationalization/PolyglotTextData.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetBundleData.h"
#include "AssetRegistry/AssetData.h"

#include "../../../ApplicationCore/Public/GenericPlatform/ICursor.h"

#endif

#if !CPP      //noexport class

/// @cond DOXYGEN_IGNORE

/**
 * Determines case sensitivity options for string comparisons. 
 * @note Mirrored from Engine\Source\Runtime\Core\Public\Containers\UnrealString.h
 */
UENUM()
namespace ESearchCase
{
	enum Type
	{
		CaseSensitive,
		IgnoreCase,
	};
}

/**
 * Determines search direction for string operations.
 * @note Mirrored from Engine\Source\Runtime\Core\Public\Containers\UnrealString.h
 */
UENUM()
namespace ESearchDir
{
	enum Type
	{
		FromStart,
		FromEnd,
	};
}

/**
 * Enum that defines how the log times are to be displayed.
 * @note Mirrored from Engine\Source\Runtime\Core\Public\Misc\OutputDevice.h
 */
UENUM()
namespace ELogTimes
{
	enum Type
	{
		/** Do not display log timestamps. */
		None UMETA(DisplayName = "None"),

		/** Display log timestamps in UTC. */
		UTC UMETA(DisplayName = "UTC"),

		/** Display log timestamps in seconds elapsed since GStartTime. */
		SinceGStartTime UMETA(DisplayName = "Time since application start"),

		/** Display log timestamps in local time. */
		Local UMETA(DisplayName = "Local time"),
	};
}

/** Generic axis enum (mirrored for native use in Axis.h). */
UENUM(meta=(ScriptName="AxisType"))
namespace EAxis
{
	enum Type
	{
		None,
		X,
		Y,
		Z
	};
}

/** Describes shape of an interpolation curve (mirrored from InterpCurvePoint.h). */
UENUM()
enum EInterpCurveMode
{
	/** A straight line between two keypoint values. */
	CIM_Linear UMETA(DisplayName="Linear"),
	
	/** A cubic-hermite curve between two keypoints, using Arrive/Leave tangents. These tangents will be automatically
		updated when points are moved, etc.  Tangents are unclamped and will plateau at curve start and end points. */
	CIM_CurveAuto UMETA(DisplayName="Curve Auto"),
	
	/** The out value is held constant until the next key, then will jump to that value. */
	CIM_Constant UMETA(DisplayName="Constant"),
	
	/** A smooth curve just like CIM_Curve, but tangents are not automatically updated so you can have manual control over them (eg. in Curve Editor). */
	CIM_CurveUser UMETA(DisplayName="Curve User"),
	
	/** A curve like CIM_Curve, but the arrive and leave tangents are not forced to be the same, so you can create a 'corner' at this key. */
	CIM_CurveBreak UMETA(DisplayName="Curve Break"),
	
	/** A cubic-hermite curve between two keypoints, using Arrive/Leave tangents. These tangents will be automatically
	    updated when points are moved, etc.  Tangents are clamped and will plateau at curve start and end points. */
	CIM_CurveAutoClamped UMETA(DisplayName="Curve Auto Clamped"),
};

/**
 * Describes the format of a each pixel in a graphics buffer.
 * @warning: When you update this, you must add an entry to GPixelFormats(see RenderUtils.cpp)
 * @warning: When you update this, you must add an entries to PixelFormat.h, usually just copy the generated section on the header into EPixelFormat
 * @warning: The *Tools DLLs will also need to be recompiled if the ordering is changed, but should not need code changes.
 */
UENUM()
enum EPixelFormat
{
	PF_Unknown,
	PF_A32B32G32R32F,
	/** UNORM (0..1), corresponds to FColor.  Unpacks as rgba in the shader. */
	PF_B8G8R8A8,
	/** UNORM red (0..1) */
	PF_G8,
	PF_G16,
	PF_DXT1,
	PF_DXT3,
	PF_DXT5,
	PF_UYVY,
	/** Same as PF_FloatR11G11B10 */
	PF_FloatRGB,
	/** RGBA 16 bit signed FP format.  Use FFloat16Color on the CPU. */
	PF_FloatRGBA,
	/** A depth+stencil format with platform-specific implementation, for use with render targets. */
	PF_DepthStencil,
	/** A depth format with platform-specific implementation, for use with render targets. */
	PF_ShadowDepth,
	PF_R32_FLOAT,
	PF_G16R16,
	PF_G16R16F,
	PF_G16R16F_FILTER,
	PF_G32R32F,
	PF_A2B10G10R10,
	PF_A16B16G16R16,
	PF_D24,
	PF_R16F,
	PF_R16F_FILTER,
	PF_BC5,
	/** SNORM red, green (-1..1). Not supported on all RHI e.g. Metal */
	PF_V8U8,
	PF_A1,
	/** A low precision floating point format, unsigned.  Use FFloat3Packed on the CPU. */
	PF_FloatR11G11B10,
	PF_A8,
	PF_R32_UINT,
	PF_R32_SINT,
	PF_PVRTC2,
	PF_PVRTC4,
	PF_R16_UINT,
	PF_R16_SINT,
	PF_R16G16B16A16_UINT,
	PF_R16G16B16A16_SINT,
	PF_R5G6B5_UNORM,
	PF_R8G8B8A8,
	/** Only used for legacy loading; do NOT use! */
	PF_A8R8G8B8,
	/** High precision single channel block compressed, equivalent to a single channel BC5, 8 bytes per 4x4 block. */
	PF_BC4,
	/** UNORM red, green (0..1). */
	PF_R8G8,
	/** ATITC format. */
	PF_ATC_RGB,
	/** ATITC format. */
	PF_ATC_RGBA_E,
	/** ATITC format. */
	PF_ATC_RGBA_I,
	/** Used for creating SRVs to alias a DepthStencil buffer to read Stencil.  Don't use for creating textures. */
	PF_X24_G8,
	PF_ETC1,
	PF_ETC2_RGB,
	PF_ETC2_RGBA,
	PF_R32G32B32A32_UINT,
	PF_R16G16_UINT,
	/** 8.00 bpp */
	PF_ASTC_4x4,
	/** 3.56 bpp */
	PF_ASTC_6x6,
	/** 2.00 bpp */
	PF_ASTC_8x8,
	/** 1.28 bpp */
	PF_ASTC_10x10,
	/** 0.89 bpp */
	PF_ASTC_12x12,
	PF_BC6H,
	PF_BC7,
	PF_R8_UINT,
	PF_L8,
	PF_XGXR8,
	PF_R8G8B8A8_UINT,
	/** SNORM (-1..1), corresponds to FFixedRGBASigned8. */
	PF_R8G8B8A8_SNORM,
	PF_R16G16B16A16_UNORM,
	PF_R16G16B16A16_SNORM,
	PF_PLATFORM_HDR_0,
	PF_PLATFORM_HDR_1,
	PF_PLATFORM_HDR_2,
	PF_NV12,
	PF_R32G32_UINT,
	PF_ETC2_R11_EAC,
	PF_ETC2_RG11_EAC,
	PF_R8,
	PF_B5G5R5A1_UNORM,
	PF_G16R16_SNORM,
	PF_R8G8_UINT,
	PF_R32G32B32_UINT,
	PF_R32G32B32_SINT,
	PF_R32G32B32F,
	PF_R8_SINT,
	PF_R64_UINT,
	PF_MAX,
};

/** Mouse cursor types (mirrored from ICursor.h) */
UENUM()
namespace EMouseCursor
{
	enum Type
	{
		/** Causes no mouse cursor to be visible. */
		None,

		/** Default cursor (arrow). */
		Default,

		/** Text edit beam. */
		TextEditBeam,

		/** Resize horizontal. */
		ResizeLeftRight,

		/** Resize vertical. */
		ResizeUpDown,

		/** Resize diagonal. */
		ResizeSouthEast,

		/** Resize other diagonal. */
		ResizeSouthWest,

		/** MoveItem. */
		CardinalCross,

		/** Target Cross. */
		Crosshairs,

		/** Hand cursor. */
		Hand,

		/** Grab Hand cursor. */
		GrabHand,

		/** Grab Hand cursor closed. */
		GrabHandClosed,

		/** a circle with a diagonal line through it. */
		SlashedCircle,

		/** Eye-dropper cursor for picking colors. */
		EyeDropper,
	};
}

/** A set of numerical unit types supported by the engine. Mirrored from UnitConversion.h */
UENUM(BlueprintType)
enum class EUnit : uint8
{
	/** Scalar distance/length unit. */
	Micrometers, Millimeters, Centimeters, Meters, Kilometers, Inches, Feet, Yards, Miles, Lightyears,
	
	/** Angular units */
	Degrees, Radians,
	
	/** Speed units */
	CentimetersPerSecond, MetersPerSecond, KilometersPerHour, MilesPerHour,
	
	/** Temperature units */
	Celsius, Farenheit, Kelvin,
	
	/** Mass units */
	Micrograms, Milligrams, Grams, Kilograms, MetricTons, Ounces, Pounds, Stones,
	
	/** Force units */
	Newtons, PoundsForce, KilogramsForce,
	
	/** Frequency units */
	Hertz, Kilohertz, Megahertz, Gigahertz, RevolutionsPerMinute,
	
	/** Data Size units */	
	Bytes, Kilobytes, Megabytes, Gigabytes, Terabytes,
	
	/** Luminous flux units */	
	Lumens,
	
	/** Time units */	
	Milliseconds, Seconds, Minutes, Hours, Days, Months, Years,

	/** Arbitrary multiplier */	
	Multiplier,


	/** Percentage */
	Percentage,

	/** Symbolic entry, not specifiable on meta data. */
	Unspecified
};

/**
 * Enum controlling when to emit property change notifications when setting a property value.
 * @note Mirrored from PropertyAccessUtil.h
 */
UENUM(BlueprintType)
enum class EPropertyAccessChangeNotifyMode : uint8
{
	/** Notify only when a value change has actually occurred */
	Default,
	/** Never notify that a value change has occurred */
	Never,
	/** Always notify that a value change has occurred, even if the value is unchanged */
	Always,
};

/**
* Enum denoting message dialog return types.
* @note Mirrored from GenericPlatformMisc.h
*/
UENUM(BlueprintType)
namespace EAppReturnType
{
	enum Type
	{
		No,
		Yes,
		YesAll,
		NoAll,
		Cancel,
		Ok,
		Retry,
		Continue,
	};
}

/**
* Enum denoting message dialog button choices. Used in combination with EAppReturnType.
* @note Mirrored from GenericPlatformMisc.h
*/
UENUM(BlueprintType)
namespace EAppMsgType
{
	/**
	 * Enumerates supported message dialog button types.
	 */
	enum Type
	{
		Ok,
		YesNo,
		OkCancel,
		YesNoCancel,
		CancelRetryContinue,
		YesNoYesAllNoAll,
		YesNoYesAllNoAllCancel,
		YesNoYesAll,
	};
}

/**
 * A struct used as stub for deleted ones. 
 */
USTRUCT(noexport)
struct FFallbackStruct
{
};

/** A globally unique identifier (mirrored from Guid.h) */
USTRUCT(immutable, noexport, BlueprintType)
struct FGuid
{
	UPROPERTY(EditAnywhere, SaveGame, Category=Guid)
	int32 A;

	UPROPERTY(EditAnywhere, SaveGame, Category=Guid)
	int32 B;

	UPROPERTY(EditAnywhere, SaveGame, Category=Guid)
	int32 C;

	UPROPERTY(EditAnywhere, SaveGame, Category=Guid)
	int32 D;
};

/**
 * A point or direction FVector in 3d space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector.h
 */
USTRUCT(immutable, noexport, BlueprintType, meta = (HasNativeBreak = "Engine.KismetMathLibrary.BreakVector3f"))
struct FVector3f
{
	UPROPERTY(EditAnywhere, Category = Vector, SaveGame)
	float X;

	UPROPERTY(EditAnywhere, Category = Vector, SaveGame)
	float Y;

	UPROPERTY(EditAnywhere, Category = Vector, SaveGame)
	float Z;
};

/**
 * A point or direction FVector in 3d space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector.h
 */
USTRUCT(immutable, noexport)
struct FVector3d
{
	UPROPERTY(EditAnywhere, Category = Vector, SaveGame)
	double X;

	UPROPERTY(EditAnywhere, Category = Vector, SaveGame)
	double Y;

	UPROPERTY(EditAnywhere, Category = Vector, SaveGame)
	double Z;
};

/**
 * A point or direction FVector in 3d space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector.h
 */
USTRUCT(immutable, noexport, BlueprintType, meta = (HasNativeMake = "Engine.KismetMathLibrary.MakeVector", HasNativeBreak = "Engine.KismetMathLibrary.BreakVector"))
struct FVector
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vector, SaveGame)
	FLargeWorldCoordinatesReal X;		// Alias for float/double depending on LWC status. Note: Will be refactored to double before UE5 ships.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vector, SaveGame)
	FLargeWorldCoordinatesReal Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vector, SaveGame)
	FLargeWorldCoordinatesReal Z;
};


/**
* A 4-D homogeneous vector.
* @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector4.h
*/
USTRUCT(immutable, noexport)
struct FVector4f
{
	UPROPERTY(EditAnywhere, Category = Vector4, SaveGame)
	float X;

	UPROPERTY(EditAnywhere, Category = Vector4, SaveGame)
	float Y;

	UPROPERTY(EditAnywhere, Category = Vector4, SaveGame)
	float Z;

	UPROPERTY(EditAnywhere, Category = Vector4, SaveGame)
	float W;
};

/**
* A 4-D homogeneous vector.
* @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector4.h
*/
USTRUCT(immutable, noexport)
struct FVector4d
{
	UPROPERTY(EditAnywhere, Category = Vector4, SaveGame)
	double X;

	UPROPERTY(EditAnywhere, Category = Vector4, SaveGame)
	double Y;

	UPROPERTY(EditAnywhere, Category = Vector4, SaveGame)
	double Z;

	UPROPERTY(EditAnywhere, Category = Vector4, SaveGame)
	double W;
};


/**
* A 4-D homogeneous vector.
* @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector4.h
*/
USTRUCT(immutable, noexport, BlueprintType, meta = (HasNativeMake = "Engine.KismetMathLibrary.MakeVector4", HasNativeBreak = "Engine.KismetMathLibrary.BreakVector4"))
struct FVector4
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector4, SaveGame)
	FLargeWorldCoordinatesReal X;		// Alias for float/double depending on LWC status. Note: Will be refactored to double before UE5 ships.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector4, SaveGame)
	FLargeWorldCoordinatesReal Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector4, SaveGame)
	FLargeWorldCoordinatesReal Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector4, SaveGame)
	FLargeWorldCoordinatesReal W;
};


/**
* A vector in 2-D space composed of components (X, Y) with floating point precision.
* @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector2D.h
*/
USTRUCT(immutable, noexport)
struct FVector2f
{
	UPROPERTY(EditAnywhere, Category=Vector2D, SaveGame)
	float X;

	UPROPERTY(EditAnywhere, Category=Vector2D, SaveGame)
	float Y;
};

/**
* A vector in 2-D space composed of components (X, Y) with floating point precision.
* @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector2D.h
*/
// LWC_TODO: CRITICAL! Name collision in UHT with FVector2D due to case insensitive FNames!
// USTRUCT(immutable, noexport)
// struct FVector2d
// {
// 	UPROPERTY(EditAnywhere, Category=Vector2D, SaveGame)
// 	double X;
//
// 	UPROPERTY(EditAnywhere, Category=Vector2D, SaveGame)
// 	double Y;
// };

/**
 * A vector in 2-D space composed of components (X, Y) with floating point precision.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Vector2D.h
 */
USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetMathLibrary.MakeVector2D", HasNativeBreak="Engine.KismetMathLibrary.BreakVector2D"))
struct FVector2D
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector2D, SaveGame)
	FLargeWorldCoordinatesReal X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vector2D, SaveGame)
	FLargeWorldCoordinatesReal Y;
};

/** A pair of 3D vectors (mirrored from TwoVectors.h). */
USTRUCT(immutable, BlueprintType, noexport)
struct FTwoVectors
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TwoVectors, SaveGame)
	FVector v1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TwoVectors, SaveGame)
	FVector v2;
};

/**
 * A plane definition in 3D space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Plane.h
 */
USTRUCT(immutable, noexport)
struct FPlane4f : public FVector3f
{
	UPROPERTY(EditAnywhere, Category=Plane, SaveGame)
	float W;
};

/**
 * A plane definition in 3D space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Plane.h
 */
USTRUCT(immutable, noexport)
struct FPlane4d : public FVector3d
{
	UPROPERTY(EditAnywhere, Category = Plane, SaveGame)
	double W;
};

/**
 * A plane definition in 3D space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Plane.h
 */
USTRUCT(immutable, noexport, BlueprintType)
struct FPlane : public FVector
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Plane, SaveGame)
	FLargeWorldCoordinatesReal W;
};


/**
 * An orthogonal rotation in 3d space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Rotator.h
 */
USTRUCT(immutable, noexport)
struct FRotator3f
{
	/** Pitch (degrees) around Y axis */
	UPROPERTY(EditAnywhere, Category=Rotator, SaveGame, meta=(DisplayName="Y"))
	float Pitch;

	/** Yaw (degrees) around Z axis */
	UPROPERTY(EditAnywhere, Category=Rotator, SaveGame, meta=(DisplayName="Z"))
	float Yaw;

	/** Roll (degrees) around X axis */
	UPROPERTY(EditAnywhere, Category=Rotator, SaveGame, meta=(DisplayName="X"))
	float Roll;
};

/**
 * An orthogonal rotation in 3d space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Rotator.h
 */
USTRUCT(immutable, noexport)
struct FRotator3d
{
	/** Pitch (degrees) around Y axis */
	UPROPERTY(EditAnywhere, Category=Rotator, SaveGame, meta=(DisplayName="Y"))
	double Pitch;

	/** Yaw (degrees) around Z axis */
	UPROPERTY(EditAnywhere, Category=Rotator, SaveGame, meta=(DisplayName="Z"))
	double Yaw;

	/** Roll (degrees) around X axis */
	UPROPERTY(EditAnywhere, Category=Rotator, SaveGame, meta=(DisplayName="X"))
	double Roll;
};

/**
 * An orthogonal rotation in 3d space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Rotator.h
 */
USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetMathLibrary.MakeRotator", HasNativeBreak="Engine.KismetMathLibrary.BreakRotator"))
struct FRotator
{
	/** Pitch (degrees) around Y axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rotator, SaveGame, meta=(DisplayName="Y"))
	FLargeWorldCoordinatesReal Pitch;

	/** Yaw (degrees) around Z axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rotator, SaveGame, meta=(DisplayName="Z"))
	FLargeWorldCoordinatesReal Yaw;

	/** Roll (degrees) around X axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rotator, SaveGame, meta=(DisplayName="X"))
	FLargeWorldCoordinatesReal Roll;
};

/**
 * Quaternion.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Quat.h
 */
USTRUCT(immutable, noexport)
struct FQuat4f
{
	UPROPERTY(EditAnywhere, Category=Quat, SaveGame)
	float X;

	UPROPERTY(EditAnywhere, Category=Quat, SaveGame)
	float Y;

	UPROPERTY(EditAnywhere, Category=Quat, SaveGame)
	float Z;

	UPROPERTY(EditAnywhere, Category=Quat, SaveGame)
	float W;

};


/**
 * Quaternion.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Quat.h
 */
USTRUCT(immutable, noexport)
struct FQuat4d
{
	UPROPERTY(EditAnywhere, Category = Quat, SaveGame)
	double X;

	UPROPERTY(EditAnywhere, Category = Quat, SaveGame)
	double Y;

	UPROPERTY(EditAnywhere, Category = Quat, SaveGame)
	double Z;

	UPROPERTY(EditAnywhere, Category = Quat, SaveGame)
	double W;

};


/**
 * Quaternion.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Quat.h
 */
USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMake ="Engine.KismetMathLibrary.MakeQuat", HasNativeBreak="Engine.KismetMathLibrary.BreakQuat"))
struct FQuat
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Quat, SaveGame)
	FLargeWorldCoordinatesReal X;		// Alias for float/double depending on LWC status. Note: Will be refactored to double before UE5 ships.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Quat, SaveGame)
	FLargeWorldCoordinatesReal Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Quat, SaveGame)
	FLargeWorldCoordinatesReal Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Quat, SaveGame)
	FLargeWorldCoordinatesReal W;
};


/**
 * A packed normal.
 * @note The full C++ class is located here: Engine\Source\Runtime\RenderCore\Public\PackedNormal.h
 */
USTRUCT(immutable, noexport)
struct FPackedNormal
{
	UPROPERTY(EditAnywhere, Category=PackedNormal, SaveGame)
	uint8 X;

	UPROPERTY(EditAnywhere, Category=PackedNormal, SaveGame)
	uint8 Y;

	UPROPERTY(EditAnywhere, Category=PackedNormal, SaveGame)
	uint8 Z;

	UPROPERTY(EditAnywhere, Category=PackedNormal, SaveGame)
	uint8 W;

};

/**
 * A packed basis vector.
 * @note The full C++ class is located here: Engine\Source\Runtime\RenderCore\Public\PackedNormal.h
 */
USTRUCT(immutable, noexport)
struct FPackedRGB10A2N
{
	UPROPERTY(EditAnywhere, Category = PackedBasis, SaveGame)
	int32 Packed;
};

/**
 * A packed vector.
 * @note The full C++ class is located here: Engine\Source\Runtime\RenderCore\Public\PackedNormal.h
 */
USTRUCT(immutable, noexport)
struct FPackedRGBA16N
{
	UPROPERTY(EditAnywhere, Category = PackedNormal, SaveGame)
	int32 XY;

	UPROPERTY(EditAnywhere, Category = PackedNormal, SaveGame)
	int32 ZW;
};

/**
 * Screen coordinates.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\IntPoint.h
 */
USTRUCT(immutable, noexport, BlueprintType)
struct FIntPoint
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IntPoint, SaveGame)
	int32 X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IntPoint, SaveGame)
	int32 Y;

};

/**
 * An integer vector in 3D space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\IntVector.h
 */
USTRUCT(immutable, noexport, BlueprintType)
struct FIntVector
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IntVector, SaveGame)
	int32 X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IntVector, SaveGame)
	int32 Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IntVector, SaveGame)
	int32 Z;
};

/**
 * Stores a color with 8 bits of precision per channel. (BGRA).
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Color.h
 */
USTRUCT(immutable, noexport, BlueprintType)
struct FColor
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Color, SaveGame, meta=(ClampMin="0", ClampMax="255"))
	uint8 B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Color, SaveGame, meta=(ClampMin="0", ClampMax="255"))
	uint8 G;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Color, SaveGame, meta=(ClampMin="0", ClampMax="255"))
	uint8 R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Color, SaveGame, meta=(ClampMin="0", ClampMax="255"))
	uint8 A;

};

/**
 * A linear, 32-bit/component floating point RGBA color.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Color.h
 */
USTRUCT(immutable, noexport, BlueprintType)
struct FLinearColor
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LinearColor, SaveGame)
	float R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LinearColor, SaveGame)
	float G;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LinearColor, SaveGame)
	float B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LinearColor, SaveGame)
	float A;

};

/**
 * A point or direction FVector in 3d space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Box.h
 */
USTRUCT(immutable, noexport)
struct FBox3f
{
	UPROPERTY(EditAnywhere, Category = Box, SaveGame)
	FVector3f Min;

	UPROPERTY(EditAnywhere, Category = Box, SaveGame)
	FVector3f Max;

	UPROPERTY()
	uint8 IsValid;
};


/**
 * A point or direction FVector in 3d space.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Box.h
 */
USTRUCT(immutable, noexport)
struct FBox3d
{
	UPROPERTY(EditAnywhere, Category = Box, SaveGame)
	FVector3d Min;

	UPROPERTY(EditAnywhere, Category = Box, SaveGame)
	FVector3d Max;

	UPROPERTY()
	uint8 IsValid;
};

/**
 * A bounding box.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Box.h
 */
USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetMathLibrary.MakeBox"))
struct FBox
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Box, SaveGame)
	FVector Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Box, SaveGame)
	FVector Max;

	UPROPERTY()
	uint8 IsValid;
};

/**
 * A rectangular 2D Box.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Box2D.h
 */
USTRUCT(immutable, noexport)
struct FBox2f
{
	UPROPERTY(EditAnywhere, Category=Box2D, SaveGame)
	FVector2f Min;

	UPROPERTY(EditAnywhere, Category=Box2D, SaveGame)
	FVector2f Max;

	UPROPERTY()
	uint8 bIsValid;
};

/**
* A rectangular 2D Box.
* @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Box2D.h
*/
// LWC_TODO: CRITICAL! Name collision in UHT with FBox2D due to case insensitive FNames!
// USTRUCT(immutable, noexport)
// struct FBox2d
// {
// 	UPROPERTY(EditAnywhere, Category=Box2D, SaveGame)
// 	FVector2d Min;
//
// 	UPROPERTY(EditAnywhere, Category=Box2D, SaveGame)
// 	FVector2d Max;
//
// 	UPROPERTY()
// 	uint8 bIsValid;
// };

/**
* A rectangular 2D Box.
* @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Box2D.h
*/
USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetMathLibrary.MakeBox2D"))
struct FBox2D
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Box2D, SaveGame)
	FVector2D Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Box2D, SaveGame)
	FVector2D Max;

	UPROPERTY()
	uint8 bIsValid;
};

/**
 * A bounding box and bounding sphere with the same origin.
 * @note The full C++ class is located here : Engine\Source\Runtime\Core\Public\Math\BoxSphereBounds.h
 */
USTRUCT(noexport)
struct FBoxSphereBounds3f
{
	/** Holds the origin of the bounding box and sphere. */
	UPROPERTY(EditAnywhere, Category = BoxSphereBounds, SaveGame)
	FVector3f Origin;

	/** Holds the extent of the bounding box, which is half the size of the box in 3D space */
	UPROPERTY(EditAnywhere, Category = BoxSphereBounds, SaveGame)
	FVector3f BoxExtent;

	/** Holds the radius of the bounding sphere. */
	UPROPERTY(EditAnywhere, Category = BoxSphereBounds, SaveGame)
	float SphereRadius;
};

/**
 * A bounding box and bounding sphere with the same origin.
 * @note The full C++ class is located here : Engine\Source\Runtime\Core\Public\Math\BoxSphereBounds.h
 */
USTRUCT(noexport)
struct FBoxSphereBounds3d
{
	/** Holds the origin of the bounding box and sphere. */
	UPROPERTY(EditAnywhere, Category = BoxSphereBounds, SaveGame)
	FVector3d Origin;

	/** Holds the extent of the bounding box, which is half the size of the box in 3D space */
	UPROPERTY(EditAnywhere, Category = BoxSphereBounds, SaveGame)
	FVector3d BoxExtent;

	/** Holds the radius of the bounding sphere. */
	UPROPERTY(EditAnywhere, Category = BoxSphereBounds, SaveGame)
	double SphereRadius;
};

/**
 * A bounding box and bounding sphere with the same origin.
 * @note The full C++ class is located here : Engine\Source\Runtime\Core\Public\Math\BoxSphereBounds.h
 */
USTRUCT(noexport, BlueprintType, meta = (HasNativeMake = "Engine.KismetMathLibrary.MakeBoxSphereBounds", HasNativeBreak = "Engine.KismetMathLibrary.BreakBoxSphereBounds"))
struct FBoxSphereBounds
{
	/** Holds the origin of the bounding box and sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BoxSphereBounds, SaveGame)
	FVector Origin;

	/** Holds the extent of the bounding box, which is half the size of the box in 3D space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BoxSphereBounds, SaveGame)
	FVector BoxExtent;

	/** Holds the radius of the bounding sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BoxSphereBounds, SaveGame)
	FLargeWorldCoordinatesReal SphereRadius;
};

/**
 * Structure for arbitrarily oriented boxes (i.e. not necessarily axis-aligned).
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\OrientedBox.h
 */
USTRUCT(immutable, noexport)
struct FOrientedBox
{
	/** Holds the center of the box. */
	UPROPERTY(EditAnywhere, Category=OrientedBox, SaveGame)
	FVector Center;

	/** Holds the x-axis vector of the box. Must be a unit vector. */
	UPROPERTY(EditAnywhere, Category=OrientedBox, SaveGame)
	FVector AxisX;
	
	/** Holds the y-axis vector of the box. Must be a unit vector. */
	UPROPERTY(EditAnywhere, Category=OrientedBox, SaveGame)
	FVector AxisY;
	
	/** Holds the z-axis vector of the box. Must be a unit vector. */
	UPROPERTY(EditAnywhere, Category=OrientedBox, SaveGame)
	FVector AxisZ;

	/** Holds the extent of the box along its x-axis. */
	UPROPERTY(EditAnywhere, Category=OrientedBox, SaveGame)
	FLargeWorldCoordinatesReal ExtentX;
	
	/** Holds the extent of the box along its y-axis. */
	UPROPERTY(EditAnywhere, Category=OrientedBox, SaveGame)
	FLargeWorldCoordinatesReal ExtentY;

	/** Holds the extent of the box along its z-axis. */
	UPROPERTY(EditAnywhere, Category=OrientedBox, SaveGame)
	FLargeWorldCoordinatesReal ExtentZ;
};

/**
 * A 4x4 matrix.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Matrix.h
 */
USTRUCT(immutable, noexport)
struct FMatrix44f
{
	UPROPERTY(EditAnywhere, Category=Matrix, SaveGame)
	FPlane4f XPlane;

	UPROPERTY(EditAnywhere, Category=Matrix, SaveGame)
	FPlane4f YPlane;

	UPROPERTY(EditAnywhere, Category=Matrix, SaveGame)
	FPlane4f ZPlane;

	UPROPERTY(EditAnywhere, Category=Matrix, SaveGame)
	FPlane4f WPlane;

};

/**
 * A 4x4 matrix.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Matrix.h
 */
USTRUCT(immutable, noexport)
struct FMatrix44d
{
	UPROPERTY(EditAnywhere, Category = Matrix, SaveGame)
	FPlane4d XPlane;

	UPROPERTY(EditAnywhere, Category = Matrix, SaveGame)
	FPlane4d YPlane;

	UPROPERTY(EditAnywhere, Category = Matrix, SaveGame)
	FPlane4d ZPlane;

	UPROPERTY(EditAnywhere, Category = Matrix, SaveGame)
	FPlane4d WPlane;

};

/**
 * A 4x4 matrix.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\Matrix.h
 */
USTRUCT(immutable, noexport, BlueprintType)
struct FMatrix
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Matrix, SaveGame)
	FPlane XPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Matrix, SaveGame)
	FPlane YPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Matrix, SaveGame)
	FPlane ZPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Matrix, SaveGame)
	FPlane WPlane;

};

/** 
 * Describes one specific point on an interpolation curve.
 * @note This is a mirror of TInterpCurvePoint<float>, defined in InterpCurvePoint.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurvePointFloat
{
	/** Float input value that corresponds to this key (eg. time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	float InVal;

	/** Float output value type when input is equal to InVal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	float OutVal;

	/** Tangent of curve arriving at this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	float ArriveTangent;

	/** Tangent of curve leaving this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	float LeaveTangent;

	/** Interpolation mode between this point and the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointFloat)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;

};

/** 
 * Describes an entire curve that is used to compute a float output value from a float input.
 * @note This is a mirror of TInterpCurve<float>, defined in InterpCurve.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurveFloat
{
	/** Holds the collection of interpolation points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveFloat)
	TArray<FInterpCurvePointFloat> Points;

	/** Specify whether the curve is looped or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveFloat)
	bool bIsLooped;

	/** Specify the offset from the last point's input key corresponding to the loop point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveFloat)
	float LoopKeyOffset;
};

/** 
 * Describes one specific point on an interpolation curve.
 * @note This is a mirror of TInterpCurvePoint<FVector2D>, defined in InterpCurvePoint.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurvePointVector2D
{
	/** Float input value that corresponds to this key (eg. time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	float InVal;

	/** 2D vector output value of when input is equal to InVal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	FVector2D OutVal;

	/** Tangent of curve arriving at this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	FVector2D ArriveTangent;

	/** Tangent of curve leaving this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	FVector2D LeaveTangent;

	/** Interpolation mode between this point and the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector2D)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;
};

/** 
 * Describes an entire curve that is used to compute a 2D vector output value from a float input.
 * @note This is a mirror of TInterpCurve<FVector2D>, defined in InterpCurve.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurveVector2D
{
	/** Holds the collection of interpolation points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveVector2D)
	TArray<FInterpCurvePointVector2D> Points;

	/** Specify whether the curve is looped or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveVector2D)
	bool bIsLooped;

	/** Specify the offset from the last point's input key corresponding to the loop point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveVector2D)
	float LoopKeyOffset;
};

/** 
 * Describes one specific point on an interpolation curve.
 * @note This is a mirror of TInterpCurvePoint<FVector>, defined in InterpCurvePoint.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurvePointVector
{
	/** Float input value that corresponds to this key (eg. time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	float InVal;

	/** 3D vector output value of when input is equal to InVal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	FVector OutVal;

	/** Tangent of curve arriving at this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	FVector ArriveTangent;

	/** Tangent of curve leaving this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	FVector LeaveTangent;

	/** Interpolation mode between this point and the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointVector)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;
};

/** 
 * Describes an entire curve that is used to compute a 3D vector output value from a float input.
 * @note This is a mirror of TInterpCurve<FVector>, defined in InterpCurve.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurveVector
{
	/** Holds the collection of interpolation points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveVector)
	TArray<FInterpCurvePointVector> Points;

	/** Specify whether the curve is looped or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveVector)
	bool bIsLooped;

	/** Specify the offset from the last point's input key corresponding to the loop point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveVector)
	float LoopKeyOffset;
};

/**
 * Describes one specific point on an interpolation curve.
 * @note This is a mirror of TInterpCurvePoint<FQuat>, defined in InterpCurvePoint.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurvePointQuat
{
	/** Float input value that corresponds to this key (eg. time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointQuat)
	float InVal;

	/** Quaternion output value of when input is equal to InVal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointQuat)
	FQuat OutVal;

	/** Tangent of curve arriving at this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointQuat)
	FQuat ArriveTangent;

	/** Tangent of curve leaving this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointQuat)
	FQuat LeaveTangent;

	/** Interpolation mode between this point and the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointQuat)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;
};

/**
 * Describes an entire curve that is used to compute a quaternion output value from a float input.
 * @note This is a mirror of TInterpCurve<FQuat>, defined in InterpCurve.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurveQuat
{
	/** Holds the collection of interpolation points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveQuat)
	TArray<FInterpCurvePointQuat> Points;

	/** Specify whether the curve is looped or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveQuat)
	bool bIsLooped;

	/** Specify the offset from the last point's input key corresponding to the loop point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveQuat)
	float LoopKeyOffset;
};

/**
 * Describes one specific point on an interpolation curve.
 * @note This is a mirror of TInterpCurvePoint<FTwoVectors>, defined in InterpCurvePoint.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurvePointTwoVectors
{
	/** Float input value that corresponds to this key (eg. time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	float InVal;

	/** Two 3D vectors output value of when input is equal to InVal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	FTwoVectors OutVal;

	/** Tangent of curve arriving at this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	FTwoVectors ArriveTangent;

	/** Tangent of curve leaving this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	FTwoVectors LeaveTangent;

	/** Interpolation mode between this point and the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointTwoVectors)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;
};

/**
 * Describes an entire curve that is used to compute two 3D vector values from a float input.
 * @note This is a mirror of TInterpCurve<FTwoVectors>, defined in InterpCurve.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurveTwoVectors
{
	/** Holds the collection of interpolation points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveTwoVectors)
	TArray<FInterpCurvePointTwoVectors> Points;

	/** Specify whether the curve is looped or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveTwoVectors)
	bool bIsLooped;

	/** Specify the offset from the last point's input key corresponding to the loop point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveTwoVectors)
	float LoopKeyOffset;
};

/**
 * Describes one specific point on an interpolation curve.
 * @note This is a mirror of TInterpCurvePoint<FLinearColor>, defined in InterpCurvePoint.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurvePointLinearColor
{
	/** Float input value that corresponds to this key (eg. time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	float InVal;

	/** Color output value of when input is equal to InVal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	FLinearColor OutVal;

	/** Tangent of curve arriving at this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	FLinearColor ArriveTangent;

	/** Tangent of curve leaving this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	FLinearColor LeaveTangent;

	/** Interpolation mode between this point and the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurvePointLinearColor)
	TEnumAsByte<enum EInterpCurveMode> InterpMode;
};

/**
 * Describes an entire curve that is used to compute a color output value from a float input.
 * @note This is a mirror of TInterpCurve<FLinearColor>, defined in InterpCurve.h
 */
USTRUCT(noexport, BlueprintType)
struct FInterpCurveLinearColor
{
	/** Holds the collection of interpolation points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveLinearColor)
	TArray<FInterpCurvePointLinearColor> Points;

	/** Specify whether the curve is looped or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveLinearColor)
	bool bIsLooped;

	/** Specify the offset from the last point's input key corresponding to the loop point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InterpCurveLinearColor)
	float LoopKeyOffset;
};

/**
 * Transform composed of Quat/Translation/Scale.
 * @note This is implemented in either TransformVectorized.h or TransformNonVectorized.h depending on the platform.
 */
USTRUCT(immutable, noexport)
struct FTransform3f
{
	/** Rotation of this transformation, as a quaternion. */
	UPROPERTY(EditAnywhere, Category = Transform, SaveGame)
	FQuat4f Rotation;

	/** Translation of this transformation, as a vector. */
	UPROPERTY(EditAnywhere, Category = Transform, SaveGame)
	FVector3f Translation;

	/** 3D scale (always applied in local space) as a vector. */
	UPROPERTY(EditAnywhere, Category = Transform, SaveGame)
	FVector3f Scale3D;
};

/**
 * Transform composed of Quat/Translation/Scale.
 * @note This is implemented in either TransformVectorized.h or TransformNonVectorized.h depending on the platform.
 */
USTRUCT(immutable, noexport)
struct FTransform3d
{
	/** Rotation of this transformation, as a quaternion. */
	UPROPERTY(EditAnywhere, Category = Transform, SaveGame)
	FQuat4d Rotation;

	/** Translation of this transformation, as a vector. */
	UPROPERTY(EditAnywhere, Category = Transform, SaveGame)
	FVector3d Translation;

	/** 3D scale (always applied in local space) as a vector. */
	UPROPERTY(EditAnywhere, Category = Transform, SaveGame)
	FVector3d Scale3D;
};

/**
 * Transform composed of Quat/Translation/Scale.
 * @note This is implemented in either TransformVectorized.h or TransformNonVectorized.h depending on the platform.
 */
USTRUCT(noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetMathLibrary.MakeTransform", HasNativeBreak="Engine.KismetMathLibrary.BreakTransform"))
struct FTransform
{
	/** Rotation of this transformation, as a quaternion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transform, SaveGame)
	FQuat Rotation;

	/** Translation of this transformation, as a vector. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transform, SaveGame)
	FVector Translation;

	/** 3D scale (always applied in local space) as a vector. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transform, SaveGame)
	FVector Scale3D;
};

/**
 * Thread-safe random number generator that can be manually seeded.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Math\RandomStream.h
 */
USTRUCT(noexport, BlueprintType, meta = (HasNativeMake = "Engine.KismetMathLibrary.MakeRandomStream", HasNativeBreak = "Engine.KismetMathLibrary.BreakRandomStream"))
struct FRandomStream
{
public:
	/** Holds the initial seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RandomStream, SaveGame)
	int32 InitialSeed;
	
	/** Holds the current seed. */
	UPROPERTY()
	int32 Seed;
};

/** 
 * A value representing a specific point date and time over a wide range of years.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Misc\DateTime.h
 */
USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetMathLibrary.MakeDateTime", HasNativeBreak="Engine.KismetMathLibrary.BreakDateTime"))
struct FDateTime
{
	int64 Ticks;
};

/** 
 * A frame number value, representing discrete frames since the start of timing.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Misc\FrameNumber.h
 */
USTRUCT(noexport, BlueprintType)
struct FFrameNumber
{
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=FrameNumber)
	int32 Value;
};

/** 
 * A frame rate represented as a fraction comprising 2 integers: a numerator (number of frames), and a denominator (per second).
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Misc\FrameRate.h
 */
USTRUCT(noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetMathLibrary.MakeFrameRate", HasNativeBreak="Engine.KismetMathLibrary.BreakFrameRate"))
struct FFrameRate
{
	/** The numerator of the framerate represented as a number of frames per second (e.g. 60 for 60 fps) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=FrameRate)
	int32 Numerator;

	/** The denominator of the framerate represented as a number of frames per second (e.g. 1 for 60 fps) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=FrameRate)
	int32 Denominator;
};

/** 
 * Represents a time by a context-free frame number, plus a sub frame value in the range [0:1). 
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Misc\FrameTime.h
 * @note The 'SubFrame' field is private to match its C++ class declaration in the header above.
 */
USTRUCT(noexport, BlueprintType)
struct FFrameTime
{
	/** Count of frames from start of timing */
	UPROPERTY(BlueprintReadWrite, Category=FrameTime)
	FFrameNumber FrameNumber;
	
private:
	/** Time within a frame, always between >= 0 and < 1 */
	UPROPERTY(BlueprintReadWrite, Category=FrameTime, meta=(AllowPrivateAccess="true"))
	float SubFrame;
};

/** 
 * A frame time qualified by a frame rate context.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Misc\QualifiedFrameTime.h
 */
USTRUCT(noexport, BlueprintType, meta=(ScriptName="QualifiedTime", HasNativeMake="Engine.KismetMathLibrary.MakeQualifiedFrameTime", HasNativeBreak="Engine.KismetMathLibrary.BreakQualifiedFrameTime"))
struct FQualifiedFrameTime
{
	/** The frame time */
	UPROPERTY(BlueprintReadWrite, Category=QualifiedFrameTime)
	FFrameTime Time;

	/** The rate that this frame time is in */
	UPROPERTY(BlueprintReadWrite, Category=QualifiedFrameTime)
	FFrameRate Rate;
};

/** 
 * A timecode that stores time in HH:MM:SS format with the remainder of time represented by an integer frame count. 
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Misc\TimeCode.h
 */
USTRUCT(noexport, BlueprintType)
struct FTimecode
{
	UPROPERTY(BlueprintReadWrite, Category=Timecode)
	int32 Hours;

	UPROPERTY(BlueprintReadWrite, Category=Timecode)
	int32 Minutes;

	UPROPERTY(BlueprintReadWrite, Category=Timecode)
	int32 Seconds;

	UPROPERTY(BlueprintReadWrite, Category=Timecode)
	int32 Frames;

	/** If true, this Timecode represents a Drop Frame timecode used to account for fractional frame rates in NTSC play rates. */
	UPROPERTY(BlueprintReadWrite, Category= Timecode)
	bool bDropFrameFormat;
};

/** 
 * A time span value, which is the difference between two dates and times.
 * @note The full C++ class is located here: Engine\Source\Runtime\Core\Public\Misc\Timespan.h
 */
USTRUCT(immutable, noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetMathLibrary.MakeTimespan", HasNativeBreak="Engine.KismetMathLibrary.BreakTimespan"))
struct FTimespan
{
	int64 Ticks;
};

/** 
 * A struct that contains a string reference to an object, either a top level asset or a subobject.
 * @note The full C++ class is located here: Engine\Source\Runtime\CoreUObject\Public\UObject\SoftObjectPath.h
 */
USTRUCT(noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetSystemLibrary.MakeSoftObjectPath", HasNativeBreak="Engine.KismetSystemLibrary.BreakSoftObjectPath"))
struct FSoftObjectPath
{
	/** Asset path, patch to a top level object in a package */
	UPROPERTY()
	FName AssetPathName;

	/** Optional FString for subobject within an asset */
	UPROPERTY()
	FString SubPathString;
};

/** 
 * A struct that contains a string reference to a class, can be used to make soft references to classes.
 * @note The full C++ class is located here: Engine\Source\Runtime\CoreUObject\Public\UObject\SoftObjectPath.h
 */
USTRUCT(noexport, BlueprintType, meta=(HasNativeMake="Engine.KismetSystemLibrary.MakeSoftClassPath", HasNativeBreak="Engine.KismetSystemLibrary.BreakSoftClassPath"))
struct FSoftClassPath : public FSoftObjectPath
{
};

/** 
 * A type of primary asset, used by the Asset Manager system.
 * @note The full C++ class is located here: Engine\Source\Runtime\CoreUObject\Public\UObject\PrimaryAssetId.h
 */
USTRUCT(noexport, BlueprintType)
struct FPrimaryAssetType
{
	/** The Type of this object, by default its base class's name */
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = PrimaryAssetType)
	FName Name;
};

/** 
 * This identifies an object as a "primary" asset that can be searched for by the AssetManager and used in various tools
 * @note The full C++ class is located here: Engine\Source\Runtime\CoreUObject\Public\UObject\PrimaryAssetId.h
 */
USTRUCT(noexport, BlueprintType)
struct FPrimaryAssetId
{
	/** The Type of this object, by default its base class's name */
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = PrimaryAssetId)
	FPrimaryAssetType PrimaryAssetType;

	/** The Name of this object, by default its short name */
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = PrimaryAssetId)
	FName PrimaryAssetName;
};

/** Enumerates the valid types of range bounds (mirrored from RangeBound.h) */
UENUM(BlueprintType)
namespace ERangeBoundTypes
{
	enum Type
	{
		/**
		* The range excludes the bound.
		*/
		Exclusive,

		/**
		* The range includes the bound.
		*/
		Inclusive,

		/**
		* The bound is open.
		*/
		Open
	};
}

/**
 * Defines a single bound for a range of values.
 * @note This is a mirror of TRangeBound<float>, defined in RangeBound.h
 * @note Fields are private to match the C++ declaration in the header above.
 */
USTRUCT(noexport, BlueprintType)
struct FFloatRangeBound
{
private:
	/** Holds the type of the bound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	TEnumAsByte<ERangeBoundTypes::Type> Type;

	/** Holds the bound's value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	float Value;
};

/**
 * A contiguous set of floats described by lower and upper bound values.
 * @note This is a mirror of TRange<float>, defined in Range.h
 * @note Fields are private to match the C++ declaration in the header above.
 */
USTRUCT(noexport, BlueprintType)
struct FFloatRange
{
private:
	/** Holds the range's lower bound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	FFloatRangeBound LowerBound;

	/** Holds the range's upper bound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	FFloatRangeBound UpperBound;
};

/**
 * Defines a single bound for a range of values.
 * @note This is a mirror of TRangeBound<int32>, defined in RangeBound.h
 * @note Fields are private to match the C++ declaration in the header above.
 */
USTRUCT(noexport, BlueprintType)
struct FInt32RangeBound
{
private:
	/** Holds the type of the bound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	TEnumAsByte<ERangeBoundTypes::Type> Type;

	/** Holds the bound's value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	int32 Value;
};

/**
 * A contiguous set of floats described by lower and upper bound values.
 * @note This is a mirror of TRange<int32>, defined in Range.h
 * @note Fields are private to match the C++ declaration in the header above.
 */
USTRUCT(noexport, BlueprintType)
struct FInt32Range
{
private:
	/** Holds the range's lower bound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	FInt32RangeBound LowerBound;

	/** Holds the range's upper bound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	FInt32RangeBound UpperBound;
};

/**
 * Defines a single bound for a range of frame numbers.
 * @note This is a mirror of TRangeBound<FFrameNumber>, defined in RangeBound.h
 * @note Fields are private to match the C++ declaration in the header above.
 */
USTRUCT(noexport, BlueprintType)
struct FFrameNumberRangeBound
{
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	TEnumAsByte<ERangeBoundTypes::Type> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	FFrameNumber Value;
};

/**
 * A contiguous set of frame numbers described by lower and upper bound values.
 * @note This is a mirror of TRange<FFrameNumber>, defined in Range.h
 * @note Fields are private to match the C++ declaration in the header above.
 */
USTRUCT(noexport, BlueprintType)
struct FFrameNumberRange
{
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	FFrameNumberRangeBound LowerBound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Range, meta=(AllowPrivateAccess="true"))
	FFrameNumberRangeBound UpperBound;
};

/**
 * An interval of floats, defined by inclusive min and max values
 * @note This is a mirror of TInterval<float>, defined in Interval.h
 */
USTRUCT(noexport, BlueprintType)
struct FFloatInterval
{
	/** Values must be >= Min */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Interval)
	float Min;

	/** Values must be <= Max */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Interval)
	float Max;
};

/**
 * An interval of integers, defined by inclusive min and max values
 * @note This is a mirror of TInterval<int32>, defined in Interval.h
 */
USTRUCT(noexport, BlueprintType)
struct FInt32Interval
{
	/** Values must be >= Min */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Interval)
	int32 Min;

	/** Values must be <= Max */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Interval)
	int32 Max;
};

/** Categories of localized text (mirrored in LocalizedTextSourceTypes.h */
UENUM(BlueprintType)
enum class ELocalizedTextSourceCategory : uint8
{
	Game,
	Engine,
	Editor,
};

/**
 * Polyglot data that may be registered to the text localization manager at runtime.
 * @note This struct is mirrored in PolyglotTextData.h
 */
USTRUCT(noexport, BlueprintType)
struct FPolyglotTextData
{
	/**
	 * The category of this polyglot data.
	 * @note This affects when and how the data is loaded into the text localization manager.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PolyglotData)
	ELocalizedTextSourceCategory Category;

	/**
	 * The native culture of this polyglot data.
	 * @note This may be empty, and if empty, will be inferred from the native culture of the text category.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PolyglotData)
	FString NativeCulture;

	/**
	 * The namespace of the text created from this polyglot data.
	 * @note This may be empty.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PolyglotData)
	FString Namespace;

	/**
	 * The key of the text created from this polyglot data.
	 * @note This must not be empty.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PolyglotData)
	FString Key;

	/**
	 * The native string for this polyglot data.
	 * @note This must not be empty (it should be the same as the originally authored text you are trying to replace).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PolyglotData)
	FString NativeString;

	/**
	 * Mapping between a culture code and its localized string.
	 * @note The native culture may also have a translation in this map.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PolyglotData)
	TMap<FString, FString> LocalizedStrings;

	/**
	 * True if this polyglot data is a minimal patch, and that missing translations should be
	 * ignored (falling back to any LocRes data) rather than falling back to the native string.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PolyglotData)
	bool bIsMinimalPatch;

	/**
	 * Transient cached text instance from registering this polyglot data with the text localization manager.
	 */
	UPROPERTY(Transient)
	FText CachedText;
};

/** Report level of automation events (mirrored in AutomationEvent.h). */
UENUM()
enum class EAutomationEventType : uint8
{
	Info,
	Warning,
	Error
};

/** Event emitted by automation system (mirrored in AutomationEvent.h). */
USTRUCT(noexport)
struct FAutomationEvent
{
	UPROPERTY()
	EAutomationEventType Type;

	UPROPERTY()
	FString Message;

	UPROPERTY()
	FString Context;

	UPROPERTY()
	FGuid Artifact;
};

/** Information about the execution of an automation task (mirrored in AutomationEvent.h). */
USTRUCT(noexport)
struct FAutomationExecutionEntry
{
	UPROPERTY()
	FAutomationEvent Event;

	UPROPERTY()
	FString Filename;

	UPROPERTY()
	int32 LineNumber;

	UPROPERTY()
	FDateTime Timestamp;
};

/** Enum used by DataValidation plugin to see if an asset has been validated for correctness (mirrored in UObjectGlobals.h)*/
UENUM(BlueprintType)
enum class EDataValidationResult : uint8
{
	/** Asset has failed validation */
	Invalid,
	/** Asset has passed validation */
	Valid,
	/** Asset has not yet been validated */
	NotValidated
};

USTRUCT(noexport, BlueprintType)
struct FARFilter
{
	/** The filter component for package names */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = AssetRegistry)
	TArray<FName> PackageNames;

	/** The filter component for package paths */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = AssetRegistry)
	TArray<FName> PackagePaths;

	/** The filter component containing specific object paths */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = AssetRegistry)
	TArray<FName> ObjectPaths;

	/** The filter component for class names. Instances of the specified classes, but not subclasses (by default), will be included. Derived classes will be included only if bRecursiveClasses is true. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = AssetRegistry)
	TArray<FName> ClassNames;

	/** The filter component for properties marked with the AssetRegistrySearchable flag */
	TMultiMap<FName, TOptional<FString>> TagsAndValues;

	/** Only if bRecursiveClasses is true, the results will exclude classes (and subclasses) in this list */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = AssetRegistry)
	TSet<FName> RecursiveClassesExclusionSet;

	/** If true, PackagePath components will be recursive */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = AssetRegistry)
	bool bRecursivePaths = false;

	/** If true, subclasses of ClassNames will also be included and RecursiveClassesExclusionSet will be excluded. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = AssetRegistry)
	bool bRecursiveClasses = false;

	/** If true, only on-disk assets will be returned. Be warned that this is rarely what you want and should only be used for performance reasons */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = AssetRegistry)
	bool bIncludeOnlyOnDiskAssets = false;

	/** The exclusive filter component for package flags. Only assets without any of the specified flags will be returned. */
	uint32 WithoutPackageFlags = 0;

	/** The inclusive filter component for package flags. Only assets with all of the specified flags will be returned. */
	uint32 WithPackageFlags = 0;
};

USTRUCT(noexport)
struct COREUOBJECT_API FAssetBundleEntry
{
	/** Specific name of this bundle */
	UPROPERTY()
	FName BundleName;

	/** List of string assets contained in this bundle */
	UPROPERTY()
	TArray<FSoftObjectPath> BundleAssets;
};

/** A struct with a list of asset bundle entries. If one of these is inside a UObject it will get automatically exported as the asset registry tag AssetBundleData */
USTRUCT(noexport)
struct COREUOBJECT_API FAssetBundleData
{
	/** List of bundles defined */
	UPROPERTY()
	TArray<FAssetBundleEntry> Bundles;
};

/**
 * A struct to hold important information about an assets found by the Asset Registry
 * This struct is transient and should never be serialized
 */
USTRUCT(noexport, BlueprintType)
struct FAssetData
{
	/** The object path for the asset in the form PackageName.AssetName. Only top level objects in a package can have AssetData */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
	FName ObjectPath;
	/** The name of the package in which the asset is found, this is the full long package name such as /Game/Path/Package */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
	FName PackageName;
	/** The path to the package in which the asset is found, this is /Game/Path with the Package stripped off */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
	FName PackagePath;
	/** The name of the asset without the package */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
	FName AssetName;
	/** The name of the asset's class */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
	FName AssetClass;
	/** The map of values for properties that were marked AssetRegistrySearchable or added by GetAssetRegistryTags */
	FAssetDataTagMapSharedView TagsAndValues;
	TSharedPtr<FAssetBundleData, ESPMode::ThreadSafe> TaggedAssetBundles;
	/** The IDs of the pakchunks this asset is located in for streaming install.  Empty if not assigned to a chunk */
	TArray<int32, TInlineAllocator<2>> ChunkIDs;
	/** Asset package flags */
	uint32 PackageFlags = 0;
};

USTRUCT(noexport)
struct FTestUninitializedScriptStructMembersTest
{
	UPROPERTY(Transient)
	TObjectPtr<UObject> UninitializedObjectReference;

	UPROPERTY(Transient)
	TObjectPtr<UObject> InitializedObjectReference;

	UPROPERTY(Transient)
	float UnusedValue;
};

/**
 * Direct base class for all UE objects
 * @note The full C++ class is located here: Engine\Source\Runtime\CoreUObject\Public\UObject\Object.h
 */
UCLASS(abstract, noexport)
class UObject
{
	GENERATED_BODY()
public:

	UObject(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UObject(FVTableHelper& Helper);
	
	/**
	 * Executes some portion of the ubergraph.
	 *
	 * @param	EntryPoint	The entry point to start code execution at.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(BlueprintInternalUseOnly = "true"))
	void ExecuteUbergraph(int32 EntryPoint);
};

/** Structure for file paths that are displayed in the editor with a picker UI. */
USTRUCT(noexport, BlueprintType)
struct FFilePath
{
	GENERATED_BODY()

	/**
		* The path to the file.
		*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FilePath)
	FString FilePath;
};

/** Structure for directory paths that are displayed in the editor with a picker UI. */
USTRUCT(noexport, BlueprintType)
struct FDirectoryPath
{
	GENERATED_BODY()

	/**
		* The path to the directory.
		*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Path)
	FString Path;
};



/// @endcond

#endif

