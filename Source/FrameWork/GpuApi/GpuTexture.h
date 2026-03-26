#pragma once
#include "GpuResourceCommon.h"

namespace FW
{
	class GpuTextureView;

    struct GpuTextureDesc
    {
        uint32 Width;
        uint32 Height;
		GpuFormat Format;
		GpuTextureUsage Usage = GpuTextureUsage::None;
 
		TArrayView<uint8> InitialData;
        Vector4f ClearValues{0,0,0,1};
		uint32 Depth = 1;
		// 0 means auto-calculate mip count and generate mipmaps.
		uint32 NumMips = 1;
		GpuTextureDimension Dimension = GpuTextureDimension::Tex2D;
    };

    class GpuTexture : public GpuResource
    {
    public:
        GpuTexture(GpuTextureDesc InDesc, GpuResourceState InState)
            : GpuResource(GpuResourceType::Texture)
			, TexDesc(MoveTemp(InDesc))
        {
			SubResourceStates.SetNum(GetNumSubResources());
			for (auto& S : SubResourceStates) S = InState;
		}
        
    public:
        const GpuTextureDesc& GetResourceDesc() const { return TexDesc; }
        GpuFormat GetFormat() const { return TexDesc.Format; }
        uint32 GetWidth() const { return TexDesc.Width; }
        uint32 GetHeight() const { return TexDesc.Height; }
		uint32 GetDepth() const { return TexDesc.Depth; }
		uint32 GetNumMips() const { return TexDesc.NumMips; }
		uint32 GetArrayLayerCount() const { return TexDesc.Dimension == GpuTextureDimension::TexCube ? 6 : 1; }
		uint32 GetNumSubResources() const { return GetNumMips() * GetArrayLayerCount(); }
		virtual uint32 GetAllocationSize() const { AUX::Unreachable(); return 0; }

		GpuTextureView* GetDefaultView() const { return DefaultView; }

		uint32 CalcSubResourceIndex(uint32 MipLevel, uint32 ArraySlice) const { return MipLevel + ArraySlice * TexDesc.NumMips; }
		GpuResourceState GetSubResourceState(uint32 MipLevel, uint32 ArraySlice) const { return SubResourceStates[CalcSubResourceIndex(MipLevel, ArraySlice)]; }
		void SetSubResourceState(uint32 MipLevel, uint32 ArraySlice, GpuResourceState NewState) { SubResourceStates[CalcSubResourceIndex(MipLevel, ArraySlice)] = NewState; }
		void SetAllSubResourceStates(GpuResourceState NewState) { for (auto& S : SubResourceStates) S = NewState; }

	protected:
		TRefCountPtr<GpuTextureView> DefaultView;

    private:
        GpuTextureDesc TexDesc;
		TArray<GpuResourceState> SubResourceStates;
    };

	struct GpuTextureViewDesc
	{
		GpuTexture* Texture;
		uint32 BaseMipLevel = 0;
		uint32 MipLevelCount = 1;
		uint32 BaseArrayLayer = 0;
		uint32 ArrayLayerCount = 1;
	};

	class GpuTextureView : public GpuResource
	{
	public:
		GpuTextureView(GpuTextureViewDesc InDesc)
			: GpuResource(GpuResourceType::TextureView)
			, ViewDesc(MoveTemp(InDesc))
		{
		}

		GpuTexture* GetTexture() const { return ViewDesc.Texture; }
		uint32 GetBaseMipLevel() const { return ViewDesc.BaseMipLevel; }
		uint32 GetMipLevelCount() const { return ViewDesc.MipLevelCount; }
		uint32 GetBaseArrayLayer() const { return ViewDesc.BaseArrayLayer; }
		uint32 GetArrayLayerCount() const { return ViewDesc.ArrayLayerCount; }
		const GpuTextureViewDesc& GetViewDesc() const { return ViewDesc; }

		uint32 GetWidth() const { return FMath::Max(ViewDesc.Texture->GetWidth() >> ViewDesc.BaseMipLevel, 1u); }
		uint32 GetHeight() const { return FMath::Max(ViewDesc.Texture->GetHeight() >> ViewDesc.BaseMipLevel, 1u); }

	protected:
		GpuTextureViewDesc ViewDesc;
	};
}
