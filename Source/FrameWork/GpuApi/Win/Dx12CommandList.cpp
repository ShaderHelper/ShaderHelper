#include "CommonHeader.h"
#include "Dx12CommandList.h"
#include "Dx12Map.h"

namespace FRAMEWORK
{	
	CommandListContext::CommandListContext(FrameResourceStorage InitFrameResources, TRefCountPtr<ID3D12GraphicsCommandList> InGraphicsCmdList)
		: FrameResources(MoveTemp(InitFrameResources))
		, GraphicsCmdList(MoveTemp(InGraphicsCmdList))
		, CurrentPso(nullptr)
		, CurrentVertexBuffer(nullptr)
		, DrawType(PrimitiveType::Triangle)
		, CurrentRootSignature(nullptr)
		, CurrentBindGroup0(nullptr)
		, CurrentBindGroup1(nullptr)
		, CurrentBindGroup2(nullptr)
		, CurrentBindGroup3(nullptr)
		, IsPipelineDirty(false)
		, IsRenderTargetDirty(false)
		, IsVertexBufferDirty(false)
		, IsViewportDirty(false)
		, IsRootSigDirty(false)
        , IsBindGroupsDirty(false)
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
		ensureMsgf(Before != After, TEXT("Transitioning between the same resource sates: %d ?"), Before);
		CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(InResource, Before, After);
		GraphicsCmdList->ResourceBarrier(1, &Barrier);
	}

	void CommandListContext::PrepareDrawingEnv()
	{
		if (IsVertexBufferDirty) 
		{
			if (CurrentVertexBuffer) {
				//TODO
                GDeferredReleaseManager->AddUncompletedResource(CurrentVertexBuffer);
			}
			else {
                GraphicsCmdList->IASetVertexBuffers(0, 0, nullptr);
			}
			MarkVertexBufferDirty(false);
		}

        GraphicsCmdList->IASetPrimitiveTopology(MapPrimitiveType(DrawType));
		
		if (IsPipelineDirty)
		{
			check(CurrentPso);
			GraphicsCmdList->SetPipelineState(CurrentPso->GetResource());
            GDeferredReleaseManager->AddUncompletedResource(CurrentPso);
			MarkPipelineDirty(false);
		}

		if (IsRootSigDirty)
		{
			check(CurrentRootSignature);
			GraphicsCmdList->SetGraphicsRootSignature(CurrentRootSignature->GetResource());
			MarkRootSigDirty(false);
		}

		if (IsBindGroupsDirty)
		{

            if(CurrentBindGroup0) {
                CurrentBindGroup0->Apply(GetCommandListHandle(), CurrentRootSignature);
                GDeferredReleaseManager->AddUncompletedResource(CurrentBindGroup0);
                
            }
            
            if(CurrentBindGroup1) {
                CurrentBindGroup1->Apply(GetCommandListHandle(), CurrentRootSignature);
                GDeferredReleaseManager->AddUncompletedResource(CurrentBindGroup1);
                
            }
            
            if(CurrentBindGroup2) {
                CurrentBindGroup2->Apply(GetCommandListHandle(), CurrentRootSignature);
                GDeferredReleaseManager->AddUncompletedResource(CurrentBindGroup2);
                
            }
            
            if(CurrentBindGroup3) {
                CurrentBindGroup3->Apply(GetCommandListHandle(), CurrentRootSignature);
                GDeferredReleaseManager->AddUncompletedResource(CurrentBindGroup3);
                
            }

			MarkBindGroupsDirty(false);
		}
	
		if (IsViewportDirty)
		{
            check(CurrentViewPort.IsValid());
            check(CurrentSissorRect.IsValid());
            GraphicsCmdList->RSSetViewports(1, CurrentViewPort.Get());
            GraphicsCmdList->RSSetScissorRects(1, CurrentSissorRect.Get());
			MarkViewportDirty(false);
		}

		if (IsRenderTargetDirty)
		{
            const uint32 RenderTargetNum = CurrentRenderTargets.Num();
			check(RenderTargetNum);
            check(ClearColorValues.Num() == RenderTargetNum);
            
			TArray<D3D12_CPU_DESCRIPTOR_HANDLE> RenderTargetDescriptors;
			RenderTargetDescriptors.SetNumUninitialized(RenderTargetNum);
            
            for(uint32 i = 0; i < RenderTargetNum; i++)
            {
                Dx12Texture* RenderTarget = CurrentRenderTargets[i];
                check(RenderTarget);
                check(RenderTarget->HandleRTV.IsValid());
                
                Vector4f OptimizedClearValue = RenderTarget->GetResourceDesc().ClearValues;
                if (ClearColorValues[i]) {
                    Vector4f ClearColorValue = *ClearColorValues[i];
                    if (!ClearColorValue.Equals(OptimizedClearValue))
                    {
                        SH_LOG(LogDx12, Warning, TEXT("OptimizedClearValue(%s) != ClearColorValue(%s) that may result in invalid fast clear optimization."), *OptimizedClearValue.ToString(), *ClearColorValue.ToString());
                    }
                    GraphicsCmdList->ClearRenderTargetView(RenderTarget->HandleRTV.CpuHandle, ClearColorValue.GetData(), 0, nullptr);
                }
                else {
                    GraphicsCmdList->ClearRenderTargetView(RenderTarget->HandleRTV.CpuHandle, OptimizedClearValue.GetData(), 0, nullptr);
                }
                GDeferredReleaseManager->AddUncompletedResource(RenderTarget);
                RenderTargetDescriptors[i] = RenderTarget->HandleRTV.CpuHandle;
            }
            
            GraphicsCmdList->OMSetRenderTargets(RenderTargetNum, RenderTargetDescriptors.GetData(), false, nullptr);
			MarkRenderTartgetDirty(false);
		}
	}

    void CommandListContext::ClearBinding()
    {
        CurrentPso = nullptr;
        CurrentViewPort = nullptr;
        CurrentSissorRect = nullptr;
		CurrentRootSignature = nullptr;
		CurrentBindGroup0 = nullptr;
		CurrentBindGroup1 = nullptr;
		CurrentBindGroup2 = nullptr;
		CurrentBindGroup3 = nullptr;
        
        IsPipelineDirty = false;
        IsRenderTargetDirty = false;
        IsVertexBufferDirty = false;
        IsViewportDirty = false;
		IsRootSigDirty = false;
		IsBindGroupsDirty = false;
        
        CurrentRenderTargets.Empty();
        
        ClearColorValues.Reset();
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
		DescriptorAllocators.ShaderViewAllocator->Reset();
		DescriptorAllocators.SamplerAllocator->Reset();
	}

	void StaticFrameResource::BindToCommandList(ID3D12GraphicsCommandList* InGraphicsCmdList)
	{
		check(InGraphicsCmdList);
		//CommandList can be reset when already submit it to gpu.
		InGraphicsCmdList->Reset(CommandAllocator, nullptr);

		ID3D12DescriptorHeap* Heaps[] = {
			DescriptorAllocators.ShaderViewAllocator->GetDescriptorHeap(),
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
			DescriptorAllocators.ShaderViewAllocator.Reset(new StaticFrameResource::ShaderViewAllocatorType());
			DescriptorAllocators.SamplerAllocator.Reset(new StaticFrameResource::SamplerAllocatorType());

			FrameResources.Emplace(MoveTemp(CommandAllocator), MoveTemp(DescriptorAllocators));
		}

		TRefCountPtr<ID3D12GraphicsCommandList> GraphicsCmdList;
		DxCheck(GDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, InitialCommandAllocator, nullptr, IID_PPV_ARGS(GraphicsCmdList.GetInitReference())));
		DxCheck(GraphicsCmdList->Close());
		GCommandListContext = new CommandListContext(MoveTemp(FrameResources), MoveTemp(GraphicsCmdList));
	}

}
