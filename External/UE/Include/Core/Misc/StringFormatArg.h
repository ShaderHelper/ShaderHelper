// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ContainersFwd.h"

/** An argument supplied to FString::Format */
struct CORE_API FStringFormatArg
{
	enum EType { Int, UInt, Double, String, StringLiteralANSI, StringLiteralWIDE, StringLiteralUCS2, StringLiteralUTF8 };

	/** The type of this arg */
	EType Type;

	/* todo: convert this to a TVariant */
	union
	{
		/** Value as integer */
		int64 IntValue;
		/** Value as uint */
		uint64 UIntValue;
		/** Value as double */
		double DoubleValue;
		/** Value as an ANSI string literal */
		const ANSICHAR* StringLiteralANSIValue;
		/** Value as a WIDE string literal */
		const WIDECHAR* StringLiteralWIDEValue;
		/** Value as a UCS2 string literal */
		const UCS2CHAR* StringLiteralUCS2Value;
		/** Value as a UTF8 string literal */
		const UTF8CHAR* StringLiteralUTF8Value;
	};

	/** Value as an FString */
	FString StringValue;

	FStringFormatArg( const int32 Value );
	FStringFormatArg( const uint32 Value );
	FStringFormatArg( const int64 Value );
	FStringFormatArg( const uint64 Value );
	FStringFormatArg( const float Value );
	FStringFormatArg( const double Value );
	FStringFormatArg( FString Value );
	FStringFormatArg( const ANSICHAR* Value );
	FStringFormatArg( const WIDECHAR* Value );
	FStringFormatArg( const UCS2CHAR* Value );
	FStringFormatArg( const UTF8CHAR* Value );

	/** Copyable */
	FStringFormatArg( const FStringFormatArg& RHS );
	
private:

	/** Not default constructible */
	FStringFormatArg();
};
