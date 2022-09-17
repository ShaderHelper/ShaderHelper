// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/Char.h"
#include "Misc/AssertionMacros.h"
#include "GenericPlatform/GenericPlatformStricmp.h"
#include "GenericPlatform/GenericPlatformString.h"
#include "HAL/PlatformCrt.h"

#if PLATFORM_USE_GENERIC_STRING_IMPLEMENTATION

/**
* Standard implementation
**/
struct FGenericWidePlatformString : public FGenericPlatformString
{
	using FGenericPlatformString::Stricmp;
	using FGenericPlatformString::Strncmp;
	using FGenericPlatformString::Strnicmp;

	template <typename CharType>
	static inline CharType* Strupr(CharType* Dest, SIZE_T DestCount)
	{
		for (CharType* Char = Dest; *Char && DestCount > 0; Char++, DestCount--)
		{
			*Char = TChar<CharType>::ToUpper(*Char);
		}
		return Dest;
	}

public:

	/**
	 * Unicode implementation
	 **/
	CORE_API static WIDECHAR* Strcpy(WIDECHAR* Dest, SIZE_T DestCount, const WIDECHAR* Src);
	CORE_API static WIDECHAR* Strncpy(WIDECHAR* Dest, const WIDECHAR* Src, SIZE_T MaxLen);
	CORE_API static WIDECHAR* Strcat(WIDECHAR* Dest, SIZE_T DestCount, const WIDECHAR* Src);

	CORE_API static int32 Strcmp( const WIDECHAR* String1, const WIDECHAR* String2 )
	{
		// walk the strings, comparing them case sensitively
		for (; *String1 || *String2; String1++, String2++)
		{
			WIDECHAR A = *String1, B = *String2;
			if (A != B)
			{
				return A - B;
			}
		}
		return 0;
	}

	CORE_API static int32 Strncmp( const WIDECHAR* String1, const WIDECHAR* String2, SIZE_T Count )
	{
		// walk the strings, comparing them case sensitively, up to a max size
		for (; (*String1 || *String2) && Count; String1++, String2++, Count--)
		{
			TCHAR A = *String1, B = *String2;
			if (A != B)
			{
				return A - B;
			}
		}
		return 0;
	}

	CORE_API static int32 Strlen( const WIDECHAR* String )
	{
		int32 Length = -1;

		do
		{
			Length++;
		}
		while (*String++);

		return Length;
	}

	CORE_API static int32 Strnlen( const WIDECHAR* String, SIZE_T StringSize )
	{
		int32 Length = -1;

		do
		{
			Length++;
		}
		while (StringSize-- > 0 && *String++);

		return Length;
	}

#if PLATFORM_TCHAR_IS_CHAR16
	static int32 Strlen( const wchar_t* String )
	{
		int32 Length = -1;

		do
		{
			Length++;
		}
		while (*String++);

		return Length;
	}

	static int32 Strnlen( const wchar_t* String, SIZE_T StringSize )
	{
		int32 Length = -1;

		do
		{
			Length++;
		}
		while (StringSize-- > 0 && *String++);

		return Length;
	}
#endif

	CORE_API static const WIDECHAR* Strstr( const WIDECHAR* String, const WIDECHAR* Find)
	{
		WIDECHAR Char1, Char2;
		if ((Char1 = *Find++) != 0)
		{
			size_t Length = Strlen(Find);
			
			do
			{
				do
				{
					if ((Char2 = *String++) == 0)
					{
						return nullptr;
					}
				}
				while (Char1 != Char2);
			}
			while (Strncmp(String, Find, Length) != 0);
			
			String--;
		}
		
		return String;
	}

	CORE_API static const WIDECHAR* Strchr( const WIDECHAR* String, WIDECHAR C)
	{
		while (*String != C && *String != 0)
		{
			String++;
		}
		
		return (*String == C) ? String : nullptr;
	}

	CORE_API static const WIDECHAR* Strrchr( const WIDECHAR* String, WIDECHAR C)
	{
		const WIDECHAR *Last = nullptr;
		
		while (true)
		{
			if (*String == C)
			{
				Last = String;
			}
			
			if (*String == 0)
			{
				break;
			}
			
			String++;
		}
		
		return Last;
	}

	CORE_API static int32 Strtoi(const WIDECHAR* Start, WIDECHAR** End, int32 Base);
	CORE_API static int64 Strtoi64(const WIDECHAR* Start, WIDECHAR** End, int32 Base);

	CORE_API static uint64 Strtoui64( const WIDECHAR* Start, WIDECHAR** End, int32 Base );
	CORE_API static float Atof(const WIDECHAR* String);
	CORE_API static double Atod(const WIDECHAR* String);

	CORE_API static FORCEINLINE int32 Atoi(const WIDECHAR* String)
	{
		return Strtoi( String, NULL, 10 );
	}
	
	CORE_API static FORCEINLINE int64 Atoi64(const WIDECHAR* String)
	{
		return Strtoi64( String, NULL, 10 );
	}

	
	
	CORE_API static WIDECHAR* Strtok( WIDECHAR* StrToken, const WIDECHAR* Delim, WIDECHAR** Context );

	CORE_API static int32 GetVarArgs(WIDECHAR* Dest, SIZE_T DestSize, const WIDECHAR*& Fmt, va_list ArgPtr);

	/**
	 * Ansi implementation
	 **/
	CORE_API static FORCEINLINE ANSICHAR* Strcpy(ANSICHAR* Dest, SIZE_T DestCount, const ANSICHAR* Src)
	{
// Skip suggestions about using strcpy_s instead.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return strcpy( Dest, Src );
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	CORE_API static FORCEINLINE ANSICHAR* Strncpy(ANSICHAR* Dest, const ANSICHAR* Src, SIZE_T MaxLen)
	{
// Skip suggestions about using strncpy_s instead.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		::strncpy(Dest, Src, MaxLen);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		Dest[MaxLen-1]=0;
		return Dest;
	}

	CORE_API static FORCEINLINE ANSICHAR* Strcat(ANSICHAR* Dest, SIZE_T DestCount, const ANSICHAR* Src)
	{
// Skip suggestions about using strcat_s instead.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return strcat( Dest, Src );
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	CORE_API static FORCEINLINE int32 Strcmp( const ANSICHAR* String1, const ANSICHAR* String2 )
	{
		return strcmp(String1, String2);
	}

	CORE_API static FORCEINLINE int32 Strncmp( const ANSICHAR* String1, const ANSICHAR* String2, SIZE_T Count )
	{
		return strncmp( String1, String2, Count );
	}

	CORE_API static FORCEINLINE int32 Strlen( const ANSICHAR* String )
	{
		return strlen( String );
	}

	CORE_API static FORCEINLINE int32 Strnlen( const ANSICHAR* String, SIZE_T StringSize )
	{
		return strnlen( String, StringSize );
	}

	CORE_API static FORCEINLINE const ANSICHAR* Strstr( const ANSICHAR* String, const ANSICHAR* Find)
	{
		return strstr(String, Find);
	}

	CORE_API static FORCEINLINE const ANSICHAR* Strchr( const ANSICHAR* String, ANSICHAR C)
	{
		return strchr(String, C);
	}

	CORE_API static FORCEINLINE const ANSICHAR* Strrchr( const ANSICHAR* String, ANSICHAR C)
	{
		return strrchr(String, C);
	}

	CORE_API static FORCEINLINE int32 Atoi(const ANSICHAR* String)
	{
		return atoi( String );
	}

	CORE_API static FORCEINLINE int64 Atoi64(const ANSICHAR* String)
	{
		return strtoll( String, NULL, 10 );
	}

	CORE_API static FORCEINLINE float Atof(const ANSICHAR* String)
	{
		return (float)atof( String );
	}

	CORE_API static FORCEINLINE double Atod(const ANSICHAR* String)
	{
		return atof( String );
	}

	CORE_API static FORCEINLINE int32 Strtoi( const ANSICHAR* Start, ANSICHAR** End, int32 Base )
	{
		return strtol( Start, End, Base );
	}

	CORE_API static FORCEINLINE int64 Strtoi64( const ANSICHAR* Start, ANSICHAR** End, int32 Base )
	{
		return strtoll(Start, End, Base);
	}

	CORE_API static FORCEINLINE uint64 Strtoui64( const ANSICHAR* Start, ANSICHAR** End, int32 Base )
	{
		return strtoull(Start, End, Base);
	}

	CORE_API static FORCEINLINE ANSICHAR* Strtok(ANSICHAR* StrToken, const ANSICHAR* Delim, ANSICHAR** Context)
	{
// Skip suggestions about using strtok_s instead.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return strtok(StrToken, Delim);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	static CORE_API int32 GetVarArgs( ANSICHAR* Dest, SIZE_T DestSize, const ANSICHAR*& Fmt, va_list ArgPtr )
	{
		int32 Result = vsnprintf(Dest, DestSize, Fmt, ArgPtr);
		va_end( ArgPtr );
		return (Result != -1 && Result < (int32)DestSize) ? Result : -1;
	}

	/**
	 * UTF8CHAR implementation.
	 */
	static FORCEINLINE UTF8CHAR* Strcpy(UTF8CHAR* Dest, SIZE_T DestCount, const UTF8CHAR* Src)
	{
		return (UTF8CHAR*)Strcpy((ANSICHAR*)Dest, DestCount, (const ANSICHAR*)Src);
	}

	static FORCEINLINE UTF8CHAR* Strncpy(UTF8CHAR* Dest, const UTF8CHAR* Src, SIZE_T MaxLen)
	{
		return (UTF8CHAR*)Strncpy((ANSICHAR*)Dest, (const ANSICHAR*)Src, MaxLen);
	}

	static FORCEINLINE UTF8CHAR* Strcat(UTF8CHAR* Dest, SIZE_T DestCount, const UTF8CHAR* Src)
	{
		return (UTF8CHAR*)Strcat((ANSICHAR*)Dest, DestCount, (const ANSICHAR*)Src);
	}

	static FORCEINLINE int32 Strcmp(const UTF8CHAR* String1, const UTF8CHAR* String2)
	{
		return Strcmp((const ANSICHAR*)String1, (const ANSICHAR*)String2);
	}

	static FORCEINLINE int32 Strncmp(const UTF8CHAR* String1, const UTF8CHAR* String2, SIZE_T Count)
	{
		return Strncmp((const ANSICHAR*)String1, (const ANSICHAR*)String2, Count);
	}

	static FORCEINLINE int32 Strlen(const UTF8CHAR* String)
	{
		return Strlen((const ANSICHAR*)String);
	}

	static FORCEINLINE int32 Strnlen(const UTF8CHAR* String, SIZE_T StringSize)
	{
		return Strnlen((const ANSICHAR*)String, StringSize);
	}

	static FORCEINLINE const UTF8CHAR* Strstr(const UTF8CHAR* String, const UTF8CHAR* Find)
	{
		return (const UTF8CHAR*)Strstr((const ANSICHAR*)String, (const ANSICHAR*)Find);
	}

	static FORCEINLINE const UTF8CHAR* Strchr(const UTF8CHAR* String, UTF8CHAR C)
	{
		return (const UTF8CHAR*)Strchr((const ANSICHAR*)String, (ANSICHAR)C);
	}

	static FORCEINLINE const UTF8CHAR* Strrchr(const UTF8CHAR* String, UTF8CHAR C)
	{
		return (const UTF8CHAR*)Strrchr((const ANSICHAR*)String, (ANSICHAR)C);
	}

	static FORCEINLINE int32 Atoi(const UTF8CHAR* String)
	{
		return Atoi((const ANSICHAR*)String);
	}

	static FORCEINLINE int64 Atoi64(const UTF8CHAR* String)
	{
		return Atoi64((const ANSICHAR*)String);
	}

	static FORCEINLINE float Atof(const UTF8CHAR* String)
	{
		return Atof((const ANSICHAR*)String);
	}

	static FORCEINLINE double Atod(const UTF8CHAR* String)
	{
		return Atod((const ANSICHAR*)String);
	}

	static FORCEINLINE int32 Strtoi(const UTF8CHAR* Start, UTF8CHAR** End, int32 Base)
	{
		return Strtoi((const ANSICHAR*)Start, (ANSICHAR**)End, Base);
	}

	static FORCEINLINE int64 Strtoi64(const UTF8CHAR* Start, UTF8CHAR** End, int32 Base)
	{
		return Strtoi64((const ANSICHAR*)Start, (ANSICHAR**)End, Base);
	}

	static FORCEINLINE uint64 Strtoui64(const UTF8CHAR* Start, UTF8CHAR** End, int32 Base)
	{
		return Strtoui64((const ANSICHAR*)Start, (ANSICHAR**)End, Base);
	}

	static FORCEINLINE UTF8CHAR* Strtok(UTF8CHAR* StrToken, const UTF8CHAR* Delim, UTF8CHAR** Context)
	{
		return (UTF8CHAR*)Strtok((ANSICHAR*)StrToken, (const ANSICHAR*)Delim, (ANSICHAR**)Context);
	}

	static FORCEINLINE int32 GetVarArgs(UTF8CHAR* Dest, SIZE_T DestSize, const UTF8CHAR*& Fmt, va_list ArgPtr)
	{
		return GetVarArgs((ANSICHAR*)Dest, DestSize, *(const ANSICHAR**)&Fmt, ArgPtr);
	}

	/**
	 * UCS2 implementation
	 **/

	CORE_API static FORCEINLINE int32 Strlen( const UCS2CHAR* String )
	{
		int32 Result = 0;
		while (*String++)
		{
			++Result;
		}

		return Result;
	}

	CORE_API static FORCEINLINE int32 Strnlen( const UCS2CHAR* String, SIZE_T StringSize )
	{
		int32 Result = 0;
		while (StringSize-- > 0 && *String++)
		{
			++Result;
		}

		return Result;
	}
};

#endif
