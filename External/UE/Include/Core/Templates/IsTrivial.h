// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/AndOrNot.h"
#include "Templates/IsTriviallyDestructible.h"
#include "Templates/IsTriviallyCopyConstructible.h"
#include "Templates/IsTriviallyCopyAssignable.h"

/**
 * Traits class which tests if a type is trivial.
 */
template <typename T>
struct TIsTrivial
{
	enum { Value = TAnd<TIsTriviallyDestructible<T>, TIsTriviallyCopyConstructible<T>, TIsTriviallyCopyAssignable<T>>::Value };
};
