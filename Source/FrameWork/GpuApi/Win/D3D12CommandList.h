#include "D3D12Common.h"
#include "D3D12Descriptor.h"
#include "D3D12Device.h"

namespace FRAMEWORK
{
	extern void InitFrameResource();

	class FrameResource
	{
	public:
		using RtvAllocatorType = CpuDescriptorAllocator<256, DescriptorType::RTV>;
		using SrvAllocatorType = GpuDescriptorAllocator<1024, DescriptorType::SRV>;
		using SamplerAllocatorType = GpuDescriptorAllocator<256, DescriptorType::SAMPLER>;

		struct DescriptorAllocatorStorage
		{
			TUniquePtr<RtvAllocatorType> RtvAllocator;
			TUniquePtr<SrvAllocatorType> SrvAllocator;
			TUniquePtr<SamplerAllocatorType> SamplerAllocator;
		};
		FrameResource(TRefCountPtr<ID3D12CommandAllocator> InCommandAllocator, DescriptorAllocatorStorage&& InDescriptorAllocators);
		void Reset();
		void BindToCommandList(ID3D12GraphicsCommandList* GraphicsCmdList);

	private:
		TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
		DescriptorAllocatorStorage DescriptorAllocators;
	};

	class CommandListContext
	{
	public:
		using FrameResourceStorage = TArray<FrameResource, TFixedAllocator<FrameSourceNum>>;
	public:
		CommandListContext(FrameResourceStorage&& InitFrameResources, TRefCountPtr<ID3D12GraphicsCommandList> InGraphicsCmdList);
		void Reset(uint32 FrameResourceIndex);

	private:
		FrameResourceStorage FrameResources;
		TRefCountPtr<ID3D12GraphicsCommandList> GraphicsCmdList;
	};

	inline TUniquePtr<CommandListContext> GCommandListContext;
}