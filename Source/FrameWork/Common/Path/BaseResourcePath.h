#pragma once
#include "PathHelper.h"

namespace FW
{
namespace BaseResourcePath
{
    inline const FString UE_StandaloneRenderGLShaderDir = PathHelper::ResourceDir() / TEXT("Shaders/UE/OpenGL");
    inline const FString UE_StandaloneRenderDxShaderDir = PathHelper::ResourceDir() / TEXT("Shaders/UE/D3D");

	inline const FString UE_CoreStyleDir = PathHelper::ResourceDir();
    inline const FString UE_SlateResourceDir = PathHelper::ResourceDir() / TEXT("Slate");
	inline const FString Custom_SlateResourceDir = PathHelper::ResourceDir() / TEXT("CustomSlate");
    inline const FString UE_SlateFontDir = UE_SlateResourceDir / TEXT("Fonts");
}
}

