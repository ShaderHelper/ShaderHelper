#pragma once
#include "GpuApi/GpuResource.h"
#include "D3D12Common.h"
#include "D3D12Descriptor.h"
#include "D3D12Buffer.h"
#include "D3D12Util.h"

namespace FRAMEWORK
{
	class Dx12Texture : public GpuTexture, public TrackedResource
	{
	public:
		Dx12Texture(D3D12_RESOURCE_STATES InState, TRefCountPtr<ID3D12Resource> InResource, GpuTextureDesc InDesc);
		~Dx12Texture();
        
    public:
		ID3D12Resource* GetResource() const override { return Resource.GetReference(); }
		
	private:
		TRefCountPtr<ID3D12Resource> Resource;

	public:
		DescriptorHandle<Descriptorvisibility::CpuVisible> HandleRTV;
		DescriptorHandle<Descriptorvisibility::CpuGpuVisible> HandleSRV;
		TRefCountPtr<Dx12Buffer> UploadBuffer;
		TRefCountPtr<Dx12Buffer> ReadBackBuffer;
		bool bIsMappingForWriting = false;
	};

	TRefCountPtr<Dx12Texture> CreateDx12Texture2D(const GpuTextureDesc& InTexDesc);
}
