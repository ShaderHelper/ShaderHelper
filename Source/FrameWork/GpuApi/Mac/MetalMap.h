#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalTexture.h"

namespace FRAMEWORK
{

    inline MTLLoadAction MapLoadAction(RenderTargetLoadAction InLoadAction)
    {
        switch(InLoadAction)
        {
        case RenderTargetLoadAction::DontCare:      return MTLLoadActionDontCare;
        case RenderTargetLoadAction::Load:          return MTLLoadActionLoad;
        case RenderTargetLoadAction::Clear:         return MTLLoadActionClear;
        default:
            SH_LOG(LogMetal, Fatal, TEXT("Invalid LoadAction."));
            return MTLLoadActionDontCare;
        }
    }

    inline MTLStoreAction MapStoreAction(RenderTargetStoreAction InStoreAction)
    {
        switch(InStoreAction)
        {
        case RenderTargetStoreAction::DontCare:      return MTLStoreActionDontCare;
        case RenderTargetStoreAction::Store:         return MTLStoreActionStore;
        default:
            SH_LOG(LogMetal, Fatal, TEXT("Invalid StoreAction."));
            return MTLStoreActionDontCare;
        }
    }
    
    inline mtlpp::RenderPassDescriptor MapRenderPassDesc(const GpuRenderPassDesc& PassDesc)
    {
        MTLRenderPassDescriptor* RawPassDesc = [MTLRenderPassDescriptor new];
        uint32 PassRtNum = PassDesc.ColorRenderTargets.Num();
        for(uint32 i = 0 ; i < PassRtNum; i++)
        {
            const GpuRenderTargetInfo& RtInfo = PassDesc.ColorRenderTargets[i];
            MetalTexture* Rt = static_cast<MetalTexture*>(RtInfo.GetRenderTarget());
            RawPassDesc.colorAttachments[i].texture = Rt->GetResource();
            RawPassDesc.colorAttachments[i].loadAction = MapLoadAction(RtInfo.LoadAction);
            RawPassDesc.colorAttachments[i].storeAction = MapStoreAction(RtInfo.StoreAction);
        }
        return mtlpp::RenderPassDescriptor{RawPassDesc};
    }

}
