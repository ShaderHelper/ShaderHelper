#pragma once
#include "GpuResourceCommon.h"

namespace FW
{
    struct GpuTextureDesc
    {
        uint32 Width;
        uint32 Height;
		GpuTextureFormat Format;
		GpuTextureUsage Usage = GpuTextureUsage::None;
 
		TArray<uint8> InitialData{};
        Vector4f ClearValues{0,0,0,1};
		uint32 Depth = 1;
		uint32 NumMips = 1;
    };

    class GpuTexture : public GpuTrackedResource
    {
    public:
        GpuTexture(GpuTextureDesc InDesc)
            : GpuTrackedResource(GpuResourceType::Texture)
			, TexDesc(MoveTemp(InDesc))
        {}
        
    public:
        const GpuTextureDesc& GetResourceDesc() const { return TexDesc; }
        GpuTextureFormat GetFormat() const { return TexDesc.Format; }
        uint32 GetWidth() const { return TexDesc.Width; }
        uint32 GetHeight() const { return TexDesc.Height; }

    private:
        GpuTextureDesc TexDesc;
    };
}
