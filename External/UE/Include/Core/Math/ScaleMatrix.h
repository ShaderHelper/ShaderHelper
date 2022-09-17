// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Math/Plane.h"
#include "Math/Matrix.h"

namespace UE {
namespace Math {

/**
 * Scale matrix.
 */
template<typename T>
struct TScaleMatrix
	: public TMatrix<T>
{
public:

	/**
	 * @param Scale uniform scale to apply to matrix.
	 */
	TScaleMatrix( T Scale );

	/**
	 * @param Scale Non-uniform scale to apply to matrix.
	 */
	TScaleMatrix( const TVector<T>& Scale );

	// Conversion to other type.
	template<typename FArg, TEMPLATE_REQUIRES(!TIsSame<T, FArg>::Value)>
	explicit TScaleMatrix(const TScaleMatrix<FArg>& From) : TMatrix<T>(From) {}
	
	/** Matrix factory. Return an TMatrix<T> so we don't have type conversion issues in expressions. */
	static TMatrix<T> Make(T Scale)
	{
		return TScaleMatrix<T>(Scale);
	}

	/** Matrix factory. Return an TMatrix<T> so we don't have type conversion issues in expressions. */
	static TMatrix<T> Make(const TVector<T>& Scale)
	{
		return TScaleMatrix<T>(Scale);
	}
};


/* FScaleMatrix inline functions
 *****************************************************************************/

template<typename T>
FORCEINLINE TScaleMatrix<T>::TScaleMatrix( T Scale )
	: TMatrix<T>(
		TPlane<T>(Scale,	0.0f,	0.0f,	0.0f),
		TPlane<T>(0.0f,		Scale,	0.0f,	0.0f),
		TPlane<T>(0.0f,		0.0f,	Scale,	0.0f),
		TPlane<T>(0.0f,		0.0f,	0.0f,	1.0f)
	)
{ }


template<typename T>
FORCEINLINE TScaleMatrix<T>::TScaleMatrix( const TVector<T>& Scale )
	: TMatrix<T>(
		TPlane<T>(Scale.X,	0.0f,		0.0f,		0.0f),
		TPlane<T>(0.0f,		Scale.Y,	0.0f,		0.0f),
		TPlane<T>(0.0f,		0.0f,		Scale.Z,	0.0f),
		TPlane<T>(0.0f,		0.0f,		0.0f,		1.0f)
	)
{ }
	
} // namespace Math
} // namespace UE

UE_DECLARE_LWC_TYPE(ScaleMatrix, 44);

template<> struct TIsUECoreVariant<FScaleMatrix44f> { enum { Value = true }; };
template<> struct TIsUECoreVariant<FScaleMatrix44d> { enum { Value = true }; };