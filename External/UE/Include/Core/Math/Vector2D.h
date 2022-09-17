// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Crc.h"
#include "Math/UnrealMathUtility.h"
#include "Containers/UnrealString.h"
#include "Misc/Parse.h"
#include "Misc/LargeWorldCoordinatesSerializer.h"
#include "Misc/NetworkVersion.h"
#include "Math/IntPoint.h"
#include "Logging/LogMacros.h"

#ifdef _MSC_VER
#pragma warning (push)
// Ensure template functions don't generate shadowing warnings against global variables at the point of instantiation.
#pragma warning (disable : 4459)
#endif


/**
 * A vector in 2-D space composed of components (X, Y) with floating point precision.
 */
namespace UE
{
namespace Math
{
template<typename T>	
struct TVector2 
{
	static_assert(std::is_floating_point_v<T>, "T must be floating point");

public:
	using FReal = T;
	
	union
	{
		struct
		{
			/** Vector's X component. */
			T X;

			/** Vector's Y component. */
			T Y;
		};

		UE_DEPRECATED(all, "For internal use only")
		T XY[2];
	};

	/** Global 2D zero vector constant (0,0) */
	CORE_API static const TVector2<T> ZeroVector;

	/**
	* Global 2D one vector (poorly named) constant (1,1).
	*
	* @note Incorrectly named "unit" vector though its magnitude/length/size is not one. Would fix, though likely used all over the world. Use `Unit45Deg` below for an actual unit vector.
	*/
	CORE_API static const TVector2<T> UnitVector;

	/**
	* Global 2D unit vector constant along the 45 degree angle or symmetrical positive axes (sqrt(.5),sqrt(.5)) or (.707,.707). https://en.wikipedia.org/wiki/Unit_vector
	*
	* @note The `UnitVector` above is actually a value with axes of 1 rather than a magnitude of one.
	*/
	CORE_API static const TVector2<T> Unit45Deg;

	static inline TVector2<T> Zero() { return TVector2<T>::ZeroVector; }
	static inline TVector2<T> One() { return TVector2<T>::UnitVector; }
	static inline TVector2<T> UnitX() { return TVector2<T>(1, 0); }
	static inline TVector2<T> UnitY() { return TVector2<T>(0, 1); }

public:

	/** Default constructor (no initialization). */
	FORCEINLINE TVector2<T>() { }

	/**
	* Constructor using initial values for each component.
	*
	* @param InX X coordinate.
	* @param InY Y coordinate.
	*/
	FORCEINLINE TVector2<T>(T InX, T InY);

	/**
	* Constructor initializing both components to a single T value.
	*
	* @param InF Value to set both components to.
	*/
	explicit FORCEINLINE TVector2<T>(T InF);

	/**
	* Constructs a vector from an FIntPoint.
	*
	* @param InPos Integer point used to set this vector.
	*/
	FORCEINLINE TVector2<T>(FIntPoint InPos);

	/**
	* Constructor which initializes all components to zero.
	*
	* @param EForceInit Force init enum
	*/
	explicit FORCEINLINE TVector2<T>(EForceInit);

	/**
	* Constructor that does not initialize.  More explicit than the default constructor.
	*
	* @param ENoInit Don't init
	*/
	explicit FORCEINLINE TVector2<T>(ENoInit) { }

	/**
	* Constructs a vector from an FVector.
	* Copies the X and Y components from the FVector.
	*
	* @param V Vector to copy from.
	*/
	explicit FORCEINLINE TVector2<T>(const TVector<T>& V);

	/**
	* Constructs a vector from an FVector4.
	* Copies the X and Y components from the FVector4.
	*
	* @param V Vector to copy from.
	*/
	explicit FORCEINLINE TVector2<T>(const TVector4<T>& V);

public:

	/**
	* Gets the result of adding two vectors together.
	*
	* @param V The other vector to add to this.
	* @return The result of adding the vectors together.
	*/
	FORCEINLINE TVector2<T> operator+(const TVector2<T>& V) const;

	/**
	* Gets the result of subtracting a vector from this one.
	*
	* @param V The other vector to subtract from this.
	* @return The result of the subtraction.
	*/
	FORCEINLINE TVector2<T> operator-(const TVector2<T>& V) const;

	/**
	* Gets the result of scaling the vector (multiplying each component by a value).
	*
	* @param Scale How much to scale the vector by.
	* @return The result of scaling this vector.
	*/
	FORCEINLINE TVector2<T> operator*(T Scale) const;

	/**
	* Gets the result of dividing each component of the vector by a value.
	*
	* @param Scale How much to divide the vector by.
	* @return The result of division on this vector.
	*/
	TVector2<T> operator/(T Scale) const;

	/**
	* Gets the result of adding A to each component of the vector.
	*
	* @param A T to add to each component.
	* @return The result of adding A to each component.
	*/
	FORCEINLINE TVector2<T> operator+(T A) const;

	/**
	* Gets the result of subtracting A from each component of the vector.
	*
	* @param A T to subtract from each component
	* @return The result of subtracting A from each component.
	*/
	FORCEINLINE TVector2<T> operator-(T A) const;

	/**
	* Gets the result of component-wise multiplication of this vector by another.
	*
	* @param V The other vector to multiply this by.
	* @return The result of the multiplication.
	*/
	FORCEINLINE TVector2<T> operator*(const TVector2<T>& V) const;

	/**
	* Gets the result of component-wise division of this vector by another.
	*
	* @param V The other vector to divide this by.
	* @return The result of the division.
	*/
	TVector2<T> operator/(const TVector2<T>& V) const;

	/**
	* Calculates dot product of this vector and another.
	*
	* @param V The other vector.
	* @return The dot product.
	*/
	FORCEINLINE T operator|(const TVector2<T>& V) const;

	/**
	* Calculates cross product of this vector and another.
	*
	* @param V The other vector.
	* @return The cross product.
	*/
	FORCEINLINE T operator^(const TVector2<T>& V) const;

public:

	/**
	* Compares this vector against another for equality.
	*
	* @param V The vector to compare against.
	* @return true if the two vectors are equal, otherwise false.
	*/
	bool operator==(const TVector2<T>& V) const;

	/**
	* Compares this vector against another for inequality.
	*
	* @param V The vector to compare against.
	* @return true if the two vectors are not equal, otherwise false.
	*/
	bool operator!=(const TVector2<T>& V) const;

	/**
	* Checks whether both components of this vector are less than another.
	*
	* @param Other The vector to compare against.
	* @return true if this is the smaller vector, otherwise false.
	*/
	bool operator<(const TVector2<T>& Other) const;

	/**
	* Checks whether both components of this vector are greater than another.
	*
	* @param Other The vector to compare against.
	* @return true if this is the larger vector, otherwise false.
	*/
	bool operator>(const TVector2<T>& Other) const;

	/**
	* Checks whether both components of this vector are less than or equal to another.
	*
	* @param Other The vector to compare against.
	* @return true if this vector is less than or equal to the other vector, otherwise false.
	*/
	bool operator<=(const TVector2<T>& Other) const;

	/**
	* Checks whether both components of this vector are greater than or equal to another.
	*
	* @param Other The vector to compare against.
	* @return true if this vector is greater than or equal to the other vector, otherwise false.
	*/
	bool operator>=(const TVector2<T>& Other) const;

	/**
	* Gets a negated copy of the vector.
	*
	* @return A negated copy of the vector.
	*/
	FORCEINLINE TVector2<T> operator-() const;

	/**
	* Adds another vector to this.
	*
	* @param V The other vector to add.
	* @return Copy of the vector after addition.
	*/
	FORCEINLINE TVector2<T> operator+=(const TVector2<T>& V);

	/**
	* Subtracts another vector from this.
	*
	* @param V The other vector to subtract.
	* @return Copy of the vector after subtraction.
	*/
	FORCEINLINE TVector2<T> operator-=(const TVector2<T>& V);

	/**
	* Scales this vector.
	*
	* @param Scale The scale to multiply vector by.
	* @return Copy of the vector after scaling.
	*/
	FORCEINLINE TVector2<T> operator*=(T Scale);

	/**
	* Divides this vector.
	*
	* @param V What to divide vector by.
	* @return Copy of the vector after division.
	*/
	TVector2<T> operator/=(T V);

	/**
	* Multiplies this vector with another vector, using component-wise multiplication.
	*
	* @param V The vector to multiply with.
	* @return Copy of the vector after multiplication.
	*/
	TVector2<T> operator*=(const TVector2<T>& V);

	/**
	* Divides this vector by another vector, using component-wise division.
	*
	* @param V The vector to divide by.
	* @return Copy of the vector after division.
	*/
	TVector2<T> operator/=(const TVector2<T>& V);

	/**
	* Gets specific component of the vector.
	*
	* @param Index the index of vector component
	* @return reference to component.
	*/
	T& operator[](int32 Index);

	/**
	* Gets specific component of the vector.
	*
	* @param Index the index of vector component
	* @return copy of component value.
	*/
	T operator[](int32 Index) const;

	/**
	* Gets a specific component of the vector.
	*
	* @param Index The index of the component required.
	* @return Reference to the specified component.
	*/
	T& Component(int32 Index);

	/**
	* Gets a specific component of the vector.
	*
	* @param Index The index of the component required.
	* @return Copy of the specified component.
	*/
	T Component(int32 Index) const;

public:

	/**
	* Calculates the dot product of two vectors.
	*
	* @param A The first vector.
	* @param B The second vector.
	* @return The dot product.
	*/
	FORCEINLINE static T DotProduct(const TVector2<T>& A, const TVector2<T>& B);

	/**
	* Squared distance between two 2D points.
	*
	* @param V1 The first point.
	* @param V2 The second point.
	* @return The squared distance between two 2D points.
	*/
	FORCEINLINE static T DistSquared(const TVector2<T>& V1, const TVector2<T>& V2);

	/**
	* Distance between two 2D points.
	*
	* @param V1 The first point.
	* @param V2 The second point.
	* @return The distance between two 2D points.
	*/
	FORCEINLINE static T Distance(const TVector2<T>& V1, const TVector2<T>& V2);

	/**
	* Calculate the cross product of two vectors.
	*
	* @param A The first vector.
	* @param B The second vector.
	* @return The cross product.
	*/
	FORCEINLINE static T CrossProduct(const TVector2<T>& A, const TVector2<T>& B);

	/**
	* Returns a vector with the maximum component for each dimension from the pair of vectors.
	*
	* @param A The first vector.
	* @param B The second vector.
	* @return The max vector.
	*/
	FORCEINLINE static TVector2<T> Max(const TVector2<T>& A, const TVector2<T>& B);

	/**
	* Returns a vector with the minimum component for each dimension from the pair of vectors.
	*
	* @param A The first vector.
	* @param B The second vector.
	* @return The min vector.
	*/
	FORCEINLINE static TVector2<T> Min(const TVector2<T>& A, const TVector2<T>& B);

	/**
	* Checks for equality with error-tolerant comparison.
	*
	* @param V The vector to compare.
	* @param Tolerance Error tolerance.
	* @return true if the vectors are equal within specified tolerance, otherwise false.
	*/
	bool Equals(const TVector2<T>& V, T Tolerance=KINDA_SMALL_NUMBER) const;

	/**
	* Set the values of the vector directly.
	*
	* @param InX New X coordinate.
	* @param InY New Y coordinate.
	*/
	void Set(T InX, T InY);

	/**
	* Get the maximum value of the vector's components.
	*
	* @return The maximum value of the vector's components.
	*/
	T GetMax() const;

	/**
	* Get the maximum absolute value of the vector's components.
	*
	* @return The maximum absolute value of the vector's components.
	*/
	T GetAbsMax() const;

	/**
	* Get the minimum value of the vector's components.
	*
	* @return The minimum value of the vector's components.
	*/
	T GetMin() const;

	/**
	* Get the length (magnitude) of this vector.
	*
	* @return The length of this vector.
    * @see Length - This function is a synonym for Length()
	*/
	T Size() const;

	/**
    * Get the length (magnitude) of this vector.
    *
    * @return The length of this vector.
    * @see Size - This function is a synonym for Size()
    */
	FORCEINLINE T Length() const { return Size(); }

	/**
	* Get the squared length of this vector.
	*
	* @return The squared length of this vector.
    * @see LengthSquared - This function is a synonym for LengthSquared()
	*/
	T SizeSquared() const;
	
	/**
	* Get the squared length of this vector.
	*
	* @return The squared length of this vector.
    * @see SizeSquared - This function is a synonym for SizeSquared()
	*/
	FORCEINLINE T SquaredLength() const { return SizeSquared(); }

	/**
	* Get the dot product of this vector against another.
	*
	* @param V2 The vector to measure dot product against.
	* @return The dot product.
	*/
	T Dot(const TVector2<T>& V2) const;

	/**
	* Rotates around axis (0,0,1)
	*
	* @param AngleDeg Angle to rotate (in degrees)
	* @return Rotated Vector
	*/
	TVector2<T> GetRotated(T AngleDeg) const;

	/**
	* Gets a normalized copy of the vector, checking it is safe to do so based on the length.
	* Returns zero vector if vector length is too small to safely normalize.
	*
	* @param Tolerance Minimum squared length of vector for normalization.
	* @return A normalized copy of the vector if safe, (0,0) otherwise.
	*/
	TVector2<T> GetSafeNormal(T Tolerance=SMALL_NUMBER) const;

	/**
	* Normalize this vector in-place if it is large enough, set it to (0,0) otherwise.
	*
	* @param Tolerance Minimum squared length of vector for normalization.
	* @see GetSafeNormal()
	*/
	void Normalize(T Tolerance=SMALL_NUMBER);

	/**
	* Checks whether vector is near to zero within a specified tolerance.
	*
	* @param Tolerance Error tolerance.
	* @return true if vector is in tolerance to zero, otherwise false.
	*/
	bool IsNearlyZero(T Tolerance=KINDA_SMALL_NUMBER) const;

	/**
	* Util to convert this vector into a unit direction vector and its original length.
	*
	* @param OutDir Reference passed in to store unit direction vector.
	* @param OutLength Reference passed in to store length of the vector.
	*/
	void ToDirectionAndLength(TVector2<T> &OutDir, double &OutLength) const;
	void ToDirectionAndLength(TVector2<T> &OutDir, float &OutLength) const;

	/**
	* Checks whether all components of the vector are exactly zero.
	*
	* @return true if vector is exactly zero, otherwise false.
	*/
	bool IsZero() const;

	/**
	* Get this vector as an Int Point.
	*
	* @return New Int Point from this vector.
	*/
	FIntPoint IntPoint() const;

	/**
	* Get this vector as a vector where each component has been rounded to the nearest int.
	*
	* @return New TVector2<T> from this vector that is rounded.
	*/
	TVector2<T> RoundToVector() const;

	/**
	* Creates a copy of this vector with both axes clamped to the given range.
	* @return New vector with clamped axes.
	*/
	TVector2<T> ClampAxes(T MinAxisVal, T MaxAxisVal) const;

	/**
	* Get a copy of the vector as sign only.
	* Each component is set to +1 or -1, with the sign of zero treated as +1.
	*
	* @param A copy of the vector with each component set to +1 or -1
	*/
	FORCEINLINE TVector2<T> GetSignVector() const;

	/**
	* Get a copy of this vector with absolute value of each component.
	*
	* @return A copy of this vector with absolute value of each component.
	*/
	FORCEINLINE TVector2<T> GetAbs() const;

	/**
	* Get a textual representation of the vector.
	*
	* @return Text describing the vector.
	*/
	FString ToString() const;

	/**
	* Initialize this Vector based on an FString. The String is expected to contain X=, Y=.
	* The TVector2<T> will be bogus when InitFromString returns false.
	*
	* @param	InSourceString	FString containing the vector values.
	* @return true if the X,Y values were read successfully; false otherwise.
	*/
	bool InitFromString(const FString& InSourceString);

	bool Serialize(FArchive& Ar)
	{
		Ar << *this;
		return true;
	}

	bool Serialize(FStructuredArchive::FSlot Slot)
	{
		Slot << *this;
		return true;
	}

	bool SerializeFromMismatchedTag(FName StructTag, FArchive& Ar);

#if ENABLE_NAN_DIAGNOSTIC
	FORCEINLINE void DiagnosticCheckNaN()
	{
		if (ContainsNaN())
		{
			logOrEnsureNanError(TEXT("FVector2 contains NaN: %s"), *ToString());
			*this = TVector2<T>::ZeroVector;
		}
	}
#else
	FORCEINLINE void DiagnosticCheckNaN() {}
#endif

	/**
	* Utility to check if there are any non-finite values (NaN or Inf) in this vector.
	*
	* @return true if there are any non-finite values in this vector, false otherwise.
	*/
	FORCEINLINE bool ContainsNaN() const
	{
		return (!FMath::IsFinite(X) || 
				!FMath::IsFinite(Y));
	}

	/**
	* Network serialization function.
	* FVectors NetSerialize without quantization (ie exact values are serialized).
	*/
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		if (Ar.EngineNetVer() >= HISTORY_SERIALIZE_DOUBLE_VECTORS_AS_DOUBLES)
		{
			Ar << X << Y;
		}
		else
		{
			checkf(Ar.IsLoading(), TEXT("float -> double conversion applied outside of load!"));
			// Always serialize as float
			float SX, SY;
			Ar << SX << SY;
			X = SX;
			Y = SY;
		}
		return true;
	}

	/** Converts spherical coordinates on the unit sphere into a Cartesian unit length vector. */
	inline TVector<T> SphericalToUnitCartesian() const;
	
	
	
	// Conversion from other type.
	template<typename FArg, TEMPLATE_REQUIRES(!TIsSame<T, FArg>::Value)>
	explicit TVector2(const TVector2<FArg>& From) : TVector2<T>((T)From.X, (T)From.Y) {}
};

/**
* Creates a hash value from a TVector2<T>. 
*
* @param Vector the vector to create a hash value for
* @return The hash value from the components
*/
template<typename T>
FORCEINLINE uint32 GetTypeHash(const TVector2<T>& Vector)
{
	// Note: this assumes there's no padding in TVector2<T> that could contain uncompared data.
	return FCrc::MemCrc_DEPRECATED(&Vector,sizeof(Vector));
}

/* TVector2<T> inline functions
*****************************************************************************/

/**
* Serialize a vector.
*
* @param Ar Serialization archive.
* @param V Vector being serialized.
* @return Reference to Archive after serialization.
*/
inline FArchive& operator<<(FArchive& Ar, TVector2<float>& V)
{
	// @warning BulkSerialize: TVector2<T> is serialized as memory dump
	// See TArray::BulkSerialize for detailed description of implied limitations.
	return Ar << V.X << V.Y;
}

inline void operator<<(FStructuredArchive::FSlot Slot, TVector2<float>& V)
{
	// @warning BulkSerialize: TVector2<T> is serialized as memory dump
	// See TArray::BulkSerialize for detailed description of implied limitations.
	FStructuredArchive::FStream Stream = Slot.EnterStream();
	Stream.EnterElement() << V.X;
	Stream.EnterElement() << V.Y;
}

inline FArchive& operator<<(FArchive& Ar, TVector2<double>& V)
{
	// @warning BulkSerialize: TVector2<T> is serialized as memory dump
	// See TArray::BulkSerialize for detailed description of implied limitations.
	if (Ar.UEVer() >= EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES)
	{
		Ar << V.X << V.Y;
	}
	else
	{
		checkf(Ar.IsLoading(), TEXT("float -> double conversion applied outside of load!"));
		// Stored as floats, so serialize float and copy.
		float X, Y;
		Ar << X << Y;
		V = TVector2<double>(X, Y);
	}
	return Ar;
}

inline void operator<<(FStructuredArchive::FSlot Slot, TVector2<double>& V)
{
	// @warning BulkSerialize: TVector2<T> is serialized as memory dump
	// See TArray::BulkSerialize for detailed description of implied limitations.
	FStructuredArchive::FStream Stream = Slot.EnterStream();
	if (Slot.GetUnderlyingArchive().UEVer() >= EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES)
	{
		Stream.EnterElement() << V.X;
		Stream.EnterElement() << V.Y;
	}
	else
	{
		checkf(Slot.GetUnderlyingArchive().IsLoading(), TEXT("float -> double conversion applied outside of load!"));
		// Stored as floats, so serialize float and copy.
		float X, Y;
		Stream.EnterElement() << X;
		Stream.EnterElement() << Y;
		V = TVector2<double>(X, Y);
	}
}

#if !defined(_MSC_VER) || defined(__clang__)  // MSVC can't forward declare explicit specializations
template<> CORE_API const FVector2f FVector2f::ZeroVector;
template<> CORE_API const FVector2f FVector2f::UnitVector;
template<> CORE_API const FVector2f FVector2f::Unit45Deg;
template<> CORE_API const FVector2d FVector2d::ZeroVector;
template<> CORE_API const FVector2d FVector2d::UnitVector;
template<> CORE_API const FVector2d FVector2d::Unit45Deg;
#endif

/**
 * Multiplies a Vector2 by a scaling factor.
 *
 * @param Scale Scaling factor.
 * @param V Vector2 to scale.
 * @return Result of multiplication.
 */
template<typename T, typename T2, TEMPLATE_REQUIRES(std::is_arithmetic<T2>::value)>
FORCEINLINE TVector2<T> operator*(T2 Scale, const TVector2<T>& V)
{
	return V.operator*(Scale);
}

template<typename T>
FORCEINLINE TVector2<T>::TVector2(T InX,T InY)
	:	X(InX), Y(InY)
{ }

template<typename T>
FORCEINLINE TVector2<T>::TVector2(T InF)
	:	X(InF), Y(InF)
{ }

template<typename T>
FORCEINLINE TVector2<T>::TVector2(FIntPoint InPos)
{
	X = (T)InPos.X;
	Y = (T)InPos.Y;
}

template<typename T>
FORCEINLINE TVector2<T>::TVector2(EForceInit)
	: X(0), Y(0)
{
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator+(const TVector2<T>& V) const
{
	return TVector2<T>(X + V.X, Y + V.Y);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator-(const TVector2<T>& V) const
{
	return TVector2<T>(X - V.X, Y - V.Y);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator*(T Scale) const
{
	return TVector2<T>(X * Scale, Y * Scale);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator/(T Scale) const
{
	const T RScale = 1.f/Scale;
	return TVector2<T>(X * RScale, Y * RScale);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator+(T A) const
{
	return TVector2<T>(X + A, Y + A);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator-(T A) const
{
	return TVector2<T>(X - A, Y - A);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator*(const TVector2<T>& V) const
{
	return TVector2<T>(X * V.X, Y * V.Y);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator/(const TVector2<T>& V) const
{
	return TVector2<T>(X / V.X, Y / V.Y);
}

template<typename T>
FORCEINLINE T TVector2<T>::operator|(const TVector2<T>& V) const
{
	return X*V.X + Y*V.Y;
}

template<typename T>
FORCEINLINE T TVector2<T>::operator^(const TVector2<T>& V) const
{
	return X*V.Y - Y*V.X;
}

template<typename T>
FORCEINLINE T TVector2<T>::DotProduct(const TVector2<T>& A, const TVector2<T>& B)
{
	return A | B;
}

template<typename T>
FORCEINLINE T TVector2<T>::DistSquared(const TVector2<T> &V1, const TVector2<T> &V2)
{
	return FMath::Square(V2.X-V1.X) + FMath::Square(V2.Y-V1.Y);
}

template<typename T>
FORCEINLINE T TVector2<T>::Distance(const TVector2<T>& V1, const TVector2<T>& V2)
{
	return FMath::Sqrt(TVector2<T>::DistSquared(V1, V2));
}

template<typename T>
FORCEINLINE T TVector2<T>::CrossProduct(const TVector2<T>& A, const TVector2<T>& B)
{
	return A ^ B;
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::Max(const TVector2<T>& A, const TVector2<T>& B)
{
	return TVector2<T>(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y));
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::Min(const TVector2<T>& A, const TVector2<T>& B)
{
	return TVector2<T>(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y));
}

template<typename T>
FORCEINLINE bool TVector2<T>::operator==(const TVector2<T>& V) const
{
	return X==V.X && Y==V.Y;
}

template<typename T>
FORCEINLINE bool TVector2<T>::operator!=(const TVector2<T>& V) const
{
	return X!=V.X || Y!=V.Y;
}

template<typename T>
FORCEINLINE bool TVector2<T>::operator<(const TVector2<T>& Other) const
{
	return X < Other.X && Y < Other.Y;
}

template<typename T>
FORCEINLINE bool TVector2<T>::operator>(const TVector2<T>& Other) const
{
	return X > Other.X && Y > Other.Y;
}

template<typename T>
FORCEINLINE bool TVector2<T>::operator<=(const TVector2<T>& Other) const
{
	return X <= Other.X && Y <= Other.Y;
}

template<typename T>
FORCEINLINE bool TVector2<T>::operator>=(const TVector2<T>& Other) const
{
	return X >= Other.X && Y >= Other.Y;
}

template<typename T>
FORCEINLINE bool TVector2<T>::Equals(const TVector2<T>& V, T Tolerance) const
{
	return FMath::Abs(X-V.X) <= Tolerance && FMath::Abs(Y-V.Y) <= Tolerance;
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator-() const
{
	return TVector2<T>(-X, -Y);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator+=(const TVector2<T>& V)
{
	X += V.X; Y += V.Y;
	return *this;
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator-=(const TVector2<T>& V)
{
	X -= V.X; Y -= V.Y;
	return *this;
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator*=(T Scale)
{
	X *= Scale; Y *= Scale;
	return *this;
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator/=(T V)
{
	const T RV = 1.f/V;
	X *= RV; Y *= RV;
	return *this;
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator*=(const TVector2<T>& V)
{
	X *= V.X; Y *= V.Y;
	return *this;
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::operator/=(const TVector2<T>& V)
{
	X /= V.X; Y /= V.Y;
	return *this;
}

template<typename T>
FORCEINLINE T& TVector2<T>::operator[](int32 Index)
{
	check(Index>=0 && Index<2);
	return ((Index == 0) ? X : Y);
}

template<typename T>
FORCEINLINE T TVector2<T>::operator[](int32 Index) const
{
	check(Index>=0 && Index<2);
	return ((Index == 0) ? X : Y);
}

template<typename T>
FORCEINLINE void TVector2<T>::Set(T InX, T InY)
{
	X = InX;
	Y = InY;
}

template<typename T>
FORCEINLINE T TVector2<T>::GetMax() const
{
	return FMath::Max(X,Y);
}

template<typename T>
FORCEINLINE T TVector2<T>::GetAbsMax() const
{
	return FMath::Max(FMath::Abs(X),FMath::Abs(Y));
}

template<typename T>
FORCEINLINE T TVector2<T>::GetMin() const
{
	return FMath::Min(X,Y);
}

template<typename T>
FORCEINLINE T TVector2<T>::Size() const
{
	return FMath::Sqrt(X*X + Y*Y);
}

template<typename T>
FORCEINLINE T TVector2<T>::SizeSquared() const
{
	return X*X + Y*Y;
}

template<typename T>
FORCEINLINE T TVector2<T>::Dot(const TVector2<T>& V2) const
{ 
	return X * V2.X + Y * V2.Y;
} 

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::GetRotated(const T AngleDeg) const
{
	// Based on FVector::RotateAngleAxis with Axis(0,0,1)

	T S, C;
	FMath::SinCos(&S, &C, FMath::DegreesToRadians(AngleDeg));

	const T OMC = 1.0f - C;

	return TVector2<T>(
		C * X - S * Y,
		S * X + C * Y);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::GetSafeNormal(T Tolerance) const
{	
	const T SquareSum = X*X + Y*Y;
	if(SquareSum > Tolerance)
	{
		const T Scale = FMath::InvSqrt(SquareSum);
		return TVector2<T>(X*Scale, Y*Scale);
	}
	return TVector2<T>(0.f, 0.f);
}

template<typename T>
FORCEINLINE void TVector2<T>::Normalize(T Tolerance)
{
	const T SquareSum = X*X + Y*Y;
	if(SquareSum > Tolerance)
	{
		const T Scale = FMath::InvSqrt(SquareSum);
		X *= Scale;
		Y *= Scale;
		return;
	}
	X = 0.0f;
	Y = 0.0f;
}

template<typename T>
FORCEINLINE void TVector2<T>::ToDirectionAndLength(TVector2<T> &OutDir, double &OutLength) const
{
	OutLength = Size();
	if (OutLength > SMALL_NUMBER)
	{
		T OneOverLength = 1.0f / OutLength;
		OutDir = TVector2<T>(X*OneOverLength, Y*OneOverLength);
	}
	else
	{
		OutDir = TVector2<T>::ZeroVector;
	}
}

template<typename T>
FORCEINLINE void TVector2<T>::ToDirectionAndLength(TVector2<T> &OutDir, float &OutLength) const
{
	OutLength = Size();
	if (OutLength > SMALL_NUMBER)
	{
		float OneOverLength = 1.0f / OutLength;
		OutDir = TVector2<T>(X*OneOverLength, Y*OneOverLength);
	}
	else
	{
		OutDir = TVector2<T>::ZeroVector;
	}
}

template<typename T>
FORCEINLINE bool TVector2<T>::IsNearlyZero(T Tolerance) const
{
	return	FMath::Abs(X)<=Tolerance
		&&	FMath::Abs(Y)<=Tolerance;
}

template<typename T>
FORCEINLINE bool TVector2<T>::IsZero() const
{
	return X==0.f && Y==0.f;
}

template<typename T>
FORCEINLINE T& TVector2<T>::Component(int32 Index)
{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return XY[Index];
PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

template<typename T>
FORCEINLINE T TVector2<T>::Component(int32 Index) const
{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return XY[Index];
PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

template<typename T>
FORCEINLINE FIntPoint TVector2<T>::IntPoint() const
{
	return FIntPoint(FMath::RoundToInt(X), FMath::RoundToInt(Y));
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::RoundToVector() const
{
	if constexpr (std::is_same_v<T, float>)
	{
		return TVector2<T>(FMath::RoundToFloat(X), FMath::RoundToFloat(Y));
	}
	return TVector2<T>(FMath::RoundToDouble(X), FMath::RoundToDouble(Y));
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::ClampAxes(T MinAxisVal, T MaxAxisVal) const
{
	return TVector2<T>(FMath::Clamp(X, MinAxisVal, MaxAxisVal), FMath::Clamp(Y, MinAxisVal, MaxAxisVal));
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::GetSignVector() const
{
	return TVector2<T>
		(
		FMath::FloatSelect(X, (T)1, (T)-1),
		FMath::FloatSelect(Y, (T)1, (T)-1)
		);
}

template<typename T>
FORCEINLINE TVector2<T> TVector2<T>::GetAbs() const
{
	return TVector2<T>(FMath::Abs(X), FMath::Abs(Y));
}

template<typename T>
FORCEINLINE FString TVector2<T>::ToString() const
{
	return FString::Printf(TEXT("X=%3.3f Y=%3.3f"), X, Y);
}

template<typename T>
FORCEINLINE bool TVector2<T>::InitFromString(const FString& InSourceString)
{
	X = Y = 0;

	// The initialization is only successful if the X and Y values can all be parsed from the string
	const bool bSuccessful = FParse::Value(*InSourceString, TEXT("X=") , X) && FParse::Value(*InSourceString, TEXT("Y="), Y) ;

	return bSuccessful;
}

} // namespace UE::Math
} // UE

UE_DECLARE_LWC_TYPE(Vector2,, FVector2D);

template <> struct TIsPODType<FVector2f> { enum { Value = true }; };
template <> struct TIsUECoreVariant<FVector2f> { enum { Value = true }; };
template<> struct TCanBulkSerialize<FVector2f> { enum { Value = true }; };
template <> struct TIsPODType<FVector2d> { enum { Value = true }; };
template <> struct TIsUECoreVariant<FVector2d> { enum { Value = true }; };
template<> struct TCanBulkSerialize<FVector2d> { enum { Value = true }; };

template<>
inline bool FVector2f::SerializeFromMismatchedTag(FName StructTag, FArchive& Ar)
{
	return UE_SERIALIZE_VARIANT_FROM_MISMATCHED_TAG(Ar, Vector2D, Vector2f, Vector2d);
}

template<>
inline bool FVector2d::SerializeFromMismatchedTag(FName StructTag, FArchive& Ar)
{
	return UE_SERIALIZE_VARIANT_FROM_MISMATCHED_TAG(Ar, Vector2D, Vector2d, Vector2f);
}




#ifdef _MSC_VER
#pragma warning (pop)
#endif