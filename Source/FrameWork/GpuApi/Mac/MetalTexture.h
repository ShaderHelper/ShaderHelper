#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
	class MetalTexture : public GpuTexture
	{
    public:
        MetalTexture(mtlpp::Texture InTex)
            : Tex(MoveTemp(InTex))
        {}
        
    public:
        id<MTLTexture> GetResource() const {
            return Tex.GetPtr();
        }
        
    private:
        mtlpp::Texture Tex;
	};

    TRefCountPtr<MetalTexture> CreateMetalTexture2D(const GpuTextureDesc& InTexDesc);
}
