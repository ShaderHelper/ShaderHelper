// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/IsFloatingPoint.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/IsSigned.h"

/**
 * This file defines a few macros which can be used to restrict (via deprecation) ambiguous calls to functions expecting float params/return type.
 * This also handles mixing of argument types where at least one argument is a float/double and the rest can be promoted to the highest-precision type to infer the expected
 * type of the return value, or where non-float arguments are mixed in a function where the result should be the result of mixed arithmetic between the types.
 * 
 * For example:
 *  
 *  template< class T >
 *	static T MyFunc(const T X, const T Y)
 *	{
 *		return X < Y ? X : Y;
 *	}
 *  // Allows mixing of types without causing type ambiguity
 *  MIX_TYPES_2_ARGS(MyFunc);
 * 
 * 	static float Sin( float Value ) { return sinf(Value); }
 *	static double Sin( double Value ) { return sin(Value); }
 *  // Disallows any other type for the 'Value' argument, such as Sin(int32).
 *	RESOLVE_FLOAT_AMBIGUITY(Sin);
 *
 *	static float Fmod(float X, float Y);
 *	static double Fmod(double X, double Y);
 *  // Disallows Fmod(int32, int32) because one argument must be a floating point type.
 *  // Promotes calls such as "Fmod(float, double)" or "Fmod(double, int)" to "double Fmod(double, double)" because double has the highest precision of the argument types.
 *	RESOLVE_FLOAT_AMBIGUITY_2_ARGS(Fmod);
 */


// Assuming that a float and/or double version of a function are declared, this deprecates all other types as ambiguous and falls back to the float variant.
#define RESOLVE_FLOAT_AMBIGUITY(Func) \
	template<typename T> UE_DEPRECATED(5.0, "Arguments cause function resolution ambiguity.") \
	static FORCEINLINE float Func(T&& Value) { return Func((float)Value); }


// Mixing any types with any other type results in same type as "X * Y", which is promoted to the type of the result of mixed arithmetic between the types
#define MIX_SIGNED_TYPES_2_ARGS_ACTUAL(Func, OptionalMarkup) \
	template<typename Arg1, typename Arg2, TEMPLATE_REQUIRES(TAnd< TIsSigned<Arg1>, TIsSigned<Arg2>, TNot<TIsSame<Arg1, Arg2>> >::Value)> \
	static OptionalMarkup FORCEINLINE auto Func(Arg1 X, Arg2 Y) -> decltype(X * Y) \
	{ \
		using ArgType = decltype(X * Y); \
		return Func((ArgType)X, (ArgType)Y); \
	}

#define MIX_SIGNED_TYPES_2_ARGS(Func)				MIX_SIGNED_TYPES_2_ARGS_ACTUAL(Func,)
#define MIX_SIGNED_TYPES_2_ARGS_CONSTEXPR(Func)		MIX_SIGNED_TYPES_2_ARGS_ACTUAL(Func, CONSTEXPR)

// Mixing any types with any other type results in same type as "X * Y", which is promoted to the type of the result of mixed arithmetic between the types
#define MIX_SIGNED_TYPES_3_ARGS_ACTUAL(Func, OptionalMarkup) \
	template<typename Arg1, typename Arg2, typename Arg3, TEMPLATE_REQUIRES(TAnd< \
																				TOr< \
																					TNot<TIsSame<Arg1, Arg2>>, \
																					TNot<TIsSame<Arg2, Arg3>>, \
																					TNot<TIsSame<Arg1, Arg3>> \
																					>, \
																				TIsSigned<Arg1>, \
																				TIsSigned<Arg2>, \
																				TIsSigned<Arg3> \
																				>::Value)> \
	static OptionalMarkup FORCEINLINE auto Func(Arg1 X, Arg2 Y, Arg3 Z) -> decltype(X * Y * Z) \
	{ \
		using ArgType = decltype(X * Y * Z); \
		return Func((ArgType)X, (ArgType)Y, (ArgType)Z); \
	}

#define MIX_SIGNED_TYPES_3_ARGS(Func)				MIX_SIGNED_TYPES_3_ARGS_ACTUAL(Func,)
#define MIX_SIGNED_TYPES_3_ARGS_CONSTEXPR(Func)		MIX_SIGNED_TYPES_3_ARGS_ACTUAL(Func, CONSTEXPR)


// Cannot know whether the type of e.g. Fmod(int, int) should be float or double.
#define REMOVE_AMBIGUOUS_FLOAT_2_ARGS(Func) \
	template<typename Arg1, typename Arg2, TEMPLATE_REQUIRES(TAnd< \
																TNot<TIsFloatingPoint<Arg1>>,\
																TNot<TIsFloatingPoint<Arg2>> \
																>::Value)> UE_DEPRECATED(5.0, "Arguments cause function resolution ambiguity, expected a floating point type.") \
	static FORCEINLINE float Func(Arg1 X, Arg2 Y) \
	{ \
		return Func((float)X, (float)Y); \
	}

// Mixing float and double types with any other type results in same type as "X * Y", which is promoted to the floating point type with the highest precision of the argument types.
#define MIX_FLOATS_2_ARGS(Func) \
	template<typename Arg1, typename Arg2, TEMPLATE_REQUIRES(TAnd< \
																TOr<TIsFloatingPoint<Arg1>, TIsFloatingPoint<Arg2>>, \
																TNot<TIsSame<Arg1, Arg2>> \
																>::Value \
															)> \
	static FORCEINLINE auto Func(Arg1 X, Arg2 Y) -> decltype(X * Y) \
	{ \
		using ArgType = decltype(X * Y); \
		return Func((ArgType)X, (ArgType)Y); \
	}

// Cannot know whether the type of e.g. Fmod(int, int) should be float or double.
#define REMOVE_AMBIGUOUS_FLOAT_3_ARGS(Func) \
	template<typename Arg1, typename Arg2, typename Arg3, TEMPLATE_REQUIRES(TAnd< \
																				TNot<TIsFloatingPoint<Arg1>>, \
																				TNot<TIsFloatingPoint<Arg2>>, \
																				TNot<TIsFloatingPoint<Arg3>> \
																				>::Value \
																			)> UE_DEPRECATED(5.0, "Arguments cause function resolution ambiguity, expected a floating point type.") \
	static FORCEINLINE float Func(Arg1 X, Arg2 Y) \
	{ \
		return Func((float)X, (float)Y); \
	}

// Mixing float and double types with any other type results in same type as "X * Y", which is promoted to the floating point type with the highest precision of the argument types.
#define MIX_FLOATS_3_ARGS(Func) \
	template<typename Arg1, typename Arg2, typename Arg3, TEMPLATE_REQUIRES(TAnd< \
																				TOr< \
																					TIsFloatingPoint<Arg1>, \
																					TIsFloatingPoint<Arg2>, \
																					TIsFloatingPoint<Arg3> \
																					>, \
																				TOr< \
																					TNot<TIsSame<Arg1, Arg2>>, \
																					TNot<TIsSame<Arg2, Arg3>>, \
																					TNot<TIsSame<Arg1, Arg3>> \
																					> \
																				>::Value \
																			)> \
	static FORCEINLINE auto Func(Arg1 X, Arg2 Y, Arg3 Z) -> decltype(X * Y * Z) \
	{ \
		using ArgType = decltype(X * Y * Z); \
		return Func((ArgType)X, (ArgType)Y, (ArgType)Z); \
	}

// Resolve ambiguity between multiple arguments to functions returning a floating point type.
// If no arguments are floating point, emits a warning and resolves to float by default.
// Otherwise promotes to the highest precision floating point type of the arugments.

#define RESOLVE_FLOAT_AMBIGUITY_2_ARGS(Func) \
	REMOVE_AMBIGUOUS_FLOAT_2_ARGS(Func); \
	MIX_FLOATS_2_ARGS(Func);

#define RESOLVE_FLOAT_AMBIGUITY_3_ARGS(Func) \
	REMOVE_AMBIGUOUS_FLOAT_3_ARGS(Func); \
	MIX_FLOATS_3_ARGS(Func);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Assuming that a float and/or double version of a function are declared, this deprecates all other types as ambiguous and falls back to the float variant (returning ReturnType type)
#define RESOLVE_FLOAT_TO_TYPE_AMBIGUITY(Func, ReturnType) \
	template<typename T> UE_DEPRECATED(5.0, "Arguments cause function resolution ambiguity.") \
	static FORCEINLINE ReturnType Func(T&& Value) { return Func((float)Value); }

#define REMOVE_AMBIGUOUS_FLOAT_TO_TYPE_2_ARGS(Func, ReturnType) \
	template<typename Arg1, typename Arg2, TEMPLATE_REQUIRES(TAnd< \
																TNot<TIsFloatingPoint<Arg1>>,\
																TNot<TIsFloatingPoint<Arg2>> \
																>::Value)> UE_DEPRECATED(5.0, "Arguments cause function resolution ambiguity, expected a floating point type.") \
	static FORCEINLINE ReturnType Func(Arg1 X, Arg2 Y) \
	{ \
		return Func((float)X, (float)Y); \
	}

// Mixing float and double types with any other type results in same type as "X * Y", which is promoted to the floating point type with the highest precision of the argument types.
#define MIX_FLOATS_TO_TYPE_2_ARGS(Func, ReturnType) \
	template<typename Arg1, typename Arg2, TEMPLATE_REQUIRES(TAnd< \
																TOr<TIsFloatingPoint<Arg1>, TIsFloatingPoint<Arg2>>, \
																TNot<TIsSame<Arg1, Arg2>> \
																>::Value \
															)> \
	static FORCEINLINE ReturnType Func(Arg1 X, Arg2 Y) \
	{ \
		using ArgType = decltype(X * Y); \
		return Func((ArgType)X, (ArgType)Y); \
	}

// Cannot know whether the type of e.g. Fmod(int, int) should be float or double.
#define REMOVE_AMBIGUOUS_FLOAT_TO_TYPE_3_ARGS(Func, ReturnType) \
	template<typename Arg1, typename Arg2, typename Arg3, TEMPLATE_REQUIRES(TAnd< \
																				TNot<TIsFloatingPoint<Arg1>>, \
																				TNot<TIsFloatingPoint<Arg2>>, \
																				TNot<TIsFloatingPoint<Arg3>> \
																				>::Value \
																			)> UE_DEPRECATED(5.0, "Arguments cause function resolution ambiguity, expected a floating point type.") \
	static FORCEINLINE ReturnType Func(Arg1 X, Arg2 Y) \
	{ \
		return Func((float)X, (float)Y); \
	}

// Mixing float and double types with any other type results in same type as "X * Y", which is promoted to the floating point type with the highest precision of the argument types.
#define MIX_FLOATS_TO_TYPE_3_ARGS(Func, ReturnType) \
	template<typename Arg1, typename Arg2, typename Arg3, TEMPLATE_REQUIRES(TAnd< \
																				TOr< \
																					TIsFloatingPoint<Arg1>, \
																					TIsFloatingPoint<Arg2>, \
																					TIsFloatingPoint<Arg3> \
																					>, \
																				TOr< \
																					TNot<TIsSame<Arg1, Arg2>>, \
																					TNot<TIsSame<Arg1, Arg3>>, \
																					TNot<TIsSame<Arg2, Arg3>> \
																					> \
																				>::Value \
																			)> \
	static FORCEINLINE ReturnType Func(Arg1 X, Arg2 Y, Arg3 Z) \
	{ \
		using ArgType = decltype(X * Y * Z); \
		return Func((ArgType)X, (ArgType)Y, (ArgType)Z); \
	}

// Resolve ambiguity between multiple arguments to functions returning a generic point type.
// If no arguments are floating point, emits a warning and resolves to float by default.
// Otherwise promotes to the highest precision floating point type of the arugments.

#define RESOLVE_FLOAT_TO_TYPE_AMBIGUITY_2_ARGS(Func, ReturnType) \
	REMOVE_AMBIGUOUS_FLOAT_TO_TYPE_2_ARGS(Func, ReturnType); \
	MIX_FLOATS_TO_TYPE_2_ARGS(Func, ReturnType);

#define RESOLVE_FLOAT_TO_TYPE_AMBIGUITY_3_ARGS(Func, ReturnType) \
	REMOVE_AMBIGUOUS_FLOAT_TO_TYPE_3_ARGS(Func, ReturnType); \
	MIX_FLOATS_TO_TYPE_3_ARGS(Func, ReturnType);

// nicer names for the bool version
#define RESOLVE_FLOAT_PREDICATE_AMBIGUITY(Func) RESOLVE_FLOAT_TO_TYPE_AMBIGUITY(Func, bool)
#define RESOLVE_FLOAT_PREDICATE_AMBIGUITY_2_ARGS(Func) RESOLVE_FLOAT_TO_TYPE_AMBIGUITY_2_ARGS(Func, bool)
#define RESOLVE_FLOAT_PREDICATE_AMBIGUITY_3_ARGS(Func) RESOLVE_FLOAT_TO_TYPE_AMBIGUITY_3_ARGS(Func, bool)
