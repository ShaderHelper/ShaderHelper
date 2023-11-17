#pragma once
#include "GpuApi/GpuResource.h"
#include "Dx12Common.h"
#include "Dx12Descriptor.h"
#include "Dx12Buffer.h"
#include "Dx12Util.h"

namespace FRAMEWORK
{
	class Dx12Texture : public GpuTexture, public TrackedResource
	{
	public:
		Dx12Texture(D3D12_RESOURCE_STATES InState, TRefCountPtr<ID3D12Resource> InResource, 
			GpuTextureDesc InDesc, void* InSharedHandle = nullptr);
		~Dx12Texture();
        
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
		CpuDescriptorHandle HandleRTV;
		GpuDescriptorHandle HandleSRV;
		TRefCountPtr<Dx12Buffer> UploadBuffer;
		TRefCountPtr<Dx12Buffer> ReadBackBuffer;
		bool bIsMappingForWriting = false;
		
	};

	TRefCountPtr<Dx12Texture> CreateDx12Texture2D(const GpuTextureDesc& InTexDesc);
}
