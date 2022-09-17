// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if HACK_HEADER_GENERATOR
	#include "Templates/IsValidVariadicFunctionArg.h"
	#include "Templates/AndOrNot.h"

	/** 
	 * FError
	 * Set of functions for error reporting 
	 **/
	struct COREUOBJECT_API FError
	{
		/**
		 * Throws a printf-formatted exception as a const TCHAR*.
		 */
		template <typename... Types>
		UE_NORETURN static void VARARGS Throwf(const TCHAR* Fmt, Types... Args)
		{
			static_assert(TAnd<TIsValidVariadicFunctionArg<Types>...>::Value, "Invalid argument(s) passed to FError::Throwf");

			ThrowfImpl(Fmt, Args...);
		}

	private:
		UE_NORETURN static void VARARGS ThrowfImpl(const TCHAR* Fmt, ...);
	};
#endif


