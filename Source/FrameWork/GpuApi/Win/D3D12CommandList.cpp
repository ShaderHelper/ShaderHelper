#include "CommonHeader.h"
#include "D3D12CommandList.h"

namespace FRAMEWORK
{
	
	CommandListContext::CommandListContext(FrameResourceStorage InitFrameResources, TRefCountPtr<ID3D12GraphicsCommandList> InGraphicsCmdList)
		: FrameResources(MoveTemp(InitFrameResources))
		, GraphicsCmdList(MoveTemp(InGraphicsCmdList))
		, CurrentPso(nullptr)
		, CurrentRenderTarget(nullptr)
	{

	}


	void CommandListContext::ResetStaticFrameResource(uint32 FrameResourceIndex)
	{
		FrameResources[FrameResourceIndex].Reset();
	}

	void CommandListContext::BindStaticFrameResource(uint32 FrameResourceIndex)
	{
		FrameResources[FrameResourceIndex].BindToCommandList(GraphicsCmdList);
	}

	void CommandListContext::Transition(ID3D12Resource* InResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After)
	{
		ensureMsgf(Before == After, TEXT("Transitioning between the same resource sates: %d ?"), Before, After);
		CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(InResource, Before, After);
		GraphicsCmdList->ResourceBarrier(1, &Barrier);
	}

	void CommandListContext::PrepareDrawingEnv()
	{
		check(CurrentPso);
		GraphicsCmdList->SetGraphicsRootSignature(CurrentPso->GetRootSig());
		GraphicsCmdList->SetPipelineState(CurrentPso->GetResource());


		GCommandListContext->GetCommandListHandle()->RSSetViewports(1, CurrentViewPort.Get());
		GCommandListContext->GetCommandListHandle()->RSSetScissorRects(1, CurrentSissorRect.Get());

		check(CurrentRenderTarget);
		check(CurrentRenderTarget->HandleRTV.IsValid());
		GCommandListContext->GetCommandListHandle()->OMSetRenderTargets(1, &CurrentRenderTarget->HandleRTV.CpuHandle, false, nullptr);

		Vector4f OptimizedClearValue = CurrentRenderTarget->GetResourceDesc().ClearValues;
		if (ClearColorValue.IsValid()) {
			if (!(*ClearColorValue).Equals(OptimizedClearValue))
			{
				SH_LOG(LogDx12, Warning, TEXT("OptimizedClearValue(%s) != ClearColorValue(%s) that may result in invalid fast clear optimization."), OptimizedClearValue.ToString(), (*ClearColorValue).ToString());
			}
			GCommandListContext->GetCommandListHandle()->ClearRenderTargetView(CurrentRenderTarget->HandleRTV.CpuHandle, (*ClearColorValue).GetData(), 0, nullptr);
		}
		else {
			GCommandListContext->GetCommandListHandle()->ClearRenderTargetView(CurrentRenderTarget->HandleRTV.CpuHandle, OptimizedClearValue.GetData(), 0, nullptr);
		}

		GDynamicFrameResourceManager.AddUncompletedResource(CurrentPso);
		GDynamicFrameResourceManager.AddUncompletedResource(CurrentRenderTarget);
	}

	StaticFrameResource::StaticFrameResource(TRefCountPtr<ID3D12CommandAllocator> InCommandAllocator, DescriptorAllocatorStorage&& InDescriptorAllocators)
		: CommandAllocator(MoveTemp(InCommandAllocator))
		, DescriptorAllocators(MoveTemp(InDescriptorAllocators))
	{

	}

	void StaticFrameResource::Reset()
	{
		DxCheck(CommandAllocator->Reset());
		DescriptorAllocators.RtvAllocator->Reset();
		DescriptorAllocators.SrvAllocator->Reset();
		DescriptorAllocators.SamplerAllocator->Reset();
	}

	void StaticFrameResource::BindToCommandList(ID3D12GraphicsCommandList* InGraphicsCmdList)
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
		CommandListContext::FrameResourceStorage FrameResources;
		ID3D12CommandAllocator* InitialCommandAllocator = nullptr;
		for (int i = 0; i < FrameSourceNum; ++i)
		{
			TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
			DxCheck(GDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CommandAllocator.GetInitReference())));
			if (i == 0) { InitialCommandAllocator = CommandAllocator.GetReference(); }

			StaticFrameResource::DescriptorAllocatorStorage DescriptorAllocators;
			DescriptorAllocators.RtvAllocator.Reset(new StaticFrameResource::RtvAllocatorType());
			DescriptorAllocators.SrvAllocator.Reset(new StaticFrameResource::SrvAllocatorType());
			DescriptorAllocators.SamplerAllocator.Reset(new StaticFrameResource::SamplerAllocatorType());

			FrameResources.Emplace(MoveTemp(CommandAllocator), MoveTemp(DescriptorAllocators));
		}

		TRefCountPtr<ID3D12GraphicsCommandList> GraphicsCmdList;
		DxCheck(GDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, InitialCommandAllocator, nullptr, IID_PPV_ARGS(GraphicsCmdList.GetInitReference())));
		DxCheck(GraphicsCmdList->Close());
		GCommandListContext.Reset(new CommandListContext(MoveTemp(FrameResources), MoveTemp(GraphicsCmdList)));
	}

}