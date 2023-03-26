#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"
namespace FRAMEWORK
{
	class Dx12UploadBuffer : public GpuBuffer
	{
	public:
		Dx12UploadBuffer(uint64 BufferSize);
		ID3D12Resource* GetResource() const { return Resource; }
		void* Map() {
			Resource->Map(0, nullptr, static_cast<void**>(&MappedData));
			return MappedData;
		}

		void Unmap() {
			Resource->Unmap(0, nullptr);
		}

	private:
		TRefCountPtr<ID3D12Resource> Resource;
		void* MappedData{};
	};
}