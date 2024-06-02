#pragma once
//Only works for the current module.

#define GPU_API_DEBUG !SH_SHIPPING

#ifndef GPU_API_CAPTURE
    #define GPU_API_CAPTURE GPU_API_DEBUG
#endif

#ifndef DEBUG_SHADER
    #define DEBUG_SHADER GPU_API_DEBUG
#endif
