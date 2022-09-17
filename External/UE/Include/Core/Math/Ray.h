// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/Vector.h"


/**
 * 3D Ray represented by Origin and (normalized) Direction
 */
namespace UE
{
namespace Math
{

template<typename T>
struct TRay
{
public:
	using FReal = T;

	/** Ray origin point */
	TVector<T> Origin;

	/** Ray direction vector (always normalized) */
	TVector<T> Direction;

public:

	/** Default constructor initializes ray to Zero origin and Z-axis direction */
	TRay()
	{
		Origin = TVector<T>::ZeroVector;
		Direction = TVector<T>(0, 0, 1);
	}

	/** 
	  * Initialize Ray with origin and direction
	  *
	  * @param Origin Ray Origin Point
	  * @param Direction Ray Direction Vector
	  * @param bDirectionIsNormalized Direction will be normalized unless this is passed as true (default false)
	  */
	TRay(const TVector<T>& Origin, const TVector<T>& Direction, bool bDirectionIsNormalized = false)
	{
		this->Origin = Origin;
		this->Direction = Direction;
		if (bDirectionIsNormalized == false)
		{
			this->Direction.Normalize();    // is this a full-accuracy sqrt?
		}
	}

	// Conversion to other type.
	template<typename FArg, TEMPLATE_REQUIRES(!TIsSame<T, FArg>::Value)>
	explicit TRay(const TRay<FArg>& From) : TRay<T>(TVector<T>(From.Origin), TVector<T>(From.Direction), true) {}

public:

	/** 
	 * Calculate position on ray at given distance/parameter
	 *
	 * @param RayParameter Scalar distance along Ray
	 * @return Point on Ray
	 */
	TVector<T> PointAt(T RayParameter) const
	{
		return Origin + RayParameter * Direction;
	}

	/**
	 * Calculate ray parameter (distance from origin to closest point) for query Point
	 *
	 * @param Point query Point
	 * @return distance along ray from origin to closest point
	 */
	T GetParameter(const TVector<T>& Point) const
	{
		return TVector<T>::DotProduct((Point - Origin), Direction);
	}

	/**
	 * Find minimum squared distance from query point to ray
	 *
	 * @param Point query Point
	 * @return squared distance to Ray
	 */
	T DistSquared(const TVector<T>& Point) const
	{
		T RayParameter = TVector<T>::DotProduct((Point - Origin), Direction);
		if (RayParameter < 0)
		{
			return TVector<T>::DistSquared(Origin, Point);
		}
		else 
		{
			TVector<T> ProjectionPt = Origin + RayParameter * Direction;
			return TVector<T>::DistSquared(ProjectionPt, Point);
		}
	}

	/**
	 * Find minimum distance from query point to ray
	 *
	 * @param Point query Point
	 * @return distance to Ray
	 */
	T Dist(const TVector<T>& Point) const
	{
		return FMath::Sqrt(DistSquared(Point));
	}

	/**
	 * Find closest point on ray to query point
	 * @param Point query point
	 * @return closest point on Ray
	 */
	TVector<T> ClosestPoint(const TVector<T>& Point) const
	{
		T RayParameter = TVector<T>::DotProduct((Point - Origin), Direction);
		if (RayParameter < 0) 
		{
			return Origin;
		}
		else 
		{
			return Origin + RayParameter * Direction;
		}
	}


};

}	// namespace UE::Math
}	// namespace UE

UE_DECLARE_LWC_TYPE(Ray, 3);

template<> struct TIsUECoreVariant<FRay3f> { enum { Value = true }; };
template<> struct TIsUECoreVariant<FRay3d> { enum { Value = true }; };

