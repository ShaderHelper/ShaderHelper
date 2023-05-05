#pragma once
#include "mtlpp.hpp"
#import <Metal/Metal.h>

DECLARE_LOG_CATEGORY_EXTERN(LogMetal, Log, All);
inline DEFINE_LOG_CATEGORY(LogMetal);

//func(ConverOcError(Arg)); (√)
//const TCHAR* XX = ConverOcError(Arg); func(XX); (×)
inline const TCHAR* ConvertOcError(NSError* Error)
{
    NSString* ErrorInfo = Error.localizedDescription;
    const ANSICHAR* ErrorInfo_C = [ErrorInfo cStringUsingEncoding:NSUTF8StringEncoding];
    return ANSI_TO_TCHAR(ErrorInfo_C);
}


