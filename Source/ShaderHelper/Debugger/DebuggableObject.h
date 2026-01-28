#pragma once
#include "Debugger/ShaderDebugger.h"

namespace SH
{
	struct DebugTargetInfo
	{
		TRefCountPtr<FW::GpuTexture> Tex;
	};

	class DebuggableObject
	{
	public:
		virtual DebugTargetInfo OnStartDebugging() = 0;
		virtual void OnFinalizePixel(const FW::Vector2u& PixelCoord) = 0;
		virtual ShaderAsset* GetShaderAsset() const = 0;
		virtual void OnEndDebuggging() = 0;
		virtual InvocationState GetInvocationState() = 0;
	};
}
