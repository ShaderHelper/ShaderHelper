// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Math/Vector.h"
#include "Math/Plane.h"
#include "Math/Matrix.h"

namespace UE {
namespace Math {

template<typename T>
struct TTranslationMatrix
	: public TMatrix<T>
{
public:

	/** Constructor translation matrix based on given vector */
	TTranslationMatrix(const TVector<T>& Delta);

	// Conversion to other type.
	template<typename FArg, TEMPLATE_REQUIRES(!TIsSame<T, FArg>::Value)>
	explicit TTranslationMatrix(const TTranslationMatrix<FArg>& From) : TMatrix<T>(From) {}
	
	/** Matrix factory. Return an FMatrix so we don't have type conversion issues in expressions. */
	static TMatrix<T> Make(TVector<T> const& Delta)
	{
		return TTranslationMatrix<T>(Delta);
	}
};

template<typename T>
FORCEINLINE TTranslationMatrix<T>::TTranslationMatrix(const TVector<T>& Delta)
	: TMatrix<T>(
		TPlane<T>(1.0f,		0.0f,	0.0f,	0.0f),
		TPlane<T>(0.0f,		1.0f,	0.0f,	0.0f),
		TPlane<T>(0.0f,		0.0f,	1.0f,	0.0f),
		TPlane<T>(Delta.X,	Delta.Y,Delta.Z,1.0f)
	)
{ }
	
} // namespace Math
} // namespace UE

UE_DECLARE_LWC_TYPE(TranslationMatrix, 44);

template<> struct TIsUECoreVariant<FTranslationMatrix44f> { enum { Value = true }; };
template<> struct TIsUECoreVariant<FTranslationMatrix44d> { enum { Value = true }; };