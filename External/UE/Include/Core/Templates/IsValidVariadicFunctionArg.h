// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "IsEnum.h"

/**
 * Tests if a type is a valid argument to a variadic function, e.g. printf.
 */
template <typename T, bool = TIsEnum<T>::Value>
struct TIsValidVariadicFunctionArg
{
private:
	static uint32 Tester(uint32);
	static uint32 Tester(uint8);
	static uint32 Tester(int32);
	static uint32 Tester(uint64);
	static uint32 Tester(int64);
	static uint32 Tester(double);
	static uint32 Tester(long);
	static uint32 Tester(unsigned long);
	static uint32 Tester(TCHAR);
	static uint32 Tester(bool);
	static uint32 Tester(const void*);
	static uint8  Tester(...);

	static T DeclValT();

public:
	enum { Value = sizeof(Tester(DeclValT())) == sizeof(uint32) };
};

template <typename T>
struct TIsValidVariadicFunctionArg<T, true>
{
	enum { Value = true };
};
