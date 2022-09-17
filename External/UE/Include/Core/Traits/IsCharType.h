// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

/**
 * Type trait which tests if a type is a char type.
 */
template<typename T> struct TIsCharType            { enum { Value = false }; };
template<>           struct TIsCharType<ANSICHAR>  { enum { Value = true  }; };
template<>           struct TIsCharType<UCS2CHAR>  { enum { Value = true  }; };
#if !PLATFORM_UCS2CHAR_IS_UTF16CHAR
template<>           struct TIsCharType<UTF16CHAR> { enum { Value = true  }; };
#endif
template<>           struct TIsCharType<WIDECHAR>  { enum { Value = true  }; };
template<>           struct TIsCharType<UTF8CHAR>  { enum { Value = true  }; };
template<>           struct TIsCharType<UTF32CHAR> { enum { Value = true  }; };
#if PLATFORM_TCHAR_IS_CHAR16
template<>           struct TIsCharType<wchar_t>   { enum { Value = true  }; };
#endif
