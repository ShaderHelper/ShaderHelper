#include "CommonHeader.h"
#include "Dx12CommandRecorder.h"
#include "Dx12Map.h"

namespace FW
{	
	Dx12StateCache::Dx12StateCache()
		: IsPipelineStateDirty(false)
		, IsRenderTargetDirty(false)
		, IsVertexBufferDirty(false)
		, IsViewportDirty(false)
		, IsGraphicsRootSigDirty(false)
		, IsGraphicsBindGroup0Dirty(false)
		, IsGraphicsBindGroup1Dirty(false)
		, IsGraphicsBindGroup2Dirty(false)
		, IsGraphicsBindGroup3Dirty(false)
		, IsComputeRootSigDirty(false)
		, IsComputeBindGroup0Dirty(false)
		, IsComputeBindGroup1Dirty(false)
		, IsComputeBindGroup2Dirty(false)
		, IsComputeBindGroup3Dirty(false)
	{

	}

	void Dx12StateCache::ApplyComputeState(ID3D12GraphicsCommandList* InCmdList)
	{
		check(InCmdList);

		if (IsPipelineStateDirty && CurrentPso.IsType<Dx12ComputePso*>())
		{
			Dx12ComputePso* CurComputePso = CurrentPso.Get<Dx12ComputePso*>();
			check(CurComputePso);
			InCmdList->SetPipelineState(CurComputePso->GetResource());
			IsPipelineStateDirty = false;
		}

		if (IsComputeRootSigDirty)
		{
			check(CurrentComputeRootSignature);
			InCmdList->SetComputeRootSignature(CurrentComputeRootSignature->GetResource());
			IsComputeRootSigDirty = false;
		}

		if (CurrentComputeBindGroup0 && IsComputeBindGroup0Dirty) {
			CurrentComputeBindGroup0->ApplyComputeBinding(InCmdList, CurrentComputeRootSignature);
			IsComputeBindGroup0Dirty = false;
		}

		if (CurrentComputeBindGroup1 && IsComputeBindGroup1Dirty) {
			CurrentComputeBindGroup1->ApplyComputeBinding(InCmdList, CurrentComputeRootSignature);
			IsComputeBindGroup1Dirty = false;
		}

		if (CurrentComputeBindGroup2 && IsComputeBindGroup2Dirty) {
			CurrentComputeBindGroup2->ApplyComputeBinding(InCmdList, CurrentComputeRootSignature);
			IsComputeBindGroup2Dirty = false;
		}

		if (CurrentComputeBindGroup3 && IsComputeBindGroup3Dirty) {
			CurrentComputeBindGroup3->ApplyComputeBinding(InCmdList, CurrentComputeRootSignature);
			IsComputeBindGroup3Dirty = false;
		}

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

		if (IsPipelineStateDirty && CurrentPso.IsType<Dx12RenderPso*>())
		{
			Dx12RenderPso* CurRenderPso = CurrentPso.Get<Dx12RenderPso*>();
			check(CurRenderPso);
			InCmdList->SetPipelineState(CurRenderPso->GetResource());
			InCmdList->IASetPrimitiveTopology(CurRenderPso->GetPritimiveTopology());
			IsPipelineStateDirty = false;
		}

		if (IsGraphicsRootSigDirty)
		{
			check(CurrentGraphicsRootSignature);
			InCmdList->SetGraphicsRootSignature(CurrentGraphicsRootSignature->GetResource());
			IsGraphicsRootSigDirty = false;
		}

		if (CurrentGraphicsBindGroup0 && IsGraphicsBindGroup0Dirty) {
			CurrentGraphicsBindGroup0->ApplyDrawBinding(InCmdList, CurrentGraphicsRootSignature);
			IsGraphicsBindGroup0Dirty = false;
		}

		if (CurrentGraphicsBindGroup1 && IsGraphicsBindGroup1Dirty) {
			CurrentGraphicsBindGroup1->ApplyDrawBinding(InCmdList, CurrentGraphicsRootSignature);
			IsGraphicsBindGroup1Dirty = false;
		}

		if (CurrentGraphicsBindGroup2 && IsGraphicsBindGroup2Dirty) {
			CurrentGraphicsBindGroup2->ApplyDrawBinding(InCmdList, CurrentGraphicsRootSignature);
			IsGraphicsBindGroup2Dirty = false;
		}

		if (CurrentGraphicsBindGroup3 && IsGraphicsBindGroup3Dirty) {
			CurrentGraphicsBindGroup3->ApplyDrawBinding(InCmdList, CurrentGraphicsRootSignature);
			IsGraphicsBindGroup3Dirty = false;
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
                    Vector4f ClearColorValue = ClearColorValues[i];
                    if (!ClearColorValue.Equals(OptimizedClearValue))
                    {
                        SH_LOG(LogDx12, Warning, TEXT("OptimizedClearValue(%s) != ClearColorValue(%s) that may result in invalid fast clear optimization."), *OptimizedClearValue.ToString(), *ClearColorValue.ToString());
                    }
                    InCmdList->ClearRenderTargetView(RenderTarget->RTV->GetHandle(), ClearColorValue.GetData(), 0, nullptr);
					RenderTargetDescriptors[i] = RenderTarget->RTV->GetHandle();
				}

				InCmdList->OMSetRenderTargets(RenderTargetNum, RenderTargetDescriptors.GetData(), false, nullptr);
			}
			IsRenderTargetDirty = false;
		}
	}

	void Dx12StateCache::Clear()
	{
		CurrentPso = {};
		CurrentViewPort.Reset();
		CurrentSissorRect.Reset();

		CurrentGraphicsRootSignature = nullptr;
		CurrentGraphicsBindGroup0 = nullptr;
		CurrentGraphicsBindGroup1 = nullptr;
		CurrentGraphicsBindGroup2 = nullptr;
		CurrentGraphicsBindGroup3 = nullptr;

		CurrentComputeRootSignature = nullptr;
		CurrentComputeBindGroup0 = nullptr;
		CurrentComputeBindGroup1 = nullptr;
		CurrentComputeBindGroup2 = nullptr;
		CurrentComputeBindGroup3 = nullptr;

		CurrentRenderTargets.Empty();

		ClearColorValues.Reset();

		IsPipelineStateDirty = false;
		IsRenderTargetDirty = false;
		IsVertexBufferDirty = false;
		IsViewportDirty = false;

		IsGraphicsRootSigDirty = false;
		IsGraphicsBindGroup0Dirty = false;
		IsGraphicsBindGroup1Dirty = false;
		IsGraphicsBindGroup2Dirty = false;
		IsGraphicsBindGroup3Dirty = false;

		IsComputeRootSigDirty = false;
		IsComputeBindGroup0Dirty = false;
		IsComputeBindGroup1Dirty = false;
		IsComputeBindGroup2Dirty = false;
		IsComputeBindGroup3Dirty = false;

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
		if (CurrentPso.IsType<Dx12RenderPso*>() && CurrentPso.Get<Dx12RenderPso*>() != InPso)
		{
			CurrentPso.Set<Dx12RenderPso*>(InPso);
			IsPipelineStateDirty = true;
		}
	}

	void Dx12StateCache::SetPipeline(Dx12ComputePso* InPso)
	{
		if (CurrentPso.IsType<Dx12ComputePso*>() && CurrentPso.Get<Dx12ComputePso*>() != InPso)
		{
			CurrentPso.Set<Dx12ComputePso*>(InPso);
			IsPipelineStateDirty = true;
		}
	}

	void Dx12StateCache::SetGraphicsRootSignature(Dx12RootSignature* InRootSignature)
	{
		if (CurrentGraphicsRootSignature != InRootSignature)
		{
			CurrentGraphicsRootSignature = InRootSignature;
			IsGraphicsRootSigDirty = true;
		}
	}

	void Dx12StateCache::SetComputeRootSignature(Dx12RootSignature* InRootSignature)
	{
		if (CurrentComputeRootSignature != InRootSignature)
		{
			CurrentComputeRootSignature = InRootSignature;
			IsComputeRootSigDirty = true;
		}
	}

	void Dx12StateCache::SetRenderTargets(TArray<Dx12Texture*> InRTs, TArray<Vector4f> InClearColorValues)
	{
		if (InRTs != CurrentRenderTargets || InClearColorValues != ClearColorValues)
		{
			CurrentRenderTargets = MoveTemp(InRTs);
			ClearColorValues = MoveTemp(InClearColorValues);
			IsRenderTargetDirty = true;
		}
	}

	void Dx12StateCache::SetGraphicsBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3)
	{
		if (InGroup0 != CurrentGraphicsBindGroup0) {
			CurrentGraphicsBindGroup0 = InGroup0;
			IsGraphicsBindGroup0Dirty = true;
		}
		if (InGroup1 != CurrentGraphicsBindGroup1) {
			CurrentGraphicsBindGroup1 = InGroup1;
			IsGraphicsBindGroup1Dirty = true;
		}
		if (InGroup2 != CurrentGraphicsBindGroup2) {
			CurrentGraphicsBindGroup2 = InGroup2;
			IsGraphicsBindGroup2Dirty = true;
		}
		if (InGroup3 != CurrentGraphicsBindGroup3) {
			CurrentGraphicsBindGroup3 = InGroup3;
			IsGraphicsBindGroup3Dirty = true;
		}
	}

	void Dx12StateCache::SetComputeBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3)
	{
		if (InGroup0 != CurrentComputeBindGroup0) {
			CurrentComputeBindGroup0 = InGroup0;
			IsComputeBindGroup0Dirty = true;
		}
		if (InGroup1 != CurrentComputeBindGroup1) {
			CurrentGraphicsBindGroup1 = InGroup1;
			IsComputeBindGroup1Dirty = true;
		}
		if (InGroup2 != CurrentComputeBindGroup2) {
			CurrentComputeBindGroup2 = InGroup2;
			IsComputeBindGroup2Dirty = true;
		}
		if (InGroup3 != CurrentComputeBindGroup3) {
			CurrentComputeBindGroup3 = InGroup3;
			IsComputeBindGroup3Dirty = true;
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

	void Dx12RenderPassRecorder::SetRenderPipelineState(GpuRenderPipelineState* InPipelineState)
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

		StateCache.SetGraphicsRootSignature(Dx12RootSignatureManager::GetRootSignature(RsDesc));
		StateCache.SetGraphicsBindGroups(Dx12BindGroup0, Dx12BindGroup1, Dx12BindGroup2, Dx12BindGroup3);
	}


	void Dx12ComputePassRecorder::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
		StateCache.ApplyComputeState(CmdList);
		CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	void Dx12ComputePassRecorder::SetComputePipelineState(GpuComputePipelineState* InPipelineState)
	{
		StateCache.SetPipeline(static_cast<Dx12ComputePso*>(InPipelineState));
	}

	void Dx12ComputePassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
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

		StateCache.SetComputeRootSignature(Dx12RootSignatureManager::GetRootSignature(RsDesc));
		StateCache.SetComputeBindGroups(Dx12BindGroup0, Dx12BindGroup1, Dx12BindGroup2, Dx12BindGroup3);
	}

	GpuComputePassRecorder* Dx12CmdRecorder::BeginComputePass(const FString& PassName)
	{
		BeginCaptureEvent(PassName);
		auto NewPassRecorder = MakeUnique<Dx12ComputePassRecorder>(CmdList, StateCache);
		RequestedComputePassRecorders.Add(MoveTemp(NewPassRecorder));
		return RequestedComputePassRecorders.Last().Get();
	}

	void Dx12CmdRecorder::EndComputePass(GpuComputePassRecorder* InComputePassRecorder)
	{
		EndCaptureEvent();
	}

	GpuRenderPassRecorder* Dx12CmdRecorder::BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
	{
		BeginCaptureEvent(PassName);

		TArray<Dx12Texture*> RTs;
		TArray<Vector4f> ClearColorValues;

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

        D3D12_RESOURCE_DESC Desc = Texture->GetResource()->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
		GDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Layout, nullptr, nullptr, nullptr);

		const CommonAllocationData& AllocationData = Buffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
        CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ Texture->GetResource() };
        CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ AllocationData.UnderlyResource, Layout };
		CmdList->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc, nullptr);
	}

	void Dx12CmdRecorder::CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer)
	{
        Dx12Texture* Texture = static_cast<Dx12Texture*>(InTexture);
        Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InBuffer);

        D3D12_RESOURCE_DESC Desc = Texture->GetResource()->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
        GDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Layout, nullptr, nullptr, nullptr);

        const CommonAllocationData& AllocationData = Buffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
        CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ AllocationData.UnderlyResource, Layout };
        CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ Texture->GetResource() };
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
