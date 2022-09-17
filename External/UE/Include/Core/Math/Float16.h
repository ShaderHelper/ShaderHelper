// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Serialization/Archive.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Float32.h"
#include "Serialization/MemoryLayout.h"

/**
* 16 bit float components and conversion
*
*
* IEEE float 16
* Represented by 10-bit mantissa M, 5-bit exponent E, and 1-bit sign S
*
* Specials:
* 
* E=0, M=0			== 0.0
* E=0, M!=0			== Denormalized value (M / 2^10) * 2^-14
* 0<E<31, M=any		== (1 + M / 2^10) * 2^(E-15)
* E=31, M=0			== Infinity
* E=31, M!=0		== NAN
* 
* conversion from 32 bit float is with RTNE (round to nearest even)
*
* Legacy code truncated in the conversion.  SetTruncate can be used for backwards compatibility.
* 
*/
class FFloat16
{
public:

	uint16 Encoded;

	/** Default constructor */
	FFloat16();

	/** Copy constructor. */
	FFloat16(const FFloat16& FP16Value);

	/** Conversion constructor. Convert from Fp32 to Fp16. */
	FFloat16(float FP32Value);	

	/** Assignment operator. Convert from Fp32 to Fp16. */
	FFloat16& operator=(float FP32Value);

	/** Assignment operator. Copy Fp16 value. */
	FFloat16& operator=(const FFloat16& FP16Value);

	/** Convert from Fp16 to Fp32. */
	operator float() const;

	/** Convert from Fp32 to Fp16. (RTNE) */
	void Set(float FP32Value);
	
	/** Convert from Fp32 to Fp16. (backward-compatible truncate conversion) */
	void SetTruncate(float FP32Value);

	/** Convert from Fp16 to Fp32. */
	float GetFloat() const;

	/** Is the float negative without converting */
	bool IsNegative() const
	{
		// negative if sign bit is on
		// can be tested with int compare
		return (int16)Encoded < 0;
	}

	/**
	 * Serializes the FFloat16.
	 *
	 * @param Ar Reference to the serialization archive.
	 * @param V Reference to the FFloat16 being serialized.
	 *
	 * @return Reference to the Archive after serialization.
	 */
	friend FArchive& operator<<(FArchive& Ar, FFloat16& V)
	{
		return Ar << V.Encoded;
	}
};
template<> struct TCanBulkSerialize<FFloat16> { enum { Value = true }; };

DECLARE_INTRINSIC_TYPE_LAYOUT(FFloat16);

FORCEINLINE FFloat16::FFloat16()
	:	Encoded(0)
{ }


FORCEINLINE FFloat16::FFloat16(const FFloat16& FP16Value)
{
	Encoded = FP16Value.Encoded;
}


FORCEINLINE FFloat16::FFloat16(float FP32Value)
{
	Set(FP32Value);
}	


FORCEINLINE FFloat16& FFloat16::operator=(float FP32Value)
{
	Set(FP32Value);
	return *this;
}


FORCEINLINE FFloat16& FFloat16::operator=(const FFloat16& FP16Value)
{
	Encoded = FP16Value.Encoded;
	return *this;
}


FORCEINLINE FFloat16::operator float() const
{
	return GetFloat();
}


FORCEINLINE void FFloat16::Set(float FP32Value)
{
	// FPlatformMath::StoreHalf follows RTNE (round-to-nearest-even) rounding default convention
	FPlatformMath::StoreHalf(&Encoded, FP32Value);
}



FORCEINLINE float FFloat16::GetFloat() const
{
	return FPlatformMath::LoadHalf(&Encoded);
}


FORCEINLINE void FFloat16::SetTruncate(float FP32Value)
{

	union
	{
		struct
		{
#if PLATFORM_LITTLE_ENDIAN
			uint16	Mantissa : 10;
			uint16	Exponent : 5;
			uint16	Sign : 1;
#else
			uint16	Sign : 1;
			uint16	Exponent : 5;
			uint16	Mantissa : 10;			
#endif
		} Components;

		uint16	Encoded;
	} FP16;


	FFloat32 FP32(FP32Value);

	// Copy sign-bit
	FP16.Components.Sign = FP32.Components.Sign;

	// Check for zero, denormal or too small value.
	if (FP32.Components.Exponent <= 112)			// Too small exponent? (0+127-15)
	{
		// Set to 0.
		FP16.Components.Exponent = 0;
		FP16.Components.Mantissa = 0;

         // Exponent unbias the single, then bias the halfp
         const int32 NewExp = FP32.Components.Exponent - 127 + 15;
 
         if ( (14 - NewExp) <= 24 ) // Mantissa might be non-zero
         {
             uint32 Mantissa = FP32.Components.Mantissa | 0x800000; // Hidden 1 bit
             FP16.Components.Mantissa = (uint16)(Mantissa >> (14 - NewExp));
			 // Check for rounding
             if ( (Mantissa >> (13 - NewExp)) & 1 ) //-V1051
			 {
                 FP16.Encoded++; // Round, might overflow into exp bit, but this is OK
			 }
         }
	}
	// Check for INF or NaN, or too high value
	else if (FP32.Components.Exponent >= 143)		// Too large exponent? (31+127-15)
	{
		// Set to 65504.0 (max value)
		FP16.Components.Exponent = 30;
		FP16.Components.Mantissa = 1023;
	}
	// Handle normal number.
	else
	{
		FP16.Components.Exponent = uint16(int32(FP32.Components.Exponent) - 127 + 15);
		FP16.Components.Mantissa = uint16(FP32.Components.Mantissa >> 13);
	}

	Encoded = FP16.Encoded;
}
