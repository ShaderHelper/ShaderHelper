#include "CommonHeader.h"

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

FString NSStringToFString(NS::String* InputString)
{
    TArray<TCHAR> Result;
    Result.AddUninitialized(InputString->length() + 1);
    FPlatformString::CFStringToTCHAR((CFStringRef)InputString, Result.GetData());
    return Result.GetData();
}

NS::String* FStringToNSString(const FString& InputString)
{
    return ((NS::String*)FPlatformString::TCHARToCFString(*InputString))->autorelease();
}