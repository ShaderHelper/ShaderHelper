// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/LargeWorldCoordinates.h"

struct FLargeWorldRenderScalar
{
public:
	// This should become inline at some point, but keeping it in cpp file for now to make iteration faster when experementing with tile size
	CORE_API static double GetTileSize();
	CORE_API static FVector3f GetTileFor(FVector InPosition);

	static double MakeTile(double InValue)
	{
		return FMath::FloorToDouble(InValue / GetTileSize() + 0.5);
	}

	static double MakeQuantizedTile(double InValue, double InQuantization)
	{
		return FMath::FloorToDouble((InValue / GetTileSize()) * InQuantization + 0.5) / InQuantization;
	}

	static FVector MakeTile(const FVector& InValue)
	{
		return FVector(MakeTile(InValue.X), MakeTile(InValue.Y), MakeTile(InValue.Z));
	}

	static FVector MakeQuantizedTile(const FVector& InValue, double InQuantization)
	{
		return FVector(MakeQuantizedTile(InValue.X, InQuantization), MakeQuantizedTile(InValue.Y, InQuantization), MakeQuantizedTile(InValue.Z, InQuantization));
	}

	CORE_API static FMatrix44f SafeCastMatrix(const FMatrix& Matrix);
	CORE_API static FMatrix44f MakeToRelativeWorldMatrix(const FVector Origin, const FMatrix& ToWorld);
	CORE_API static FMatrix    MakeToRelativeWorldMatrixDouble(const FVector Origin, const FMatrix& ToWorld);
	CORE_API static FMatrix44f MakeFromRelativeWorldMatrix(const FVector Origin, const FMatrix& FromWorld);
	CORE_API static FMatrix    MakeFromRelativeWorldMatrixDouble(const FVector Origin, const FMatrix& FromWorld);
	CORE_API static FMatrix44f MakeClampedToRelativeWorldMatrix(const FVector Origin, const FMatrix& ToWorld);
	CORE_API static FMatrix    MakeClampedToRelativeWorldMatrixDouble(const FVector Origin, const FMatrix& ToWorld);

	float GetTile() const { return (float)Tile; }
	float GetOffset() const { return (float)Offset; }
	double GetTileAsDouble() const { return Tile; }
	double GetOffsetAsDouble() const { return Offset; }
	double GetTileOffset() const { return Tile * GetTileSize(); }
	double GetAbsolute() const { return GetTileOffset() + Offset; }

	FLargeWorldRenderScalar() : Tile(0.0), Offset(0.0) {}
	FLargeWorldRenderScalar(float InTile, float InOffset) : Tile(InTile), Offset(InOffset) {}

	FLargeWorldRenderScalar(double InAbsolute)
	{
		// Tiles are centered on the origin
		Tile = MakeTile(InAbsolute);
		Offset = InAbsolute - GetTileOffset();
		Validate(InAbsolute);
	}

private:
	CORE_API void Validate(double InAbsolute);

	double Tile;
	double Offset;
};

struct FLargeWorldRenderPosition
{
public:
	FVector3f GetTile() const { return FVector3f(X.GetTile(), Y.GetTile(), Z.GetTile()); }
	FVector GetTileOffset() const { return FVector(X.GetTileOffset(), Y.GetTileOffset(), Z.GetTileOffset()); }
	FVector3f GetOffset() const { return FVector3f(X.GetOffset(), Y.GetOffset(), Z.GetOffset()); }
	FVector GetAbsolute() const { return FVector(X.GetAbsolute(), Y.GetAbsolute(), Z.GetAbsolute()); }

	explicit FLargeWorldRenderPosition(const FVector3f& InWorldPosition)
		: X(InWorldPosition.X)
		, Y(InWorldPosition.Y)
		, Z(InWorldPosition.Z)
	{}
	
	explicit FLargeWorldRenderPosition(const FVector3d& InWorldPosition)
		: X(InWorldPosition.X)
		, Y(InWorldPosition.Y)
		, Z(InWorldPosition.Z)
	{}

	explicit FLargeWorldRenderPosition(const FVector4d& InWorldPosition)
		: X(InWorldPosition.X)
		, Y(InWorldPosition.Y)
		, Z(InWorldPosition.Z)
	{}

	explicit FLargeWorldRenderPosition(const FVector3f& InTilePosition, const FVector3f& InRelativePosition)
		: X(InTilePosition.X, InRelativePosition.X)
		, Y(InTilePosition.Y, InRelativePosition.Y)
		, Z(InTilePosition.Z, InRelativePosition.Z)
	{}

private:
	FLargeWorldRenderScalar X;
	FLargeWorldRenderScalar Y;
	FLargeWorldRenderScalar Z;
};
