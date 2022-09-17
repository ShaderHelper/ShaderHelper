// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "Math/Vector4.h"

struct FBounds
{
	FVector4f	Min = FVector4f(  MAX_flt,  MAX_flt,  MAX_flt );
	FVector4f	Max = FVector4f( -MAX_flt, -MAX_flt, -MAX_flt );

	FORCEINLINE FBounds& operator=( const FVector3f& Other )
	{
		Min = Other;
		Max = Other;
		return *this;
	}

	FORCEINLINE FBounds& operator+=( const FVector3f& Other )
	{
		VectorStoreAligned( VectorMin( VectorLoadAligned( &Min ), VectorLoadFloat3( &Other ) ), &Min );
		VectorStoreAligned( VectorMax( VectorLoadAligned( &Max ), VectorLoadFloat3( &Other ) ), &Max );
		return *this;
	}

	FORCEINLINE FBounds& operator+=( const FBounds& Other )
	{
		VectorStoreAligned( VectorMin( VectorLoadAligned( &Min ), VectorLoadAligned( &Other.Min ) ), &Min );
		VectorStoreAligned( VectorMax( VectorLoadAligned( &Max ), VectorLoadAligned( &Other.Max ) ), &Max );
		return *this;
	}

	FORCEINLINE FBounds operator+( const FBounds& Other ) const
	{
		return FBounds(*this) += Other;
	}

	FORCEINLINE float DistSqr( const FVector3f& Point ) const
	{
		VectorRegister4Float rMin		= VectorLoadAligned( &Min );
		VectorRegister4Float rMax		= VectorLoadAligned( &Max );
		VectorRegister4Float rPoint		= VectorLoadFloat3( &Point );
		VectorRegister4Float rClosest	= VectorSubtract( VectorMin( VectorMax( rPoint, rMin ), rMax ), rPoint );
		return VectorDot3Scalar( rClosest, rClosest );
	}

	FORCEINLINE FVector3f GetCenter() const
	{
		return (Max + Min) * 0.5f;
	}

	FORCEINLINE FVector3f GetExtent() const
	{
		return (Max - Min) * 0.5f;
	}

	FORCEINLINE float GetSurfaceArea() const
	{
		FVector3f Size = Max - Min;
		return 0.5f * ( Size.X * Size.Y + Size.X * Size.Z + Size.Y * Size.Z );
	}

	FORCEINLINE friend FArchive& operator<<( FArchive& Ar, FBounds& Bounds )
	{
		Ar << Bounds.Min;
		Ar << Bounds.Max;
		return Ar;
	}
};