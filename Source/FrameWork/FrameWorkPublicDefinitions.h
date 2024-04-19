#pragma once
//Other modules should include the header when depending on the module.

#if defined(_MSC_VER)
//Stupid IntelliSense warnings
#pragma warning(disable:26812)
#pragma warning(disable:26439)
#pragma warning(disable:26444)
#pragma warning(disable:26450)
#pragma warning(disable:26451)
#pragma warning(disable:26495)
#endif

#if defined(_WIN64)
#include <UnrealDefinitionsWin.h>
#elif defined(__APPLE__)
#include <UnrealDefinitionsMac.h>
#endif

#include <SharedPCH.h>
#include "Common/Util/SwizzleVector.h"
#include "Common/Util/Auxiliary.h"
#include "Common/Util/Singleton.h"
#include "Common/Util/Reflection.h"

#define PER_MODULE_DEFINITION() \
    PER_MODULE_BOILERPLATE

namespace FRAMEWORK
{
    extern FRAMEWORK_API TArray<FName> GProjectCategoryNames;
}

#define SH_LOG(CategoryName, Verbosity, Format, ...) \
    UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
    GProjectCategoryNames.Emplace(#CategoryName)

#define ADD_AGILITY_SDK()	\
	extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }	\
	extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = ".\\AgilitySDK\\"; }
