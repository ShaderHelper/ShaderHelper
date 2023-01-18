#pragma once

#define PER_MODULE_DEFINITION() \
    REPLACEMENT_OPERATOR_NEW_AND_DELETE

namespace FRAMEWORK {}

//Imports Framework to SH project.
namespace SH
{
    using namespace FRAMEWORK;
}

