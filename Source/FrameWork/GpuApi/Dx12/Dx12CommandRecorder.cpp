#include "CommonHeader.h"
#include "Dx12CommandRecorder.h"
#include "Dx12Map.h"

namespace FW
{	
	Dx12StateCache::Dx12StateCache()
		: CurrentPso(nullptr)
		, CurrentVertexBuffer(nullptr)
		, CurrentRootSignature(nullptr)
		, CurrentBindGroup0(nullptr)
		, CurrentBindGroup1(nullptr)
		, CurrentBindGroup2(nullptr)
		, CurrentBindGroup3(nullptr)
		, IsRenderPipelineDirty(false)
		, IsRenderTargetDirty(false)
		, IsVertexBufferDirty(false)
		, IsViewportDirty(false)
		, IsRootSigDirty(false)
		, IsBindGroup0Dirty(false)
		, IsBindGroup1Dirty(false)
		, IsBindGroup2Dirty(false)
		, IsBindGroup3Dirty(false)
	{

	}

	void Dx12StateCache::ApplyDrawState(ID3D12GraphicsCommandList* InCmdList)
	{
		check(InCmdList);

		if (!CurrentViewPort.IsSet())
		{
			if (CurrentRenderTargets.Num() > 0)
			{
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
		}

		if (IsViewportDirty)
		{
			if (CurrentViewPort)
			{
				InCmdList->RSSetViewports(1, &*CurrentViewPort);
				InCmdList->RSSetScissorRects(1, &*CurrentSissorRect);
				IsViewportDirty = false;
			}
		}

		if (IsVertexBufferDirty)
		{
			if (CurrentVertexBuffer) {
			}
			else {
				InCmdList->IASetVertexBuffers(0, 0, nullptr);
			}
			IsVertexBufferDirty = false;
		}

		if (IsRenderPipelineDirty)
		{
			check(CurrentPso);
			InCmdList->SetPipelineState(CurrentPso->GetResource());
			InCmdList->IASetPrimitiveTopology(CurrentPso->GetPritimiveTopology());
            IsRenderPipelineDirty = false;
		}

		if (IsRootSigDirty)
		{
			check(CurrentRootSignature);
			InCmdList->SetGraphicsRootSignature(CurrentRootSignature->GetResource());
			IsRootSigDirty = false;
		}

		if (CurrentBindGroup0 && IsBindGroup0Dirty) {
			CurrentBindGroup0->ApplyDrawBinding(InCmdList, CurrentRootSignature);
			IsBindGroup0Dirty = false;
		}

		if (CurrentBindGroup1 && IsBindGroup1Dirty) {
			CurrentBindGroup1->ApplyDrawBinding(InCmdList, CurrentRootSignature);
			IsBindGroup1Dirty = false;
		}

		if (CurrentBindGroup2 && IsBindGroup2Dirty) {
			CurrentBindGroup2->ApplyDrawBinding(InCmdList, CurrentRootSignature);
			IsBindGroup2Dirty = false;
		}

		if (CurrentBindGroup3 && IsBindGroup3Dirty) {
			CurrentBindGroup3->ApplyDrawBinding(InCmdList, CurrentRootSignature);
			IsBindGroup3Dirty = false;
		}

		if (IsRenderTargetDirty)
		{
			const uint32 RenderTargetNum = CurrentRenderTargets.Num();
			if (RenderTargetNum > 0)
			{
				check(ClearColorValues.Num() == RenderTargetNum);

				TArray<D3D12_CPU_DESCRIPTOR_HANDLE> RenderTargetDescriptors;
				RenderTargetDescriptors.SetNumUninitialized(RenderTargetNum);

				for (uint32 i = 0; i < RenderTargetNum; i++)
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
						InCmdList->ClearRenderTargetView(RenderTarget->RTV->GetHandle(), ClearColorValue.GetData(), 0, nullptr);
					}
					else {
						InCmdList->ClearRenderTargetView(RenderTarget->RTV->GetHandle(), OptimizedClearValue.GetData(), 0, nullptr);
					}
					RenderTargetDescriptors[i] = RenderTarget->RTV->GetHandle();
				}

				InCmdList->OMSetRenderTargets(RenderTargetNum, RenderTargetDescriptors.GetData(), false, nullptr);
			}
			IsRenderTargetDirty = false;
		}
	}

	void Dx12StateCache::Clear()
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

        IsRenderPipelineDirty = false;
		IsRenderTargetDirty = false;
		IsVertexBufferDirty = false;
		IsViewportDirty = false;
		IsRootSigDirty = false;
		IsBindGroup0Dirty = false;
		IsBindGroup1Dirty = false;
		IsBindGroup2Dirty = false;
		IsBindGroup3Dirty = false;
	}

	void Dx12StateCache::SetViewPort(D3D12_VIEWPORT InViewPort, D3D12_RECT InSissorRect)
	{
		if (!CurrentViewPort || FMemory::Memcmp(&*CurrentViewPort, &InViewPort, sizeof(D3D12_VIEWPORT)))
		{
			CurrentViewPort = MoveTemp(InViewPort);
			CurrentSissorRect = MoveTemp(InSissorRect);
			IsViewportDirty = true;
		}
	}

	void Dx12StateCache::SetVertexBuffer(Dx12Buffer* InBuffer)
	{
		if (CurrentVertexBuffer != InBuffer)
		{
			CurrentVertexBuffer = InBuffer;
			IsVertexBufferDirty = true;
		}
	}

	void Dx12StateCache::SetPipeline(Dx12RenderPso* InPso)
	{
		if (CurrentPso != InPso)
		{
			CurrentPso = InPso;
            IsRenderPipelineDirty = true;
		}
	}

	void Dx12StateCache::SetRootSignature(Dx12RootSignature* InRootSignature)
	{
		if (CurrentRootSignature != InRootSignature)
		{
			CurrentRootSignature = InRootSignature;
			IsRootSigDirty = true;
		}
	}

	void Dx12StateCache::SetRenderTargets(TArray<Dx12Texture*> InRTs, TArray<TOptional<Vector4f>> InClearColorValues)
	{
		if (InRTs != CurrentRenderTargets || InClearColorValues != ClearColorValues)
		{
			CurrentRenderTargets = MoveTemp(InRTs);
			ClearColorValues = MoveTemp(InClearColorValues);
			IsRenderTargetDirty = true;
		}
	}

	void Dx12StateCache::SetBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3)
	{
		if (InGroup0 != CurrentBindGroup0) {
			CurrentBindGroup0 = InGroup0;
			IsBindGroup0Dirty = true;
		}
		if (InGroup1 != CurrentBindGroup1) {
			CurrentBindGroup1 = InGroup1;
			IsBindGroup1Dirty = true;
		}
		if (InGroup2 != CurrentBindGroup2) {
			CurrentBindGroup2 = InGroup2;
			IsBindGroup2Dirty = true;
		}
		if (InGroup3 != CurrentBindGroup3) {
			CurrentBindGroup3 = InGroup3;
			IsBindGroup3Dirty = true;
		}
	}

	TSharedPtr<Dx12CmdRecorder> Dx12CmdRecorderPool::RetrieveFreeCmdRecorder()
	{
		for (auto It = ActiveDx12CmdRecorders.CreateIterator(); It; It++)
		{
			auto& ActiveDx12CmdRecorder = *It;
			if (ActiveDx12CmdRecorder->IsFree())
			{
				ActiveDx12CmdRecorder->Reset();
				FreeDx12CmdRecorders.Enqueue(ActiveDx12CmdRecorder);
				It.RemoveCurrent();
				break;
			}			
		}

		if (!FreeDx12CmdRecorders.IsEmpty())
		{
			TSharedPtr<Dx12CmdRecorder> FreeCmdRecorder;
			FreeDx12CmdRecorders.Dequeue(FreeCmdRecorder);
			return FreeCmdRecorder;
		}
		
		return nullptr;
	}

	Dx12CmdRecorder* Dx12CmdRecorderPool::ObtainDxCmdRecorder(const FString& RecorderName)
	{
		TSharedPtr<Dx12CmdRecorder> FreeDx12CmdRecorder = RetrieveFreeCmdRecorder();
		if (FreeDx12CmdRecorder)
		{
			FreeDx12CmdRecorder->SetName(RecorderName);
			ActiveDx12CmdRecorders.Emplace(FreeDx12CmdRecorder);
			return FreeDx12CmdRecorder.Get();
		}
		else
		{
			TRefCountPtr<ID3D12GraphicsCommandList> NewCmdList;
			TRefCountPtr<ID3D12CommandAllocator> NewCmdAllocator;
			DxCheck(GDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(NewCmdAllocator.GetInitReference())));
			DxCheck(GDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, NewCmdAllocator, nullptr, IID_PPV_ARGS(NewCmdList.GetInitReference())));
			TSharedPtr<Dx12CmdRecorder> NewCmdRecorder = MakeShared<Dx12CmdRecorder>(MoveTemp(NewCmdList), MoveTemp(NewCmdAllocator));
			NewCmdRecorder->SetName(RecorderName);
			ActiveDx12CmdRecorders.Emplace(NewCmdRecorder);
			return NewCmdRecorder.Get();
		}
	}

	void Dx12RenderPassRecorder::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
		StateCache.ApplyDrawState(CmdList);
		CmdList->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	void Dx12RenderPassRecorder::SetRenderPipelineState(GpuPipelineState* InPipelineState)
	{
		StateCache.SetPipeline(static_cast<Dx12RenderPso*>(InPipelineState));
	}

	void Dx12RenderPassRecorder::SetVertexBuffer(GpuBuffer* InVertexBuffer)
	{
		StateCache.SetVertexBuffer(static_cast<Dx12Buffer*>(InVertexBuffer));
	}

	void Dx12RenderPassRecorder::SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
		D3D12_VIEWPORT ViewPort{};
		ViewPort.Width = InViewPortDesc.Width;
		ViewPort.Height = InViewPortDesc.Height;
		ViewPort.MinDepth = InViewPortDesc.ZMin;
		ViewPort.MaxDepth = InViewPortDesc.ZMax;
		ViewPort.TopLeftX = InViewPortDesc.TopLeftX;
		ViewPort.TopLeftY = InViewPortDesc.TopLeftY;

		D3D12_RECT ScissorRect = CD3DX12_RECT(0, 0, (LONG)InViewPortDesc.Width, (LONG)InViewPortDesc.Height);
		StateCache.SetViewPort(MoveTemp(ViewPort), MoveTemp(ScissorRect));
	}

	void Dx12RenderPassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		RootSignatureDesc RsDesc{
		BindGroup0 ? static_cast<Dx12BindGroupLayout*>(BindGroup0->GetLayout()) : nullptr,
		BindGroup1 ? static_cast<Dx12BindGroupLayout*>(BindGroup1->GetLayout()) : nullptr,
		BindGroup2 ? static_cast<Dx12BindGroupLayout*>(BindGroup2->GetLayout()) : nullptr,
		BindGroup3 ? static_cast<Dx12BindGroupLayout*>(BindGroup3->GetLayout()) : nullptr
		};

		Dx12BindGroup* Dx12BindGroup0 = static_cast<Dx12BindGroup*>(BindGroup0);
		Dx12BindGroup* Dx12BindGroup1 = static_cast<Dx12BindGroup*>(BindGroup1);
		Dx12BindGroup* Dx12BindGroup2 = static_cast<Dx12BindGroup*>(BindGroup2);
		Dx12BindGroup* Dx12BindGroup3 = static_cast<Dx12BindGroup*>(BindGroup3);

		StateCache.SetRootSignature(Dx12RootSignatureManager::GetRootSignature(RsDesc));
		StateCache.SetBindGroups(Dx12BindGroup0, Dx12BindGroup1, Dx12BindGroup2, Dx12BindGroup3);
	}

	GpuRenderPassRecorder* Dx12CmdRecorder::BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
	{
		BeginCaptureEvent(PassName);

		TArray<Dx12Texture*> RTs;
		TArray<TOptional<Vector4f>> ClearColorValues;

		for (int32 i = 0; i < PassDesc.ColorRenderTargets.Num(); i++) {
			Dx12Texture* Rt = static_cast<Dx12Texture*>(PassDesc.ColorRenderTargets[i].Texture);
			RTs.Add(Rt);
			ClearColorValues.Add(PassDesc.ColorRenderTargets[i].ClearColor);
		}

		StateCache.SetRenderTargets(MoveTemp(RTs), MoveTemp(ClearColorValues));
		auto NewPassRecorder = MakeUnique<Dx12RenderPassRecorder>(CmdList, StateCache);
		RequestedRenderPassRecorders.Add(MoveTemp(NewPassRecorder));
		return RequestedRenderPassRecorders.Last().Get();
	}

	void Dx12CmdRecorder::EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder)
	{
		EndCaptureEvent();
	}

	void Dx12CmdRecorder::BeginCaptureEvent(const FString& EventName)
	{
#if GPU_API_CAPTURE && USE_PIX
		PIXBeginEvent(CmdList.GetReference(), PIX_COLOR_DEFAULT, *EventName);
#endif
	}

	void Dx12CmdRecorder::EndCaptureEvent()
	{
#if GPU_API_CAPTURE && USE_PIX
		PIXEndEvent(CmdList.GetReference());
#endif
	}

	void Dx12CmdRecorder::Barrier(GpuTrackedResource* InResource, GpuResourceState NewState)
	{
		check(InResource->State != GpuResourceState::Unknown);
		D3D12_RESOURCE_STATES BeforeDx12State = MapResourceState(InResource->State);
		D3D12_RESOURCE_STATES AfterDx12State = MapResourceState(NewState);
		if (BeforeDx12State == AfterDx12State) {
			return;
		}

		CD3DX12_RESOURCE_BARRIER Barrier;
		if (InResource->GetType() == GpuResourceType::Texture) {
			Barrier = CD3DX12_RESOURCE_BARRIER::Transition(static_cast<Dx12Texture*>(InResource)->GetResource(), BeforeDx12State, AfterDx12State);
		}
		else {
			check(false);
		}
		InResource->State = NewState;
		CmdList->ResourceBarrier(1, &Barrier);
	}

	void Dx12CmdRecorder::CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture)
	{
		Dx12Texture* Texture = static_cast<Dx12Texture*>(InTexture);
		Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InBuffer);
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
		D3D12_RESOURCE_DESC Desc = Texture->GetResource()->GetDesc();
		GDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Layout, nullptr, nullptr, nullptr);

		const CommonAllocationData& AllocationData = Buffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
		CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ AllocationData.UnderlyResource, Layout };
		CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ Texture->GetResource() };
		CmdList->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc, nullptr);
	}

	void Dx12CmdRecorder::CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer)
	{
		Dx12Texture* Texture = static_cast<Dx12Texture*>(InTexture);
		Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InBuffer);
		CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ Texture->GetResource() };

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
		D3D12_RESOURCE_DESC Desc = Texture->GetResource()->GetDesc();
		GDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Layout, nullptr, nullptr, nullptr);

		const CommonAllocationData& AllocationData = Buffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
		CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ AllocationData.UnderlyResource, Layout };
		CmdList->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc, nullptr);
	}

	void Dx12CmdRecorder::Reset()
	{
		check(IsFree());
		DxCheck(CommandAllocator->Reset());
		DxCheck(CmdList->Reset(CommandAllocator, nullptr));
		DxCheck(Fence->Signal(0));
		RequestedRenderPassRecorders.Empty();
		StateCache.Clear();
		BindDescriptorHeap();
	}

	void Dx12CmdRecorder::SetName(const FString& Name)
	{
		CmdList->SetName(*Name);
		CommandAllocator->SetName(*Name);
	}

	void Dx12CmdRecorder::BindDescriptorHeap()
	{
		ID3D12DescriptorHeap* Heaps[] = {
			Gpu_CbvSrvUavAllocator->GetDescriptorHeap(),
			Gpu_SamplerAllocator->GetDescriptorHeap()
		};
		CmdList->SetDescriptorHeaps(UE_ARRAY_COUNT(Heaps), Heaps);
	}

}
