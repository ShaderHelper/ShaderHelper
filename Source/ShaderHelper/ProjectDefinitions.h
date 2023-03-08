#pragma once

#define PER_MODULE_DEFINITION() \
	PER_MODULE_BOILERPLATE

namespace FRAMEWORK {}

//Imports Framework to SH project.
namespace SH
{
    using namespace FRAMEWORK;
}

