#include "CommonHeader.h"
#include "Dx12CommandList.h"
#include "Dx12Map.h"

namespace FRAMEWORK
{	
	CommandListContext::CommandListContext()
		: RtvAllocator(256, DescriptorType::Rtv)
		, Cpu_CbvSrvUavAllocator(1024, DescriptorType::CbvSrvUav)
		, Gpu_CbvSrvUavAllocator(1024 , DescriptorType::CbvSrvUav)
		, CurrentPso(nullptr)
		, CurrentVertexBuffer(nullptr)
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
        , IsBindGroup0Dirty(false)
		, IsBindGroup1Dirty(false)
		, IsBindGroup2Dirty(false)
		, IsBindGroup3Dirty(false)
	{

		DxCheck(GDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CommandAllocator.GetInitReference())));
		DxCheck(GDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, nullptr, IID_PPV_ARGS(GraphicsCmdList.GetInitReference())));
		DxCheck(GraphicsCmdList->Close());
		InitCommandListState();
	}

	void CommandListContext::InitCommandListState()
	{
		check(CommandAllocator);
		DxCheck(GraphicsCmdList->Reset(CommandAllocator, nullptr));

		ID3D12DescriptorHeap* Heaps[] = {
			Gpu_CbvSrvUavAllocator.GetDescriptorHeap(),
		};
		GraphicsCmdList->SetDescriptorHeaps(UE_ARRAY_COUNT(Heaps), Heaps);
	}

	ID3D12GraphicsCommandList* CommandListContext::GetCommandListHandle()
	{
		if (CommandAllocator)
		{
			return GraphicsCmdList;
		}

		TRefCountPtr<ID3D12CommandAllocator> FreeCmdAllocator = RetrieveFreeCommandAllocator();
		if (FreeCmdAllocator)
		{
			CommandAllocator = FreeCmdAllocator;
		}
		else
		{
			TRefCountPtr<ID3D12CommandAllocator> NewCmdAllocator;
			DxCheck(GDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(NewCmdAllocator.GetInitReference())));
			CommandAllocator = MoveTemp(NewCmdAllocator);
		}

		InitCommandListState();
		return GraphicsCmdList;
	}


	TRefCountPtr<ID3D12CommandAllocator> CommandListContext::RetrieveFreeCommandAllocator()
	{
		for (auto It = PendingCommandAllocators.CreateIterator(); It; It++)
		{
			auto& PendingCmdAllocator = *It;
			if (PendingCmdAllocator.IsFree())
			{
				PendingCmdAllocator.Reset();
				FreeCommandAllocatorPool.Enqueue(PendingCmdAllocator);
				It.RemoveCurrent();
				break;
			}			
		}

		if (!FreeCommandAllocatorPool.IsEmpty())
		{
			TRefCountPtr<ID3D12CommandAllocator> FreeCmdAllocator;
			FreeCommandAllocatorPool.Dequeue(FreeCmdAllocator);
			return FreeCmdAllocator;
		}
		
		return nullptr;
	}

	TUniquePtr<CpuDescriptor> CommandListContext::AllocRtv()
	{
		return RtvAllocator.Allocate();
	}

	TUniquePtr<CpuDescriptor> CommandListContext::AllocCpuCbvSrvUav()
	{
		return Cpu_CbvSrvUavAllocator.Allocate();
	}

	TUniquePtr<GpuDescriptorRange> CommandListContext::AllocGpuCbvSrvUavRange(uint32 InDescriptorNum)
	{
		return Gpu_CbvSrvUavAllocator.Allocate(InDescriptorNum);
	}

	void CommandListContext::Transition(TrackedResource* InResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After)
	{
		if (Before != After)
		{
			CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(InResource->GetResource(), Before, After);
			GraphicsCmdList->ResourceBarrier(1, &Barrier);
			GResourceStateTracker.SetResourceState(InResource, After);
		}
	
	}

	void CommandListContext::SubmitCommandList()
	{
		check(GGraphicsQueue);
		//The commandLists must be closed before executing them.
		DxCheck(GraphicsCmdList->Close());
		ID3D12CommandList* CmdLists[] = { GraphicsCmdList };
		GGraphicsQueue->ExecuteCommandLists(1, CmdLists);

		PendingCommandAllocators.Add(CommandAllocator);

		CommandAllocator = nullptr;
	}

	void CommandListContext::PrepareDrawingEnv()
	{
		check(GraphicsCmdList);

		if (!CurrentViewPort.IsSet())
		{
			check(CurrentRenderTargets.Num() > 0);

			D3D12_VIEWPORT DefaultViewPort{};
			DefaultViewPort.Width = (float)CurrentRenderTargets[0]->GetWidth();
			DefaultViewPort.Height = (float)CurrentRenderTargets[0]->GetHeight();
			DefaultViewPort.MinDepth = 0;
			DefaultViewPort.MaxDepth = 1;
			DefaultViewPort.TopLeftX = 0;
			DefaultViewPort.TopLeftY = 0;

			D3D12_RECT DefaultScissorRect = CD3DX12_RECT(0, 0, (LONG)DefaultViewPort.Width, (LONG)DefaultViewPort.Height);

			SetViewPort(MoveTemp(DefaultViewPort), MoveTemp(DefaultScissorRect));
		}

		if(IsViewportDirty)
		{
			GraphicsCmdList->RSSetViewports(1, &*CurrentViewPort);
			GraphicsCmdList->RSSetScissorRects(1, &*CurrentSissorRect);
			MarkViewportDirty(false);
		}

		if (IsVertexBufferDirty) 
		{
			if (CurrentVertexBuffer) {
			}
			else {
                GraphicsCmdList->IASetVertexBuffers(0, 0, nullptr);
			}
			MarkVertexBufferDirty(false);
		}
		
		if (IsPipelineDirty)
		{
			check(CurrentPso);
			GraphicsCmdList->SetPipelineState(CurrentPso->GetResource());
			GraphicsCmdList->IASetPrimitiveTopology(CurrentPso->GetPritimiveTopology());
			MarkPipelineDirty(false);
		}

		if (IsRootSigDirty)
		{
			check(CurrentRootSignature);
			GraphicsCmdList->SetGraphicsRootSignature(CurrentRootSignature->GetResource());
			MarkRootSigDirty(false);
		}

		if (CurrentBindGroup0 && IsBindGroup0Dirty) {
			CurrentBindGroup0->Apply(GetCommandListHandle(), CurrentRootSignature);
			MarkBindGroup0Dirty(false);
		}

		if (CurrentBindGroup1 && IsBindGroup1Dirty) {
			CurrentBindGroup1->Apply(GetCommandListHandle(), CurrentRootSignature);
			MarkBindGroup1Dirty(false);
		}

		if (CurrentBindGroup2 && IsBindGroup2Dirty) {
			CurrentBindGroup2->Apply(GetCommandListHandle(), CurrentRootSignature);
			MarkBindGroup2Dirty(false);
		}

		if (CurrentBindGroup3 && IsBindGroup3Dirty) {
			CurrentBindGroup3->Apply(GetCommandListHandle(), CurrentRootSignature);
			MarkBindGroup3Dirty(false);
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
                check(RenderTarget->RTV->IsValid());
                
                Vector4f OptimizedClearValue = RenderTarget->GetResourceDesc().ClearValues;
                if (ClearColorValues[i]) {
                    Vector4f ClearColorValue = *ClearColorValues[i];
                    if (!ClearColorValue.Equals(OptimizedClearValue))
                    {
                        SH_LOG(LogDx12, Warning, TEXT("OptimizedClearValue(%s) != ClearColorValue(%s) that may result in invalid fast clear optimization."), *OptimizedClearValue.ToString(), *ClearColorValue.ToString());
                    }
                    GraphicsCmdList->ClearRenderTargetView(RenderTarget->RTV->GetHandle(), ClearColorValue.GetData(), 0, nullptr);
                }
                else {
                    GraphicsCmdList->ClearRenderTargetView(RenderTarget->RTV->GetHandle(), OptimizedClearValue.GetData(), 0, nullptr);
                }
                RenderTargetDescriptors[i] = RenderTarget->RTV->GetHandle();
            }
            
            GraphicsCmdList->OMSetRenderTargets(RenderTargetNum, RenderTargetDescriptors.GetData(), false, nullptr);
			MarkRenderTartgetDirty(false);
		}
	}

    void CommandListContext::ClearBinding()
    {
        CurrentPso = nullptr;
		CurrentViewPort.Reset();
        CurrentSissorRect.Reset();
		CurrentRootSignature = nullptr;
		CurrentBindGroup0 = nullptr;
		CurrentBindGroup1 = nullptr;
		CurrentBindGroup2 = nullptr;
		CurrentBindGroup3 = nullptr;
        
        CurrentRenderTargets.Empty();
        
        ClearColorValues.Reset();
    }

	void InitCommandListContext()
	{
		GCommandListContext = new CommandListContext();
	}

	CommandListContext::PendingCommandAllocator::PendingCommandAllocator(TRefCountPtr<ID3D12CommandAllocator> InCmdAllocator)
		: CommandAllocator(InCmdAllocator)
	{
		DxCheck(GDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.GetInitReference())));
		DxCheck(GGraphicsQueue->Signal(Fence, 1));
	}
}
