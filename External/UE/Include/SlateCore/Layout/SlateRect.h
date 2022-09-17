// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"

/** 
 * A rectangle defined by upper-left and lower-right corners.
 * 
 * Assumes a "screen-like" coordinate system where the origin is in the top-left, with the Y-axis going down.
 * Functions like "contains" etc will not work with other conventions.
 * 
 *      +---------> X
 *      |
 *      |    (Left,Top)  
 *      |            o----o 
 *      |            |    |
 *      |            o----o 
 *      |                (Right, Bottom)
 *      \/
 *      Y
 */
class SLATECORE_API FSlateRect
{
public:

	float Left;
	float Top;
	float Right;
	float Bottom;

	explicit FSlateRect( float InLeft = -1, float InTop = -1, float InRight = -1, float InBottom = -1 )
		: Left(InLeft)
		, Top(InTop)
		, Right(InRight)
		, Bottom(InBottom)
	{ }

	FSlateRect( const FVector2D& InStartPos, const FVector2D& InEndPos )
		: Left(UE_REAL_TO_FLOAT(InStartPos.X))
		, Top(UE_REAL_TO_FLOAT(InStartPos.Y))
		, Right(UE_REAL_TO_FLOAT(InEndPos.X))
		, Bottom(UE_REAL_TO_FLOAT(InEndPos.Y))
	{ }

	/**
	 * Creates a rect from a top left point and extent. Provided as a factory function to not conflict
	 * with the TopLeft + BottomRight ctor.
	 */
	static FSlateRect FromPointAndExtent(const FVector2D& TopLeft, const FVector2D& Size)
	{
		return FSlateRect(TopLeft, TopLeft + Size);
	}
	
public:

	/**
	 * Determines if the rectangle has positive dimensions.
	 */
	FORCEINLINE bool IsValid() const
	{
		return !(Left == -1 && Right == -1 && Bottom == -1 && Top == -1) && Right >= Left && Bottom >= Top;
	}

	/**
	 * @return true, if the rectangle has a size of 0.
	 */
	FORCEINLINE bool IsEmpty() const
	{
		return GetSize().SizeSquared() == 0.0f;
	}

	/**
	 * Returns the size of the rectangle in each dimension.
	 *
	 * @return The size as a vector.
	 */
	FORCEINLINE FVector2D GetSize() const
	{
		return FVector2D( Right - Left, Bottom - Top );
	}

	/**
	 * @return the area of the rectangle
	 */
	FORCEINLINE float GetArea() const
	{
		return (Right - Left) * (Bottom - Top);
	}

	/**
	 * Returns the center of the rectangle
	 * 
	 * @return The center point.
	 */
	FORCEINLINE FVector2D GetCenter() const
	{
		return FVector2D( Left, Top ) + GetSize() * 0.5f;
	}

	/**
	 * Returns the top-left position of the rectangle
	 * 
	 * @return The top-left position.
	 */
	FORCEINLINE FVector2D GetTopLeft() const
	{
		return FVector2D( Left, Top );
	}

	/**
	 * Returns the top-right position of the rectangle
	 *
	 * @return The top-right position.
	 */
	FORCEINLINE FVector2D GetTopRight() const
	{
		return FVector2D(Right, Top);
	}

	/**
	 * Returns the bottom-right position of the rectangle
	 * 
	 * @return The bottom-right position.
	 */
	FORCEINLINE FVector2D GetBottomRight() const
	{
		return FVector2D( Right, Bottom );
	}

	/**
	 * Returns the bottom-left position of the rectangle
	 * 
	 * @return The bottom-left position.
	 */
	FORCEINLINE FVector2D GetBottomLeft() const
	{
		return FVector2D( Left, Bottom );
	}

	/**
	 * Return a rectangle that is contracted on each side by the amount specified in each margin.
	 *
	 * @param InsetAmount The amount to contract the geometry.
	 *
	 * @return An inset rectangle.
	 */
	FORCEINLINE FSlateRect InsetBy(const struct FMargin& InsetAmount) const
	{
		return FSlateRect(Left + InsetAmount.Left, Top + InsetAmount.Top, Right - InsetAmount.Right, Bottom - InsetAmount.Bottom);
	}

	/**
	 * Return a rectangle that is extended on each side by the amount specified in each margin.
	 *
	 * @param ExtendAmount The amount to extend the geometry.
	 *
	 * @return An extended rectangle.
	 */
	FORCEINLINE FSlateRect ExtendBy(const FMargin& ExtendAmount) const
	{
		return FSlateRect(Left - ExtendAmount.Left, Top - ExtendAmount.Top, Right + ExtendAmount.Right, Bottom + ExtendAmount.Bottom);
	}

	/**
	 * Return a rectangle that is offset by the amount specified .
	 *
	 * @param OffsetAmount The amount to contract the geometry.
	 *
	 * @return An offset rectangle.
	 */
	FORCEINLINE FSlateRect OffsetBy( const FVector2D& OffsetAmount ) const
	{
		return FSlateRect(GetTopLeft() + OffsetAmount, GetBottomRight() + OffsetAmount);
	}

	/**
	 * Return a rectangle that is scaled by the amount specified.
	 *
	 * @param ScaleBy The amount to scale the geometry.
	 *
	 * @return An scaled rectangle.
	 */
	FORCEINLINE FSlateRect ScaleBy(float ScaleBy) const
	{
		const FVector2D Delta = GetSize() * 0.5f * ScaleBy;
		return ExtendBy(FMargin(Delta));
	}

	/**
	 * Returns the rect that encompasses both rectangles
	 * 
	 * @param	Other	The other rectangle
	 *
	 * @return	Rectangle that is big enough to fit both rectangles
	 */
	FORCEINLINE FSlateRect Expand( const FSlateRect& Other ) const
	{
		return FSlateRect( FMath::Min( Left, Other.Left ), FMath::Min( Top, Other.Top ), FMath::Max( Right, Other.Right ), FMath::Max( Bottom, Other.Bottom ) );
	}

	/**
	 * Rounds the Left, Top, Right and Bottom fields and returns a new FSlateRect with rounded components.
	 */
	FORCEINLINE FSlateRect Round() const
	{
		return FSlateRect(
			FMath::RoundToFloat(Left),
			FMath::RoundToFloat(Top),
			FMath::RoundToFloat(Right),
			FMath::RoundToFloat(Bottom));
	}

	/**
	 * Returns the rectangle that is the intersection of this rectangle and Other.
	 * 
	 * @param	Other	The other rectangle
	 *
	 * @return	Rectangle over intersection.
	 */
	FORCEINLINE FSlateRect IntersectionWith(const FSlateRect& Other) const
	{
		bool bOverlapping;
		return IntersectionWith(Other, bOverlapping);
	}

	/**
	 * Returns the rectangle that is the intersection of this rectangle and Other, as well as if they were overlapping at all.
	 * 
	 * @param	Other	The other rectangle
	 * @param	OutOverlapping	[Out] Was there any overlap with the other rectangle.
	 *
	 * @return	Rectangle over intersection.
	 */
	FSlateRect IntersectionWith(const FSlateRect& Other, bool& OutOverlapping) const
	{
		FSlateRect Intersected( FMath::Max( this->Left, Other.Left ), FMath::Max(this->Top, Other.Top), FMath::Min( this->Right, Other.Right ), FMath::Min( this->Bottom, Other.Bottom ) );
		if ( (Intersected.Bottom < Intersected.Top) || (Intersected.Right < Intersected.Left) )
		{
			OutOverlapping = false;
			// The intersection has 0 area and should not be rendered at all.
			return FSlateRect(0,0,0,0);
		}
		else
		{
			OutOverlapping = true;
			return Intersected;
		}
	}

	/**
	 * Returns whether or not a point is inside the rectangle
	 * 
	 * @param Point	The point to check
	 * @return True if the point is inside the rectangle
	 */
	FORCEINLINE bool ContainsPoint( const FVector2D& Point ) const
	{
		return Point.X >= Left && Point.X <= Right && Point.Y >= Top && Point.Y <= Bottom;
	}

	bool operator==( const FSlateRect& Other ) const
	{
		return
			Left == Other.Left &&
			Top == Other.Top &&
			Right == Other.Right &&
			Bottom == Other.Bottom;
	}

	bool operator!=( const FSlateRect& Other ) const
	{
		return Left != Other.Left || Top != Other.Top || Right != Other.Right || Bottom != Other.Bottom;
	}

	friend FSlateRect operator+( const FSlateRect& A, const FSlateRect& B )
	{
		return FSlateRect( A.Left + B.Left, A.Top + B.Top, A.Right + B.Right, A.Bottom + B.Bottom );
	}

	friend FSlateRect operator-( const FSlateRect& A, const FSlateRect& B )
	{
		return FSlateRect( A.Left - B.Left, A.Top - B.Top, A.Right - B.Right, A.Bottom - B.Bottom );
	}

	friend FSlateRect operator*( float Scalar, const FSlateRect& Rect )
	{
		return FSlateRect( Rect.Left * Scalar, Rect.Top * Scalar, Rect.Right * Scalar, Rect.Bottom * Scalar );
	}

	/** Do rectangles A and B intersect? */
	static bool DoRectanglesIntersect( const FSlateRect& A, const FSlateRect& B )
	{
		//  Segments A and B do not intersect when:
		//
		//       (left)   A     (right)
		//         o-------------o
		//  o---o        OR         o---o
		//    B                       B
		//
		//
		// We assume the A and B are well-formed rectangles.
		// i.e. (Top,Left) is above and to the left of (Bottom,Right)
		const bool bDoNotOverlap =
			B.Right < A.Left || A.Right < B.Left ||
			B.Bottom < A.Top || A.Bottom < B.Top;

		return ! bDoNotOverlap;
	}

	/** Is rectangle B contained within rectangle A? */
	FORCEINLINE static bool IsRectangleContained( const FSlateRect& A, const FSlateRect& B )
	{
		return (A.Left <= B.Left) && (A.Right >= B.Right) && (A.Top <= B.Top) && (A.Bottom >= B.Bottom);
	}

	/**
	* Returns a string of containing the coordinates of the rect
	*
	* @return	A string of the rect coordinates 
	*/
	FString ToString() const;

	/**
	* Returns a string of containing the coordinates of the rect
	*
	* @param InSourceString A string containing the values to initialize this rect in format Left=Value Top=Value...
	*
	* @return	True if initialized successfully
	*/
	bool InitFromString(const FString& InSourceString);
};

/**
 * Transforms a rect by the given transform, ensuring the rect does not get inverted.
 * WARNING: this only really supports scales and offsets. Any skew or rotation that 
 * would turn this into an un-aligned rect will won't work because FSlateRect doesn't support
 * non-axis-alignment. Instead, convert to ta FSlateRotatedRect first and transform that.
 */
template <typename TransformType>
FSlateRect TransformRect(const TransformType& Transform, const FSlateRect& Rect)
{
	FVector2D TopLeftTransformed = TransformPoint(Transform, FVector2D(Rect.Left, Rect.Top));
	FVector2D BottomRightTransformed = TransformPoint(Transform, FVector2D(Rect.Right, Rect.Bottom));

	if (TopLeftTransformed.X > BottomRightTransformed.X)
	{
		Swap(TopLeftTransformed.X, BottomRightTransformed.X);
	}
	if (TopLeftTransformed.Y > BottomRightTransformed.Y)
	{
		Swap(TopLeftTransformed.Y, BottomRightTransformed.Y);
	}
	return FSlateRect(TopLeftTransformed, BottomRightTransformed);
}

template<> struct TIsPODType<FSlateRect> { enum { Value = true }; };
