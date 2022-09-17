#pragma once
#include "PathHelper.h"

namespace SH {
	namespace BaseResourcePath {
		inline const FString UE_StandaloneRenderShaderDir = PathHelper::ExternalDir() / TEXT("UE/Shader/D3D");
		inline const FString UE_SlateResourceDir = PathHelper::ResourceDir();
	}
}

