// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Traits class which tests if a type is const.
 */
template <typename T>
struct TIsConst
{
	static constexpr bool Value = false;
};

template <typename T>
struct TIsConst<const T>
{
	static constexpr bool Value = true;
};
