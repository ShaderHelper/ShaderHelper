#pragma once

#define EXPAND(x) x
#define TEST_LOG(...) EXPAND(UE_INTERNAL_LOG_IMPL(__VA_ARGS__))
#undef UE_LOG
#define UE_LOG(...)

#define PER_MODULE_DEFINITION() \
    REPLACEMENT_OPERATOR_NEW_AND_DELETE

namespace FRAMEWORK {}
using namespace FRAMEWORK;

