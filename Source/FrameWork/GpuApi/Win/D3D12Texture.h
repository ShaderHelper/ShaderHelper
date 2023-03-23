#pragma once
#include "GpuApi/GpuResource.h"
#include "D3D12Common.h"
#include "D3D12Descriptor.h"

namespace FRAMEWORK
{
	class Dx12Texture : public GpuTextureResource
	{
	public:
		Dx12Texture(TRefCountPtr<ID3D12Resource> InResource)
			: Resource(MoveTemp(InResource)) 
		{}
		ID3D12Resource* GetResource() const { return Resource.GetReference(); }
	public:
		DescriptorHandle<Descriptorvisibility::CpuVisible> HandleRTV;
		DescriptorHandle<Descriptorvisibility::CpuGpuVisible> HandleSRV;
	private:
		TRefCountPtr<ID3D12Resource> Resource;
	};

	TRefCountPtr<Dx12Texture> CreateDx12Texture(const GpuTextureDesc& InTexDesc);
	DXGI_FORMAT MapTextureFormat(GpuTextureFormat InTexFormat);
}
