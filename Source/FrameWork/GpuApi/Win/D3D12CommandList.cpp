#include "CommonHeader.h"
#include "D3D12CommandList.h"

namespace FRAMEWORK
{
	
	CommandListContext::CommandListContext(FrameResourceStorage&& InitFrameResources, TRefCountPtr<ID3D12GraphicsCommandList> InGraphicsCmdList)
		: FrameResources(MoveTemp(InitFrameResources))
		, GraphicsCmdList(MoveTemp(InGraphicsCmdList))
	{

	}


	void CommandListContext::ResetFrameResource(uint32 FrameResourceIndex)
	{
		FrameResources[FrameResourceIndex].Reset();
	}

	void CommandListContext::BindFrameResource(uint32 FrameResourceIndex)
	{
		FrameResources[FrameResourceIndex].BindToCommandList(GraphicsCmdList);
	}

	FrameResource::FrameResource(TRefCountPtr<ID3D12CommandAllocator> InCommandAllocator, DescriptorAllocatorStorage&& InDescriptorAllocators)
		: CommandAllocator(MoveTemp(InCommandAllocator))
		, DescriptorAllocators(MoveTemp(InDescriptorAllocators))
	{

	}

	void FrameResource::Reset()
	{
		DxCheck(CommandAllocator->Reset());
		DescriptorAllocators.RtvAllocator->Reset();
		DescriptorAllocators.SrvAllocator->Reset();
		DescriptorAllocators.SamplerAllocator->Reset();
	}

	void FrameResource::BindToCommandList(ID3D12GraphicsCommandList* InGraphicsCmdList)
	{
		check(InGraphicsCmdList);
		//CommandList can be reset when already submit it to gpu.
		InGraphicsCmdList->Reset(CommandAllocator, nullptr);

		ID3D12DescriptorHeap* Heaps[] = {
			DescriptorAllocators.SrvAllocator->GetDescriptorHeap(),
			DescriptorAllocators.SamplerAllocator->GetDescriptorHeap()
		};
		InGraphicsCmdList->SetDescriptorHeaps(UE_ARRAY_COUNT(Heaps), Heaps);
	}

	void InitFrameResource()
	{
		TRefCountPtr<ID3D12GraphicsCommandList> GraphicsCmdList;
		DxCheck(GDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, nullptr, nullptr, IID_PPV_ARGS(GraphicsCmdList.GetInitReference())));
		DxCheck(GraphicsCmdList->Close());

		CommandListContext::FrameResourceStorage FrameResources;

		for (int i = 0; i < FrameSourceNum; ++i)
		{
			TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
			DxCheck(GDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CommandAllocator.GetInitReference())));

			FrameResource::DescriptorAllocatorStorage DescriptorAllocators;
			DescriptorAllocators.RtvAllocator.Reset(new FrameResource::RtvAllocatorType());
			DescriptorAllocators.SrvAllocator.Reset(new FrameResource::SrvAllocatorType());
			DescriptorAllocators.SamplerAllocator.Reset(new FrameResource::SamplerAllocatorType());

			FrameResources.Add(FrameResource(MoveTemp(CommandAllocator), MoveTemp(DescriptorAllocators)));
		}

		GCommandListContext.Reset(new CommandListContext(MoveTemp(FrameResources), MoveTemp(GraphicsCmdList)));
	}

}