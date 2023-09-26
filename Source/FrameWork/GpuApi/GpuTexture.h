#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{
    struct GpuTextureDesc
    {
        GpuTextureDesc(
            uint32 InWidth,
            uint32 InHeight,
            GpuTextureFormat InFormat,
            GpuTextureUsage InUsage = GpuTextureUsage::None,
            Vector4f InClearValues = Vector4f{ 0 },
            uint32 InDepth = 1,
            uint32 InNumMips = 1
        )
            : Width(InWidth), Height(InHeight), Depth(InDepth)
            , NumMips(InNumMips)
            , Format(InFormat)
            , Usage(InUsage)
            , ClearValues(MoveTemp(InClearValues))
        {

        }

        uint32 Width;
        uint32 Height;
        uint32 Depth;
        uint32 NumMips;
        GpuTextureFormat Format;
        GpuTextureUsage Usage;
        Vector4f ClearValues;

        TArray<uint8> InitialData;
    };

    class GpuTexture : public GpuResource
    {
    public:
        GpuTexture(GpuTextureDesc InDesc)
            : TexDesc(MoveTemp(InDesc))
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
