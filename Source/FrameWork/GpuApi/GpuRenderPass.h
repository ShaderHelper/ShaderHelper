#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"

namespace FRAMEWORK
{
	struct GpuViewPortDesc
	{
		GpuViewPortDesc(uint32 InWidth, uint32 InHeight, float InZMin = 0.0f, float InZMax = 1.0f, float InTopLeftX = 0.0f, float InTopLeftY = 0.0f)
			: TopLeftX(InTopLeftX)
			, TopLeftY(InTopLeftY)
			, Width(InWidth)
			, Height(InHeight)
			, ZMin(InZMin)
			, ZMax(InZMax)
		{
		}
		float TopLeftX, TopLeftY;
		uint32 Width, Height;
		float ZMin, ZMax;
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
