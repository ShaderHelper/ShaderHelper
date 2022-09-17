// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Math/Color.h"
#include "Math/Float16.h"

/**
 *	RGBA Color made up of FFloat16
 */
class FFloat16Color
{
public:

	FFloat16 R;
	FFloat16 G;
	FFloat16 B;
	FFloat16 A;

	/* Get as a pointer to four half floats */
	uint16 * GetFourHalves() { return (uint16 *)this; }
	const uint16 * GetFourHalves() const { return (const uint16 *)this; }

	const FLinearColor GetFloats() const;

	/** Default constructor */
	FFloat16Color();

	/** Copy constructor. */
	FFloat16Color(const FFloat16Color& Src);

	/** Constructor from a linear color. */
	FFloat16Color(const FLinearColor& Src);

	/** assignment operator */
	FFloat16Color& operator=(const FFloat16Color& Src);

 	/**
	 * Checks whether two colors are identical.
	 *
	 * @param Src The other color.
	 * @return true if the two colors are identical, otherwise false.
	 */
	bool operator==(const FFloat16Color& Src) const;
};


FORCEINLINE FFloat16Color::FFloat16Color() { }


FORCEINLINE FFloat16Color::FFloat16Color(const FFloat16Color& Src)
{
	R = Src.R;
	G = Src.G;
	B = Src.B;
	A = Src.A;
}


FORCEINLINE FFloat16Color::FFloat16Color(const FLinearColor& Src)
{
	FPlatformMath::VectorStoreHalf( GetFourHalves(), (const float *)&Src );
}

	
FORCEINLINE const FLinearColor FFloat16Color::GetFloats() const
{
	FLinearColor Ret;
	FPlatformMath::VectorLoadHalf( (float *)&Ret, GetFourHalves() );
	return Ret;
}


FORCEINLINE FFloat16Color& FFloat16Color::operator=(const FFloat16Color& Src)
{
	R = Src.R;
	G = Src.G;
	B = Src.B;
	A = Src.A;
	return *this;
}

FORCEINLINE bool FFloat16Color::operator==(const FFloat16Color& Src) const
{
	return (
		(R == Src.R) &&
		(G == Src.G) &&
		(B == Src.B) &&
		(A == Src.A)
		);
}
