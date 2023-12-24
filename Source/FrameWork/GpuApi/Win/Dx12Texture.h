#pragma once
#include "GpuApi/GpuResource.h"
#include "Dx12Common.h"
#include "Dx12Descriptor.h"
#include "Dx12Buffer.h"
#include "Dx12Util.h"

namespace FRAMEWORK
{
	class Dx12Sampler : public GpuSampler
	{
	public:
		D3D12_FILTER Filter;
		D3D12_TEXTURE_ADDRESS_MODE AddressU;
		D3D12_TEXTURE_ADDRESS_MODE AddressV;
		D3D12_TEXTURE_ADDRESS_MODE AddressW;
		D3D12_COMPARISON_FUNC ComparisonFunc;
	};

	class Dx12Texture : public GpuTexture, public TrackedResource, public Dx12DeferredDeleteObject<Dx12Texture>
	{
	public:
		Dx12Texture(D3D12_RESOURCE_STATES InState, TRefCountPtr<ID3D12Resource> InResource, 
			GpuTextureDesc InDesc, void* InSharedHandle = nullptr);
        
    public:
		ID3D12Resource* GetResource() const override { return Resource.GetReference(); }
		void* GetSharedHandle() const { 
			check(SharedHandle);
			return SharedHandle; 
		}
		
	private:
		TRefCountPtr<ID3D12Resource> Resource;
		void* SharedHandle;

	public:
		TUniquePtr<CpuDescriptor> RTV;
		TUniquePtr<CpuDescriptor> SRV;
		TRefCountPtr<Dx12Buffer> UploadBuffer;
		TRefCountPtr<Dx12Buffer> ReadBackBuffer;
		bool bIsMappingForWriting = false;
		
	};

	TRefCountPtr<Dx12Texture> CreateDx12Texture2D(const GpuTextureDesc& InTexDesc);
	TRefCountPtr<Dx12Sampler> CreateDx12Sampler(const GpuSamplerDesc& InSamplerDesc);
}
