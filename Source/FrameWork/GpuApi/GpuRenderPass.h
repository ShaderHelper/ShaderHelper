#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"

namespace FW
{
	struct GpuViewPortDesc
	{
		float Width;
		float Height;

		float ZMin = 0.0f;
		float ZMax = 1.0f;
		float TopLeftX = 0.0f;
		float TopLeftY = 0.0f;
	};

	struct GpuRenderTargetInfo
	{
		GpuTexture* Texture;
		RenderTargetLoadAction LoadAction = RenderTargetLoadAction::DontCare;
		RenderTargetStoreAction StoreAction = RenderTargetStoreAction::DontCare;
		TOptional<Vector4f> ClearColor;
	};

	struct GpuRenderPassDesc
	{
		TArray<GpuRenderTargetInfo, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> ColorRenderTargets;
	};

}
