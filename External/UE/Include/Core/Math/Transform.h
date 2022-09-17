// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Include the current implementation of a FTransform, depending on the vector processing mode
 */

#include "CoreTypes.h"
#include "Math/Quat.h"
#include "Math/ScalarRegister.h"
#include "Math/TransformVectorized.h"
#include "Math/TransformNonVectorized.h"
#include "Misc/LargeWorldCoordinatesSerializer.h"

UE_DECLARE_LWC_TYPE(Transform, 3);

template<> struct TIsPODType<FTransform3f> { enum { Value = true }; };
template<> struct TIsPODType<FTransform3d> { enum { Value = true }; };
template<> struct TIsUECoreVariant<FTransform3f> { enum { Value = true }; };
template<> struct TIsUECoreVariant<FTransform3d> { enum { Value = true }; };
template<> struct TCanBulkSerialize<FTransform3f> { enum { Value = false }; }; // LWC_TODO: this was never defined before, should it be true or false?
template<> struct TCanBulkSerialize<FTransform3d> { enum { Value = false }; };
DECLARE_INTRINSIC_TYPE_LAYOUT(FTransform3f);
DECLARE_INTRINSIC_TYPE_LAYOUT(FTransform3d);

template<>
inline bool FTransform3f::SerializeFromMismatchedTag(FName StructTag, FArchive& Ar)
{
	// FTransform3f/d are immutable, but FTransform is not and needs a per-property conversion
	if(StructTag == NAME_Transform)
	{
		return false;
	}
	return UE_SERIALIZE_VARIANT_FROM_MISMATCHED_TAG(Ar, Transform, Transform3f, Transform3d);
}

template<>
inline bool FTransform3d::SerializeFromMismatchedTag(FName StructTag, FArchive& Ar)
{
	// FTransform3f/d are immutable, but FTransform is not and needs a per-property conversion
	if(StructTag == NAME_Transform)
	{
		return false;
	}
	return UE_SERIALIZE_VARIANT_FROM_MISMATCHED_TAG(Ar, Transform, Transform3d, Transform3f);
}