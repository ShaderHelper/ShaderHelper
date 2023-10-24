#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{
    struct GpuTextureDesc
    {
        uint32 Width;
        uint32 Height;
		GpuTextureFormat Format;

        GpuTextureUsage Usage = GpuTextureUsage::None;
        Vector4f ClearValues = 0;
		uint32 Depth = 1;
		uint32 NumMips = 1;
		TArray<uint8> InitialData{};
    };

    class GpuTexture : public GpuResource
    {
    public:
        GpuTexture(GpuTextureDesc InDesc)
            : GpuResource(GpuResourceType::Texture)
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
