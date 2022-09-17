// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CityHash.h"

template<typename T>
uint64 AppendCityHash(const T& Value, uint64 Seed)
{
	return CityHash64WithSeed((const char*)&Value, sizeof(Value), Seed);
}
