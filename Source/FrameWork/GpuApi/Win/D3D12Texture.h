#pragma once
#include "GpuApi/GpuResource.h"
#include "D3D12Common.h"
#include "D3D12Descriptor.h"
#include "D3D12Buffer.h"
#include "D3D12Util.h"
#include "D3D12CommandList.h"

namespace FRAMEWORK
{
	class Dx12Texture : public GpuTexture, public TrackedResource
	{
	public:
		Dx12Texture(D3D12_RESOURCE_STATES InState, TRefCountPtr<ID3D12Resource> InResource)
			: TrackedResource(InState), Resource(MoveTemp(InResource))
		{}
		ID3D12Resource* GetResource() const override { return Resource.GetReference(); }

		~Dx12Texture() {
			auto& DescriptorAllocators = GCommandListContext->GetCurFrameResource().GetDescriptorAllocators();
			DescriptorAllocators.RtvAllocator->Free(HandleRTV);
			DescriptorAllocators.SrvAllocator->Free(HandleSRV);
		}
		
	private:
		TRefCountPtr<ID3D12Resource> Resource;

	public:
		DescriptorHandle<Descriptorvisibility::CpuVisible> HandleRTV;
		DescriptorHandle<Descriptorvisibility::CpuGpuVisible> HandleSRV;
		TRefCountPtr<Dx12UploadBuffer> UploadBuffer;
		bool bIsMappingForWriting = false;
	};

	TRefCountPtr<Dx12Texture> CreateDx12Texture(const GpuTextureDesc& InTexDesc);
	DXGI_FORMAT MapTextureFormat(GpuTextureFormat InTexFormat);
}
