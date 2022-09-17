// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	ApplePlatformString.h: Apple platform string classes, mostly implemented with ANSI C++
==============================================================================================*/

#pragma once

#include "CoreTypes.h"
#if PLATFORM_TCHAR_IS_CHAR16
#include "GenericPlatform/GenericWidePlatformString.h"
#else
#include "GenericPlatform/StandardPlatformString.h"
#endif
#include "Misc/AssertionMacros.h"
#include "Misc/OutputDevice.h"

#if PLATFORM_MAC
#include "Mac/MacSystemIncludes.h"
#elif PLATFORM_IOS
#include "IOS/IOSSystemIncludes.h"
#endif

#ifdef __OBJC__
#import <Foundation/NSString.h>

class FString;

@interface NSString (FString_Extensions)

/**
 * Converts an TCHAR string to an NSString
 */
+ (NSString*) stringWithTCHARString:(const TCHAR*)MyTCHARString;

/**
 * Converts an FString to an NSString
 */
+ (NSString*) stringWithFString:(const FString&)MyFString;

@end
#endif


/**
* 2-byte replacement s string implementation
**/
struct FApplePlatformString
#if PLATFORM_TCHAR_IS_CHAR16
	: public FGenericWidePlatformString
#else
	: public FStandardPlatformString
#endif
{
	// These should be replaced with equivalent Convert and ConvertedLength functions when we properly support variable-length encodings.
/*	static void WideCharToMultiByte(const TCHAR *Source, uint32 LengthWM1, ANSICHAR *Dest, uint32 LengthA)
	{
		CFStringRef CFSource = CFStringCreateWithBytes( kCFAllocatorDefault, ( const uint8 *)Source, LengthWM1 * sizeof( TCHAR ), kCFStringEncodingUTF32LE, false );
		check(CFSource);
		const SIZE_T CFSourceLength = CFStringGetLength( CFSource );
		LengthWM1 = LengthWM1 < CFSourceLength ? LengthWM1 : CFSourceLength;
		CFRange Range = CFRangeMake( 0, LengthWM1 );
		CFIndex ActualLength = 0;
		CFIndex Converted = CFStringGetBytes( CFSource, Range, kCFStringEncodingUTF8, '?', false, NULL, LengthA, &ActualLength );
		check(Converted == LengthWM1 && ActualLength <= LengthA);
		CFStringGetBytes( CFSource, Range, kCFStringEncodingUTF8, '?', false, ( uint8 *)Dest, LengthA, &ActualLength );
		Dest[ActualLength] = 0;
		CFRelease( CFSource );
	}
	static void MultiByteToWideChar(const ANSICHAR *Source, TCHAR *Dest, uint32 LengthM1)
	{
		CFStringRef CFSource = CFStringCreateWithBytes( kCFAllocatorDefault, ( const uint8 *)Source, LengthM1, kCFStringEncodingUTF8, false );
		check(CFSource);
		const SIZE_T CFSourceLength = CFStringGetLength( CFSource );
		LengthM1 = LengthM1 < CFSourceLength ? LengthM1 : CFSourceLength;
		CFRange Range = CFRangeMake( 0, LengthM1 );
		CFIndex ActualLength = 0;
		CFStringGetBytes( CFSource, Range, kCFStringEncodingUTF32LE, '?', false, NULL, LengthM1 * sizeof( TCHAR ), &ActualLength );
		check(ActualLength <= (LengthM1 * sizeof( TCHAR )));
		CFStringGetBytes( CFSource, Range, kCFStringEncodingUTF32LE, '?', false, ( uint8 *)Dest, LengthM1 * sizeof( TCHAR ), &ActualLength );
		uint8* Ptr = (uint8*)Dest;
		Ptr[ActualLength] = Ptr[ActualLength + 1] = Ptr[ActualLength + 2] = Ptr[ActualLength + 3] = 0;
		CFRelease( CFSource );
	}*/
	static void CFStringToTCHAR( CFStringRef CFStr, TCHAR *TChar )
	{
		const SIZE_T Length = CFStringGetLength( CFStr );
		CFRange Range = CFRangeMake( 0, Length );
		CFStringGetBytes( CFStr, Range, sizeof( TCHAR ) == 4 ? kCFStringEncodingUTF32LE : kCFStringEncodingUnicode, '?', false, ( uint8 *)TChar, Length * sizeof( TCHAR ) + 1, NULL );
		TChar[Length] = 0;
	}
	static CFStringRef TCHARToCFString( const TCHAR *TChar )
	{
		const SIZE_T Length = Strlen( TChar );
		CFStringRef String = CFStringCreateWithBytes( kCFAllocatorDefault, ( const uint8 *)TChar, Length * sizeof( TCHAR ), sizeof( TCHAR ) == 4 ? kCFStringEncodingUTF32LE : kCFStringEncodingUnicode, false );
		// we are getting some crashes on strings that aren't converting - so, instead of crashing, print them out instead
		ensureMsgf(String, TEXT("Failed to allocated CFString for '%s' -- Length id %d"), TChar ? TChar : TEXT("nullptr"), Length);
		return String;
	}
};

typedef FApplePlatformString FPlatformString;

// Format specifiers to be able to print values of these types correctly, for example when using UE_LOG.
// SIZE_T format specifier
#define SIZE_T_FMT "zu"
// SIZE_T format specifier for lowercase hexadecimal output
#define SIZE_T_x_FMT "zx"
// SIZE_T format specifier for uppercase hexadecimal output
#define SIZE_T_X_FMT "zX"

// SSIZE_T format specifier
#define SSIZE_T_FMT "lld"
// SSIZE_T format specifier for lowercase hexadecimal output
#define SSIZE_T_x_FMT "llx"
// SSIZE_T format specifier for uppercase hexadecimal output
#define SSIZE_T_X_FMT "llX"

// PTRINT format specifier for decimal output
#define PTRINT_FMT SSIZE_T_FMT
// PTRINT format specifier for lowercase hexadecimal output
#define PTRINT_x_FMT SSIZE_T_x_FMT
// PTRINT format specifier for uppercase hexadecimal output
#define PTRINT_X_FMT SSIZE_T_X_FMT

// UPTRINT format specifier for decimal output
#define UPTRINT_FMT "llu"
// UPTRINT format specifier for lowercase hexadecimal output
#define UPTRINT_x_FMT "llx"
// UPTRINT format specifier for uppercase hexadecimal output
#define UPTRINT_X_FMT "llX"

// int64 format specifier for decimal output
#define INT64_FMT SSIZE_T_FMT
// int64 format specifier for lowercase hexadecimal output
#define INT64_x_FMT SSIZE_T_x_FMT
// int64 format specifier for uppercase hexadecimal output
#define INT64_X_FMT SSIZE_T_X_FMT

// uint64 format specifier for decimal output
#define UINT64_FMT "llu"
// uint64 format specifier for lowercase hexadecimal output
#define UINT64_x_FMT "llx"
// uint64 format specifier for uppercase hexadecimal output
#define UINT64_X_FMT "llX"
