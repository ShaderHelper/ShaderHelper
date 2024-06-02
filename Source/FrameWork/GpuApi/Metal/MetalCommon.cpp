#include "CommonHeader.h"
#include "MetalCommon.h"
FString NSStringToFString(NS::String* InputString)
{
    TArray<TCHAR> Result;
    Result.AddUninitialized((int)InputString->length() + 1);
    FPlatformString::CFStringToTCHAR((CFStringRef)InputString, Result.GetData());
    return Result.GetData();
}

NS::String* FStringToNSString(const FString& InputString)
{
    return ((NS::String*)FPlatformString::TCHARToCFString(*InputString))->autorelease();
}
