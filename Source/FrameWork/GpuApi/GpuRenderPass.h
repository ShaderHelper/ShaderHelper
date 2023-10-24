#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"

namespace FRAMEWORK
{
	struct GpuViewPortDesc
	{
		uint32 Width, Height;

		float ZMin = 0.0f;
		float ZMax = 1.0f;
		float TopLeftX = 0.0f;
		float TopLeftY = 0.0f;
	};

	class GpuRenderTargetInfo
	{
	public:
		GpuRenderTargetInfo(GpuTexture* InTexture,
			RenderTargetLoadAction InLoadAction = RenderTargetLoadAction::DontCare,
			RenderTargetStoreAction InStoreAction = RenderTargetStoreAction::DontCare)
			: Texture(InTexture)
			, LoadAction(InLoadAction)
			, StoreAction(InStoreAction)
		{}
	public:
		GpuTexture* GetRenderTarget() const { return Texture; }
		void SetClearColor(Vector4f InClearColor) { ClearColor = MoveTemp(InClearColor); }


	public:
		RenderTargetLoadAction LoadAction;
		RenderTargetStoreAction StoreAction;
		TOptional<Vector4f> ClearColor;

	private:
		GpuTexture* Texture;
	};

	struct GpuRenderPassDesc
	{
		TArray<GpuRenderTargetInfo, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> ColorRenderTargets;
	};

}
