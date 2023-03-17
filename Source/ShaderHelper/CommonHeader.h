#pragma once
// UE Headers
#if defined(_WIN64)
#include <UnrealDefinitionsWin.h>
#elif defined(__APPLE__)
#include <UnrealDefinitionsMac.h>
#endif
#include <SharedPCH.h>

//Project Headers
#include "GlobalBoilerplate.h" //From FrameWork
#include "ProjectDefinitions.h" //From self

//Custom Headers
#include "Common/Util/SwizzleVector.h"
#include "Common/Util/Auxiliary.h"


