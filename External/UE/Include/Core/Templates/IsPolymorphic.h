// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* Traits class which tests if a type is polymorphic (virtual).
*/
template <typename T>
struct TIsPolymorphic
{
	enum { Value = __is_polymorphic(T) };
};
