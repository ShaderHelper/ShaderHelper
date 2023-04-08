//Other modules should include the header when depending the module.
#pragma once

#define PER_MODULE_DEFINITION() \
    PER_MODULE_BOILERPLATE

#define IMPORT_MODULE_NAMESPACE(CurrentModuleName, ImportedModuleName) \
    namespace ImportedModuleName{} \
    namespace CurrentModuleName { using namespace ImportedModuleName; }

namespace FRAMEWORK
{
    extern FRAMEWORK_API TArray<FName> GProjectCategoryNames;
}

#define SH_LOG(CategoryName, Verbosity, Format, ...) \
    UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
    GProjectCategoryNames.Emplace(#CategoryName)
