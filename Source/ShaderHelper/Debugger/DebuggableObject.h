#pragma once
#include "Debugger/ShaderDebugger.h"

namespace SH
{
	enum class DebugItem
	{
		Vertex,
		Pixel,
		Compute,
	};

	struct DebugTargetOutput
	{
		FString Name;
		TRefCountPtr<FW::GpuTexture> Tex;
	};

	struct DebugTargetInfo
	{
		TRefCountPtr<FW::GpuTexture> Tex;
		TArray<DebugTargetOutput> Outputs;
		TRefCountPtr<FW::GpuTexture> CoverageMask;
		int32 SelectedOutputIndex = 0;

		void Normalize()
		{
			if (Outputs.IsEmpty() && Tex)
			{
				Outputs.Add({ TEXT("RT"), Tex });
			}
			if (!Tex && Outputs.IsValidIndex(SelectedOutputIndex))
			{
				Tex = Outputs[SelectedOutputIndex].Tex;
			}
		}
	};

	class DebuggableObject
	{
	public:
		virtual TArray<DebugItem> GetSupportedDebugItems() const = 0;
		virtual DebugTargetInfo OnStartDebugging(DebugItem Item) = 0;
		virtual void OnFinalizePixel(const FW::Vector2u& PixelCoord) {}
		virtual void OnFinalizeCompute(const FW::Vector3u& WorkGroupId, const FW::Vector3u& LocalInvocationId) {}
		virtual ShaderAsset* GetShaderAsset(DebugItem Item) const = 0;
		virtual void OnEndDebuggging() = 0;
		virtual InvocationState GetInvocationState(DebugItem Item) = 0;
	};
}
