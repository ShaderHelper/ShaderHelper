#pragma once
#include "PathHelper.h"

namespace FRAMEWORK
{
namespace BaseResourcePath
{
#if PLATFORM_MAC
    inline const FString UE_StandaloneRenderShaderDir = PathHelper::ExternalDir() / TEXT("UE/Shader/OpenGL");
#elif PLATFORM_WINDOWS
    inline const FString UE_StandaloneRenderShaderDir = PathHelper::ExternalDir() / TEXT("UE/Shader/D3D");
#endif
	
    inline const FString UE_SlateResourceDir = PathHelper::ResourceDir();
    inline const FString UE_SlateFontDir = UE_SlateResourceDir / TEXT("Slate/Fonts");
}
}

