#include "D3D12Common.h"
#include "D3D12Descriptor.h"
#include "D3D12Device.h"

namespace FRAMEWORK
{
	extern void InitFrameResource();

	class FrameResource : public FNoncopyable
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
		void BindToCommandList(ID3D12GraphicsCommandList* InGraphicsCmdList);
		const DescriptorAllocatorStorage& GetDescriptorAllocators() const { return DescriptorAllocators; }

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
		void ResetFrameResource(uint32 FrameResourceIndex);
		void BindFrameResource(uint32 FrameResourceIndex);
		const FrameResource& GetCurFrameResource() const {
			uint32 Index = GetCurFrameSourceIndex();
			return FrameResources[Index];
		}

	private:
		FrameResourceStorage FrameResources;
		TRefCountPtr<ID3D12GraphicsCommandList> GraphicsCmdList;
	};

	inline TUniquePtr<CommandListContext> GCommandListContext;
}