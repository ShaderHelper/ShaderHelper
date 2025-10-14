#pragma once
#include "PathHelper.h"

namespace FW
{
namespace BaseResourcePath
{
#if PLATFORM_MAC || PLATFORM_LINUX
    inline const FString UE_StandaloneRenderShaderDir = PathHelper::ResourceDir() / TEXT("Shaders/UE/OpenGL");
#elif PLATFORM_WINDOWS
    inline const FString UE_StandaloneRenderShaderDir = PathHelper::ResourceDir() / TEXT("Shaders/UE/D3D");
#endif
	inline const FString UE_CoreStyleDir = PathHelper::ResourceDir();
    inline const FString UE_SlateResourceDir = PathHelper::ResourceDir() / TEXT("Slate");
	inline const FString Custom_SlateResourceDir = PathHelper::ResourceDir() / TEXT("CustomSlate");
    inline const FString UE_SlateFontDir = UE_SlateResourceDir / TEXT("Fonts");
}
}

