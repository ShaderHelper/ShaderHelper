#pragma once
#include "CommonHeader.h"
#include "GpuResourceCommon.h"
#include "GpuTexture.h"

namespace FRAMEWORK
{
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
        TArray<GpuRenderTargetInfo, TFixedAllocator<MaxRenderTargetNum>> ColorRenderTargets;
    };

}
