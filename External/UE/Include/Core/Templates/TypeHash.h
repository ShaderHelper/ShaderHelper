// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/IsEnum.h"
#include "Misc/Crc.h"

/**
 * Combines two hash values to get a third.
 * Note - this function is not commutative.
 *
 * This function cannot change for backward compatibility reasons.
 * You may want to choose HashCombineFast for a better in-memory hash combining function.
 */
inline uint32 HashCombine(uint32 A, uint32 C)
{
	uint32 B = 0x9e3779b9;
	A += B;

	A -= B; A -= C; A ^= (C>>13);
	B -= C; B -= A; B ^= (A<<8);
	C -= A; C -= B; C ^= (B>>13);
	A -= B; A -= C; A ^= (C>>12);
	B -= C; B -= A; B ^= (A<<16);
	C -= A; C -= B; C ^= (B>>5);
	A -= B; A -= C; A ^= (C>>3);
	B -= C; B -= A; B ^= (A<<10);
	C -= A; C -= B; C ^= (B>>15);

	return C;
}

/**
 * Combines two hash values to get a third.
 * Note - this function is not commutative.
 *
 * WARNING!  This function is subject to change and should only be used for creating
 *           combined hash values which don't leave the running process,
 *           e.g. GetTypeHash() overloads.
 */
inline uint32 HashCombineFast(uint32 A, uint32 B)
{
	// Currently call HashCombine because it is exists, but only as a placeholder until it
	// can be replaced with something better
	return HashCombine(A, B);
}

inline uint32 PointerHash(const void* Key,uint32 C = 0)
{
#if PLATFORM_64BITS
	// Ignoring the lower 4 bits since they are likely zero anyway.
	// Higher bits are more significant in 64 bit builds.
	auto PtrInt = reinterpret_cast<UPTRINT>(Key) >> 4;
#else
	auto PtrInt = reinterpret_cast<UPTRINT>(Key);
#endif

	// we can use HashCombineFast here because pointers are non-persistent
	return HashCombineFast((uint32)PtrInt, C);
}


//
// Hash functions for common types.
//
// WARNING!  GetTypeHash result values are not expected to leave the running process.
//           Do not persist them to disk, send them to another running process or
//           expect them to be consistent across multiple runs.
//

inline uint32 GetTypeHash( const uint8 A )
{
	return A;
}

inline uint32 GetTypeHash( const int8 A )
{
	return A;
}

inline uint32 GetTypeHash( const uint16 A )
{
	return A;
}

inline uint32 GetTypeHash( const int16 A )
{
	return A;
}

inline uint32 GetTypeHash( const int32 A )
{
	return A;
}

inline uint32 GetTypeHash( const uint32 A )
{
	return A;
}

inline uint32 GetTypeHash( const uint64 A )
{
	return (uint32)A+((uint32)(A>>32) * 23);
}

inline uint32 GetTypeHash( const int64 A )
{
	return (uint32)A+((uint32)(A>>32) * 23);
}

#if PLATFORM_MAC
inline uint32 GetTypeHash( const __uint128_t A )
{
	uint64 Low = (uint64)A;
	uint64 High = (uint64)(A >> 64);
	return GetTypeHash(Low) ^ GetTypeHash(High);
}
#endif

#if defined(INT64_T_TYPES_NOT_LONG_LONG)
// int64_t and uint64_t are long types, not long long (aka int64/uint64). These types can't be automatically converted.
inline uint32 GetTypeHash(uint64_t A)
{
	return GetTypeHash((uint64)A);
}
inline uint32 GetTypeHash(int64_t A)
{
	return GetTypeHash((int64)A);
}
#endif

inline uint32 GetTypeHash( float Value )
{
	return *(uint32*)&Value;
}

inline uint32 GetTypeHash( double Value )
{
	return GetTypeHash(*(uint64*)&Value);
}

inline uint32 GetTypeHash( const TCHAR* S )
{
	return FCrc::Strihash_DEPRECATED(S);
}

inline uint32 GetTypeHash( const void* A )
{
	return PointerHash(A);
}

inline uint32 GetTypeHash( void* A )
{
	return PointerHash(A);
}

template <typename EnumType>
FORCEINLINE  typename TEnableIf<TIsEnum<EnumType>::Value, uint32>::Type GetTypeHash(EnumType E)
{
	return GetTypeHash((__underlying_type(EnumType))E);
}
