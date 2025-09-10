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
#elif defined(__linux__)
#include <UnrealDefinitionsLinux.h>
#endif

#define DO_CHECK !SH_SHIPPING

//Ue Headers.
#include <SharedPCH.h>

#include "Common/Util/SwizzleVector.h"
#include "Common/Util/Auxiliary.h"
#include "Common/Util/Singleton.h"
#include "Common/Util/Reflection.h"
#include "Common/FrameWorkCore.h"
#include "Common/ObjectPtr.h"

namespace FW
{
	extern FRAMEWORK_API FString GAppName;
    extern FRAMEWORK_API TArray<FName> GProjectCategoryNames;
}

#define PER_MODULE_DEFINITION() PER_MODULE_BOILERPLATE

#define PER_APP_DEFINITION(AppName) \
    static const int AppNameSetter = [] { \
    FW::GAppName = TEXT(AppName); \
    /*Parts of the ApplicationCore depend on ue ProjectName*/ \
    FApp::SetProjectName(TEXT(AppName));  \
    return 0; }(); \
    PER_MODULE_DEFINITION()

#define SH_LOG(CategoryName, Verbosity, Format, ...) \
    UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
    FW::GProjectCategoryNames.Emplace(#CategoryName)

#define ADD_AGILITY_SDK()	\
	extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }	\
	extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = ".\\AgilitySDK\\"; }

#include <Internationalization/StringTableRegistry.h>
#define LOCALIZATION(Key) \
	FText::FromStringTable(TEXT("Localization"), Key)

PRAGMA_DISABLE_DEPRECATION_WARNINGS
	
