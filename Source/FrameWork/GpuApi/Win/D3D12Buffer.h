#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"
namespace FRAMEWORK
{
	enum class BufferUsage
	{
		Upload = D3D12_HEAP_TYPE_UPLOAD,
		ReadBack = D3D12_HEAP_TYPE_READBACK,
		Default = D3D12_HEAP_TYPE_DEFAULT,
	};
	
	class Dx12Buffer : public GpuBuffer
	{
	public:
		Dx12Buffer(TRefCountPtr<ID3D12Resource> InResource)
            : Resource(MoveTemp(InResource))
            , MappedData(nullptr)
        {}
        
    public:
		ID3D12Resource* GetResource() const { return Resource; }
        virtual void* GetMappedData() override {
			if (!MappedData)
			{
				MappedData = Map();
			}
            return MappedData;
        }

	private:
		void* Map() {
			Resource->Map(0, nullptr, static_cast<void**>(&MappedData));
			return MappedData;
		}
		
	private:
		TRefCountPtr<ID3D12Resource> Resource;
		void* MappedData{};
	};

    TRefCountPtr<Dx12Buffer> CreateDx12Buffer(uint64 BufferSize, BufferUsage InUsage);

}
